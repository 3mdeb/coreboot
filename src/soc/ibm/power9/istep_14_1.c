/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/istep_14_1.h>
#include <cpu/power/scom.h>

void StateMachine::doMaintCommand(WorkFlowProperties & i_wfp)
{
    uint64_t workItem;
    TargetHandle_t target;

    // starting a maint cmd ...  register a timeout monitor
    uint64_t maintCmdTO = getTimeoutValue();

    i_wfp.timeoutCnt = 0; // reset for new work item
    workItem = *i_wfp.workItem;
    // new command...use the full range
    fapi2::Target<fapi2::TARGET_TYPE_MCBIST> fapiMcbist(target);
    mss::mcbist::stop_conditions<mss::mc_type::NIMBUS> stopCond;
    switch(workItem)
    {
        case START_SCRUB:
            stopCond.set_pause_on_mpe(mss::ON);
            stopCond.set_pause_on_ue(mss::ON);
            stopCond.set_pause_on_aue(mss::ON);
            stopCond.set_nce_inter_symbol_count_enable(mss::ON);
            stopCond.set_nce_soft_symbol_count_enable(mss::ON);
            stopCond.set_nce_hard_symbol_count_enable(mss::ON);
            mss::memdiags::sf_read<mss::mc_type::NIMBUS>(fapiMcbist, stopCond);
            break;
        case START_PATTERN_0:
            sf_init(fapiMcbist);
            break;
    }
    i_wfp.timer = getMonitor().addMonitor(maintCmdTO);
}

fapi2::ReturnCode nim_sf_init( const fapi2::Target<fapi2::TARGET_TYPE_MCBIST>& i_target)
{
    return mss::memdiags::sf_init<mss::mc_type::NIMBUS>(i_target);
}

template< mss::mc_type MC, fapi2::TargetType T = mss::mcbistMCTraits<MC>::MC_TARGET_TYPE , typename TT = mcbistTraits<MC, T> >
fapi2::ReturnCode sf_read(
    const fapi2::Target<T>& i_target,
    const stop_conditions<MC>& i_stop)
{
    using ET = mss::mcbistMCTraits<MC>;
    fapi2::ReturnCode l_rc;
    constraints<MC> l_const(
        i_stop,
        speed::LUDICROUS,
        end_boundary::STOP_AFTER_SLAVE_RANK,
        mss::mcbist::address(),
        mss::mcbist::address(TT::LARGEST_ADDRESS));
    sf_read_operation<MC> l_read_op(i_target, l_const, l_rc);
    return l_read_op.execute();
}

template< mss::mc_type MC, fapi2::TargetType T, typename TT >
inline void operation<MC, T, TT>::base_init()
{
    // Check the state of the MCBIST engine to make sure its OK that we proceed.
    // Force stop the engine (per spec, as opposed to waiting our turn)
    memdiags::stop<MC>(iv_target);
    // Zero out cmd timebase - mcbist::program constructor does that for us.
    // Load pattern
    iv_program.change_pattern(iv_const.iv_pattern);
    // Load end boundaries
    iv_program.change_end_boundary(iv_const.iv_end_boundary);
    // Load thresholds
    iv_program.change_thresholds(iv_const.iv_stop);
    // Setup the requested speed
    iv_program.change_speed(iv_target, iv_const.iv_speed);
    // Enable maint addressing mode - enabled by default in the mcbist::program ctor
    // Apparently the MCBIST engine needs the ports selected even though the ports are specified
    // in the subtest. We can just select them all, and it adjusts when it executes the subtest
    iv_program.select_ports(0b1111);
    // Kick it off, don't wait for a result
    iv_program.change_async(mss::ON);
}

template< mss::mc_type MC, fapi2::TargetType T, typename TT >
inline fapi2::ReturnCode operation<MC, T, TT>::single_port_init()
{
    using ET = mcbistMCTraits<MC>;

    const uint64_t l_relative_port_number = iv_const.iv_start_address.get_port();
    const uint64_t l_dimm_number = iv_const.iv_start_address.get_dimm();

    // No broadcast mode for this one
    // Push on a read subtest
    {
        mss::mcbist::subtest_t<MC> l_subtest = iv_subtest;
        l_subtest.enable_port(l_relative_port_number);
        l_subtest.enable_dimm(l_dimm_number);
        iv_program.iv_subtests.push_back(l_subtest);
    }

    // The address should have the port and DIMM noted in it. All we need to do is calculate the
    // remainder of the address
    if (iv_const.iv_end_address == TT::LARGEST_ADDRESS)
    {
        // Only the DIMM range as we don't want to cross ports.
        iv_const.iv_start_address.template get_range<mss::mcbist::address::DIMM>(iv_const.iv_end_address);
    }

    // Configure the address range
    mss::mcbist::config_address_range0<MC>(iv_target, iv_const.iv_start_address, iv_const.iv_end_address);

    // Initialize the common sections
    base_init();

fapi_try_exit:
    return fapi2::current_err;
}

template<>
fapi2::ReturnCode operation<DEFAULT_MC_TYPE>::multi_port_init_internal()
{
    // Let's assume we are going to send out all subtest unless we are in broadcast mode,
    // where we only send up to 2 subtests under an MCA ( 1 for each DIMM) which is why no const
    auto l_dimms = mss::find_targets<fapi2::TARGET_TYPE_DIMM>(iv_target);

    // Get the port/DIMM information for the addresses. This is an integral value which allows us to index
    // all the DIMM across a controller.
    const uint64_t l_portdimm_start_address = iv_const.iv_start_address.get_port_dimm();
    const uint64_t l_portdimm_end_address = iv_const.iv_end_address.get_port_dimm();

    // If start address == end address we can handle the single port case simply
    if (l_portdimm_start_address == l_portdimm_end_address)
    {
        // Single port case; simple.
        return single_port_init();
    }

    // Determine which ports are functional and whether we can broadcast to them
    // If we're in broadcast mode, PRD sends DIMM 0/1 of the first functional and configured port,
    // and we then run all ports in parallel (ports set in subtest config)
    if( mss::mcbist::is_broadcast_capable(iv_target) == mss::YES )
    {
        const auto l_prev_size = l_dimms.size();
        broadcast_mode_start_address_check(iv_target, l_portdimm_start_address, l_dimms);
    }

    // Configures all subtests under an MCBIST
    // If we're here we know start port < end port. We want to run one subtest (for each DIMM) from start_address
    // to the max range of the start address port, then one subtest (for each DIMM) for each port between the
    // start/end ports and one test (for each DIMM) from the start of the end port to the end address.

    // Setup the address configurations
     multi_port_addr();

    // We need to do three things here. One is to create a subtest which starts at start address and runs to
    // the end of the port. Next, create subtests to go from the start of the next port to the end of the
    // next port. Last we need a subtest which goes from the start of the last port to the end address specified
    // in the end address. Notice this may mean one subtest (start and end are on the same port) or it might
    // mean two subtests (start is one one port, end is on the next.) Or it might mean three or more subtests.

    // Configure multiport subtests, can be all subtests for the DIMMs under an MCBIST,
    // or just the DIMMs under the first configured MCA if in broadcast mode.
    configure_multiport_subtests(l_dimms);

    // Here's an interesting problem. PRD (and others maybe) expect the operation to proceed in address-order.
    // That is, when PRD finds an address it stops on, it wants to continue from there "to the end." That means
    // we need to keep the subtests sorted, otherwise PRD could pass one subtest come upon a fail in a subsequent
    // subtest and re-test something it already passed. So we sort the resulting iv_subtest vector by port/DIMM
    // in the subtest.
    std::sort(iv_program.iv_subtests.begin(), iv_program.iv_subtests.end(),
              [](const decltype(iv_subtest)& a, const decltype(iv_subtest)& b) -> bool
    {
        const uint64_t l_a_portdimm = (a.get_port() << 1) | a.get_dimm();
        const uint64_t l_b_portdimm = (b.get_port() << 1) | b.get_dimm();
        return l_a_portdimm < l_b_portdimm;
    });
    // Initialize the common sections
    base_init();
    // And configure broadcast mode if required
    mss::mcbist::configure_broadcast_mode(iv_target, iv_program);

fapi_try_exit:
    return fapi2::current_err;
}

template< mss::mc_type MC, fapi2::TargetType T, typename TT >
inline fapi2::ReturnCode operation<MC, T, TT>::multi_port_init()
{
    const auto l_port = mss::find_targets<TT::PORT_TYPE>(iv_target);
    // Make sure we have ports, if we don't then exit out
    if(l_port.size() == 0)
    {
        // Cronus can have no ports under an MCBIST, FW deconfigures by association
        return fapi2::FAPI2_RC_SUCCESS;
    }
    // Let's assume we are going to send out all subtest unless we are in broadcast mode,
    // where we only send up to 2 subtests under an port ( 1 for each DIMM) which is why no const
    auto l_dimms = mss::find_targets<fapi2::TARGET_TYPE_DIMM>(iv_target);
    if( l_dimms.size() == 0)
    {
        // Cronus can have no DIMMS under an MCBIST, FW deconfigures by association
        return fapi2::FAPI2_RC_SUCCESS;
    }
    return multi_port_init_internal();
}

template< mss::mc_type MC = DEFAULT_MC_TYPE, fapi2::TargetType T = mss::mcbistMCTraits<MC>::MC_TARGET_TYPE , typename TT = mcbistTraits<MC, T> >
struct sf_read_operation : public operation<MC>
{
    sf_read_operation( const fapi2::Target<T>& i_target,
                       const constraints<MC> i_const,
                       fapi2::ReturnCode& o_rc):
    operation<MC>(i_target, mss::mcbist::read_subtest<MC>(), i_const)
    {
        o_rc = this->multi_port_init();
    }
};

void mss_get_address_range(
    const fapi2::Target<fapi2::TARGET_TYPE_MBA>& i_target,
    uint64_t& o_end_addr)
{
    static const uint8_t memConfigType[9][4][2] =
    {

        // Refer to Centaur Workbook: 5.2 Master and Slave Rank Usage
        //
        //       SUBTYPE_A                    SUBTYPE_B                    SUBTYPE_C                    SUBTYPE_D
        //
        //SLOT_0_ONLY   SLOT_0_AND_1   SLOT_0_ONLY   SLOT_0_AND_1   SLOT_0_ONLY   SLOT_0_AND_1   SLOT_0_ONLY   SLOT_0_AND_1
        //
        //master slave  master slave   master slave  master slave   master slave  master slave   master slave  master slave
        //
        {{0xff,         0xff},         {0xff,        0xff},         {0xff,         0xff},       {0xff,         0xff}},  // TYPE_0
        {{0x00,         0x40},         {0x10,        0x50},         {0x30,         0x70},       {0xff,         0xff}},  // TYPE_1
        {{0x01,         0x41},         {0x03,        0x43},         {0x07,         0x47},       {0xff,         0xff}},  // TYPE_2
        {{0x11,         0x51},         {0x13,        0x53},         {0x17,         0x57},       {0xff,         0xff}},  // TYPE_3
        {{0x31,         0x71},         {0x33,        0x73},         {0x37,         0x77},       {0xff,         0xff}},  // TYPE_4
        {{0x00,         0x40},         {0x10,        0x50},         {0x30,         0x70},       {0xff,         0xff}},  // TYPE_5
        {{0x01,         0x41},         {0x03,        0x43},         {0x07,         0x47},       {0xff,         0xff}},  // TYPE_6
        {{0x11,         0x51},         {0x13,        0x53},         {0x17,         0x57},       {0xff,         0xff}},  // TYPE_7
        {{0x31,         0x71},         {0x33,        0x73},         {0x37,         0x77},       {0xff,         0xff}}   // TYPE_8
    };

    uint64_t l_data;
    mss_memconfig::MemOrg l_row;
    mss_memconfig::MemOrg l_col;
    mss_memconfig::MemOrg l_bank;
    uint8_t l_dramWidth = 0;
    uint8_t l_mbaPosition = 0;
    uint8_t l_dram_gen = 0;

    const auto l_targetCentaur = i_target.getParent<fapi2::TARGET_TYPE_MEMBUF_CHIP>();
    FAPI_ATTR_GET(fapi2::ATTR_CHIP_UNIT_POS, i_target,  l_mbaPosition);
    FAPI_ATTR_GET(fapi2::ATTR_CEN_EFF_DRAM_WIDTH, i_target,  l_dramWidth);
    FAPI_ATTR_GET(fapi2::ATTR_CEN_EFF_DRAM_GEN, i_target,  l_dram_gen);
    fapi2::getScom(l_targetCentaur, mss_mbaxcr[l_mbaPosition], l_data);

    uint32_t l_dramSize = (uint32_t)((l_data & 0x300000000000000) >> 24);

    if((l_dramWidth == mss_memconfig::X8) &&
       (l_dramSize == mss_memconfig::GBIT_2) &&
       (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR3))
    {
        l_row =   mss_memconfig::ROW_15;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_3;
    } else if((l_dramWidth == mss_memconfig::X8) &&
              (l_dramSize == mss_memconfig::GBIT_2) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   mss_memconfig::ROW_14;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    } else if((l_dramWidth == mss_memconfig::X4) &&
              (l_dramSize == mss_memconfig::GBIT_2) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR3))
    {
        l_row =   mss_memconfig::ROW_15;
        l_col =   mss_memconfig::COL_11;
        l_bank =  mss_memconfig::BANK_3;
    } else if((l_dramWidth == mss_memconfig::X4) &&
              (l_dramSize == mss_memconfig::GBIT_2) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   mss_memconfig::ROW_15;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    } else if((l_dramWidth == mss_memconfig::X8) &&
              (l_dramSize == mss_memconfig::GBIT_4) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR3))
    {
        l_row =   mss_memconfig::ROW_16;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_3;
    } else if((l_dramWidth == mss_memconfig::X8) &&
              (l_dramSize == mss_memconfig::GBIT_4) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   mss_memconfig::ROW_15;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    } else if((l_dramWidth == mss_memconfig::X4) &&
              (l_dramSize == mss_memconfig::GBIT_4) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR3))
    {
        l_row =   mss_memconfig::ROW_16;
        l_col =   mss_memconfig::COL_11;
        l_bank =  mss_memconfig::BANK_3;
    } else if((l_dramWidth == mss_memconfig::X4) &&
              (l_dramSize == mss_memconfig::GBIT_4) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   mss_memconfig::ROW_16;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    } else if((l_dramWidth == mss_memconfig::X8) &&
              (l_dramSize == mss_memconfig::GBIT_8) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR3))
    {
        l_row =   mss_memconfig::ROW_16;
        l_col =   mss_memconfig::COL_11;
        l_bank =  mss_memconfig::BANK_3;
    } else if((l_dramWidth == mss_memconfig::X8) &&
              (l_dramSize == mss_memconfig::GBIT_8) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   mss_memconfig::ROW_16;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    } else if((l_dramWidth == mss_memconfig::X4) &&
              (l_dramSize == mss_memconfig::GBIT_8) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR3))
    {
        l_row =   mss_memconfig::ROW_16;
        l_col =   mss_memconfig::COL_12;
        l_bank =  mss_memconfig::BANK_3;
    } else if((l_dramWidth == mss_memconfig::X4) &&
              (l_dramSize == mss_memconfig::GBIT_8) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   mss_memconfig::ROW_17;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    } else if((l_dramWidth == mss_memconfig::X4) &&
              (l_dramSize == mss_memconfig::GBIT_16) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   ss_memconfig::ROW_17;
        l_col =   ss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    } else if((l_dramWidth == mss_memconfig::X8) &&
              (l_dramSize == mss_memconfig::GBIT_16) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   mss_memconfig::ROW_17;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    }

    uint8_t l_configType = (l_data & 0xF0) >> 4;
    uint8_t l_configSubType = (l_data & 0xC) >> 2;
    uint8_t l_slotConfig = l_data & 0x1;

    uint8_t l_end_master_rank = (memConfigType[l_configType][l_configSubType][l_slotConfig] & 0xf0) >> 4;
    uint8_t l_end_slave_rank = memConfigType[l_configType][l_configSubType][l_slotConfig] & 0x0f;

    o_end_addr =
        (l_end_master_rank & 0xF) << 60
      | (l_end_slave_rank & 0x7) << 57
      | ((uint32_t)l_bank & 0xF) << 53
      | ((uint32_t)l_row & 0x1FFFF) << 36
      | ((uint32_t)l_col & 0xFFF) << 24;

    if(l_dramWidth == mss_memconfig::X4
    && l_dramSize  == mss_memconfig::GBIT_16
    && l_dram_gen  == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4)
    {
        o_end_addr.writeBit<CEN_MBA_MBMEAQ_CMD_ROW17>(l_row18);
    }
}

class operation
{
    inline fapi2::ReturnCode execute(const fapi2::Target<T>& i_target)
    {
        return mss::mcbist::execute(i_target, iv_program);
    }
}

template< mss::mc_type MC = DEFAULT_MC_TYPE, fapi2::TargetType T, typename TT = mcbistTraits<MC, T>, typename ET = mcbistMCTraits<MC> >
fapi2::ReturnCode execute( const fapi2::Target<T>& i_target, const program<MC>& i_program )
{
    fapi2::buffer<uint64_t> l_status;
    bool l_poll_result = false;
    poll_parameters l_poll_parameters;

    // Init the fapi2 return code
    fapi2::current_err = fapi2::FAPI2_RC_SUCCESS;

    clear_error_helper<MC>(i_target, const_cast<program<MC>&>(i_program));
    load_fifo_mode<MC>(i_target, i_program);
    load_addr_gen<MC>(i_target, i_program);
    load_mcbparm<MC>(i_target, i_program);
    load_mcbamr(i_target, i_program);
    load_config<MC>(i_target, i_program);
    load_control<MC>(i_target, i_program);
    load_data_config<MC>(i_target, i_program);
    load_thresholds<MC>(i_target, i_program);
    load_mcbmr<MC>(i_target, i_program);
    start_stop<MC>(i_target, mss::START);
    l_poll_result = mss::poll(i_target, TT::STATQ_REG, l_poll_parameters,
                              [&l_status](const size_t poll_remaining, const fapi2::buffer<uint64_t>& stat_reg) -> bool
    {
        l_status = stat_reg;
        return (l_status.getBit<TT::MCBIST_IN_PROGRESS>() == true) || (l_status.getBit<TT::MCBIST_DONE>() == true);
    });

    if (!i_program.iv_async)
    {
        return mcbist::poll(i_target, i_program);
    }
}

template<mss::mc_type MC = DEFAULT_MC_TYPE, fapi2::TargetType T>
fapi2::ReturnCode sf_init(const fapi2::Target<T>& i_target)
{
    using ET = mss::mcbistMCTraits<MC>;
    constraints<MC> l_const(PATTERN_0);
    sf_init_operation<MC> l_init_op(l_const, l_rc);
    return l_init_op.execute(i_target);
}

template<>
uint32_t mssRestoreDramRepairs<TYPE_MCA>( TargetHandle_t i_target,
                                          uint8_t & o_repairedRankMask,
                                          uint8_t & o_badDimmMask )
{
    errlHndl_t errl = NULL;
    fapi2::buffer<uint8_t> tmpRepairedRankMask, tmpBadDimmMask;
    restore_repairs(
        fapi2::Target<fapi2::TARGET_TYPE_MCA>(i_target),
        tmpRepairedRankMask, tmpBadDimmMask);

    o_repairedRankMask = (uint8_t)tmpRepairedRankMask;
    o_badDimmMask = (uint8_t)tmpBadDimmMask;
}

inline void eff_dram_width(const fapi2::Target<fapi2::TARGET_TYPE_DIMM>& i_target, uint8_t& l_width)
{
    uint8_t l_value[2][2];
    auto l_mca = i_target.getParent<fapi2::TARGET_TYPE_MCA>();
    auto l_mcs = l_mca.getParent<fapi2::TARGET_TYPE_MCS>();

    FAPI_ATTR_GET(fapi2::ATTR_EFF_DRAM_WIDTH, l_mcs, l_value);
    l_width = l_value[mss::index(l_mca)][mss::index(i_target)];
}

void reset_dqs_disable(
    const fapi2::Target<fapi2::TARGET_TYPE_DIMM>& i_target,
    const fapi2::buffer<uint64_t>& i_dq_disable,
    const uint64_t i_reg)
{
    // Declares variables
    fapi2::buffer<uint64_t> l_dqs_disable;
    const auto& l_mca = mss::find_target<fapi2::TARGET_TYPE_MCA>(i_target);

    uint8_t l_value[2][2];
    auto l_mcs = i_target.getParent<fapi2::TARGET_TYPE_MCA>().getParent<fapi2::TARGET_TYPE_MCS>();

    FAPI_ATTR_GET(fapi2::ATTR_EFF_DRAM_WIDTH, l_mcs, l_value);
    uint8_t l_width = l_value[mss::index(l_mca)][mss::index(i_target)];

    // Checks if this DIMM is a x8 DIMM first, if so, skip it as a chip kill here is beyond our corrective capabilities
    if(l_width == fapi2::ENUM_ATTR_EFF_DRAM_WIDTH_X8)
    {
        mss::putScom(l_mca, i_reg, l_dqs_disable);
        return;
    }
    // Due to plug rules check, we should be a x4 now, so let's reset some DQS bits
    for(uint64_t l_nibble = 0; l_nibble < NIBBLES_PER_DP; ++l_nibble)
    {
        // If we have a whole nibble bad, set the bad DQS bits
        const auto DQ_START = 48 + (l_nibble * 4);
        const auto DQS_START = 48 + (l_nibble * 2);
        uint64_t l_nibble_disable;

        i_dq_disable.extractToRight(l_nibble_disable, DQ_START, 4);
        if(l_nibble_disable == 0xF)
        {
            l_dqs_disable.setBit(DQS_START, 2);
        }
    }
    // Sets up the DQS register
    mss::putScom(l_mca, i_reg, l_dqs_disable);
}

inline std::vector<uint64_t> bad_dq_attr_to_vector(const uint8_t i_bad_bits, const size_t i_nibble /*can only be 0 or 1*/)
{
    std::vector<uint64_t> l_output;
    const fapi2::buffer<uint8_t> l_bit_buffer(i_bad_bits);
    const size_t l_start = (i_nibble == 0) ? 0 : 4;

    for (size_t l_offset = 0; l_offset < 4; ++l_offset)
    {
        const size_t l_position_tmp = l_start + l_offset;
        if (l_bit_buffer.getBit(l_position_tmp))
        {
            l_output.push_back(l_position_tmp);
        }
    }
    return l_output;
}

template<>
inline fapi2::ReturnCode dimm_spare< mss::mc_type::NIMBUS >(
    const fapi2::Target<fapi2::TARGET_TYPE_DIMM>& i_target,
    uint8_t (&o_array)[mss::MAX_RANK_PER_DIMM_ATTR])
{
    uint8_t l_value[mss::MAX_RANK_PER_DIMM_ATTR] = {fapi2::ENUM_ATTR_MEM_EFF_DIMM_SPARE_NO_SPARE};
    memcpy(o_array, &l_value, mss::MAX_RANK_PER_DIMM_ATTR);
}

template< mss::mc_type MC = mss::mc_type::NIMBUS, fapi2::TargetType T>
void restore_repairs_helper( const fapi2::Target<T>& i_target,
        const uint8_t i_bad_bits[4][10],
        fapi2::buffer<uint8_t>& io_repairs_applied,
        fapi2::buffer<uint8_t>& io_repairs_exceeded)
{
    std::vector<uint64_t> l_ranks;
    const auto l_dimm_idx = index(i_target);
    uint8_t l_dimm_spare[MAX_RANK_PER_DIMM_ATTR] = {0};

    // gets spare availability
    mss::dimm_spare<mss::mc_type::NIMBUS>(i_target, l_dimm_spare);
    // gets all of the ranks to loop over
    mss::rank::ranks_on_dimm_helper<mss::mc_type::NIMBUS>(i_target, l_ranks);

    // loop through ranks
    for (const auto l_rank : l_ranks)
    {
        const auto l_rank_idx = index(l_rank);
        // don't process last byte if it's a non-existent spare. Non-existent spare bits are marked 'bad' in the attribute
        const auto l_total_bytes = get_total_bytes<mss::mc_type::NIMBUS>(l_dimm_spare[l_rank]);
        repair_state_machine<mss::mc_type::NIMBUS, fapi2::TARGET_TYPE_DIMM> l_machine;
        for (uint64_t l_byte = 0; l_byte < l_total_bytes; ++l_byte)
        {
            for (size_t l_nibble = 0; l_nibble < 2; ++l_nibble)
            {
                // don't process non-existent spare nibble because all non-existent spare bits are marked 'bad' in the attribute
                if (skip_dne_spare_nibble<mss::mc_type::NIMBUS>(l_dimm_spare[l_rank], l_byte, l_nibble))
                {
                    continue;
                }

                const auto l_bad_dq_vector = mss::bad_dq_attr_to_vector(i_bad_bits[l_rank_idx][l_byte], l_nibble);

                // apply repairs and update repair machine state
                // if there are no bad bits (l_bad_dq_vector.size() == 0) no action is necessary
                if (l_bad_dq_vector.size() == 1)
                {
                    // l_bad_dq_vector is per byte, so multiply up to get the bad dq's index
                    const uint64_t l_dq = l_bad_dq_vector[0] + (l_byte * BITS_PER_BYTE);
                    l_machine.one_bad_dq(i_target, l_rank, l_dq, io_repairs_applied, io_repairs_exceeded);
                }
                else if (l_bad_dq_vector.size() > 1)
                {
                    // l_bad_dq_vector is per byte, so multiply up to get the bad dq's index
                    const uint64_t l_dq = l_bad_dq_vector[0] + (l_byte * BITS_PER_BYTE);
                    l_machine.multiple_bad_dq(i_target, l_rank, l_dq, io_repairs_applied, io_repairs_exceeded);
                }

                // if repairs have been exceeded, we're done
                if (io_repairs_exceeded.getBit(l_dimm_idx))
                {
                    return;
                }
            }
        }
    }
}


fapi2::ReturnCode reset_bad_bits_helper( const fapi2::Target<fapi2::TARGET_TYPE_DIMM>& i_target,
        const uint8_t (&i_bad_dq)[BAD_BITS_RANKS][BAD_DQ_BYTE_COUNT])
{
    std::vector<uint64_t> l_ranks;

    // Loop over the ranks, makes things simpler than looping over the DIMM
    rank::ranks(i_target, l_ranks);

    for (const auto& r : l_ranks)
    {
        uint64_t l_rp = 0;
        const uint64_t l_dimm_index = rank::get_dimm_from_rank(r);
        mss::rank::get_pair_from_rank(mss::find_target<fapi2::TARGET_TYPE_MCA>(i_target), r, l_rp);

        // This is the set of registers for this rank pair. It is indexed by DP. We know the bad bits
        // [0] and [1] are the 16 bits for DP0, [2],[3] are the 16 for DP1, etc.
        const auto& l_addrs = dp16Traits<TARGET_TYPE_MCA>::BIT_DISABLE_REG[l_rp];

        // This is the section of the attribute we need to use. The result is an array of 10 bytes.
        const uint8_t* l_bad_bits = &(i_bad_dq[mss::index(r)][0]);
        size_t l_byte_index = 0;
        for(const auto& addr : l_addrs)
        {
            const uint64_t l_register_value = (l_bad_bits[l_byte_index] << 8) | l_bad_bits[l_byte_index + 1];
            mss::putScom(mss::find_target<fapi2::TARGET_TYPE_MCA>(i_target), addr.first, l_register_value);
            reset_dqs_disable(i_target, l_register_value, addr.second);
            l_byte_index += 2;
        }
    }
}

template< mss::mc_type MC = mss::mc_type::NIMBUS, fapi2::TargetType fapi2::TARGET_TYPE_MCA>
fapi2::ReturnCode restore_repairs( const fapi2::Target<fapi2::TARGET_TYPE_MCA>& i_target,
                                   fapi2::buffer<uint8_t>& o_repairs_applied,
                                   fapi2::buffer<uint8_t>& o_repairs_exceeded)
{
    uint8_t l_bad_bits[4][10] = {};
    o_repairs_applied = 0;
    o_repairs_exceeded = 0;

    for (const auto& l_dimm : mss::find_targets<fapi2::TARGET_TYPE_DIMM>(i_target))
    {
        FAPI_ATTR_GET(fapi2::ATTR_BAD_DQ_BITMAP, i_target, l_bad_bits);
        restore_repairs_helper<mss::mc_type::NIMBUS, fapi2::TARGET_TYPE_DIMM>(
                       l_dimm, l_bad_bits, o_repairs_applied, o_repairs_exceeded);
    }

fapi_try_exit:
    return fapi2::current_err;
}

template<TARGETING::TYPE T /* TYPE_MCA */>
uint32_t restoreDramRepairs(TargetHandle_t i_trgt)
{
    bool calloutMade = false;
    // Will need the chip and system objects initialized for several parts
    // of this function and sub-functions.
    if(false == g_initialized || nullptr == systemPtr)
    {
        noLock_initialize();
    }

    std::vector<MemRank> ranks;
    getMasterRanks<TYPE_MCA>(i_trgt, ranks);

    uint8_t rankMask = 0, dimmMask = 0;
    mssRestoreDramRepairs<TYPE_MCA>(i_trgt, rankMask, dimmMask);

    // Callout DIMMs with too many bad bits and not enough repairs available
    if(RDR::processBadDimms<TYPE_MCA>(i_trgt, dimmMask))
        calloutMade = true;

    // Check repaired ranks for RAS policy violations.
    if(RDR::processRepairedRanks<TYPE_MCA>(i_trgt, rankMask))
        calloutMade = true;

    return calloutMade ? FAIL : SUCCESS;
}

void Util::__Util_ThreadPool_Impl::ThreadPoolImpl::insert(void* i_workItem)
{
    iv_worklist.push_back(i_workItem);
    sync_cond_signal(&iv_condvar);
}

void sync_cond_signal(sync_cond_t * i_cond)
{
    __sync_fetch_and_add(&(i_cond->sequence),1);
    futex_wake(&(i_cond->sequence), 1);
}

ALWAYS_INLINE inline TARGETING::TargetHandle_t getTarget(WorkFlowProperties & i_wfp)
{
    return i_wfp.assoc->first;
}

bool StateMachine::executeWorkItem(WorkFlowProperties * i_wfp)
{
    switch(*i_wfp->workItem)
    {
        case RESTORE_DRAM_REPAIRS:
            // Get the connected MCAs.
            TargetHandleList mcaList;
            getChildAffinityTargets(mcaList, getTarget(*i_wfp), CLASS_UNIT, TYPE_MCA);
            for (auto & mca : mcaList)
            {
                PRDF::restoreDramRepairs<TYPE_MCA>(mca);
            }
            break;
        case START_PATTERN_0:
        case START_SCRUB:
            doMaintCommand(*i_wfp);
            break;
    }
    ++i_wfp->workItem;
    scheduleWorkItem(*i_wfp);
}

void StateMachine::scheduleWorkItem(WorkFlowProperties & i_wfp)
{
    if(i_wfp.workItem == getWorkFlow(i_wfp).end())
    {
        i_wfp.status = COMPLETE;
    }
    else if(i_wfp.status != IN_PROGRESS && allWorkFlowsComplete())
    {
        // Clear BAD_DQ_BIT_SET bit
        TargetHandle_t top = NULL;
        targetService().getTopLevelTarget(top);
        ATTR_RECONFIGURE_LOOP_type reconfigAttr = top->getAttr<TARGETING::ATTR_RECONFIGURE_LOOP>();
        reconfigAttr &= ~RECONFIGURE_LOOP_BAD_DQ_BIT_SET;
        top->setAttr<TARGETING::ATTR_RECONFIGURE_LOOP>(reconfigAttr);
        iv_done = true;
        sync_cond_broadcast(&iv_cond);
    }
    else if(i_wfp.status == IN_PROGRESS)
    {
        uint64_t priority = getRemainingWorkItems(i_wfp);
        if(!iv_tp)
        {
            // create same number of tasks in the pool as there are cpu threads
            Util::ThreadPoolManager::setThreadCount(cpu_thread_count());
            iv_tp = new Util::ThreadPool<WorkItem>();
            iv_tp->start();
        }
        iv_tp->insert(new WorkItem(*this, &i_wfp, priority, i_wfp.chipUnit));
    }
}

void StateMachine::start()
{
    // schedule the first work items for all target / workFlow associations
    for(WorkFlowPropertiesIterator wit = iv_workFlowProperties.begin();
        wit != iv_workFlowProperties.end(); ++wit)
    {
        // bool StateMachine::executeWorkItem(WorkFlowProperties * i_wfp)
        // this is probably later called on it
        scheduleWorkItem(**wit);
    }
}

void StateMachine::reset()
{
    for(WorkFlowPropertiesIterator wit = iv_workFlowProperties.begin();
        wit != iv_workFlowProperties.end(); ++wit)
    {
        if((**wit).log)
        {
            delete (**wit).log;
        }
        delete *wit;
    }
    iv_workFlowProperties.clear();
}

void StateMachine::setup(const WorkFlowAssocMap & i_list /* target type is TYPE_MCBIST */ )
{
    // clear out any properties from a previous run
    reset();
    WorkFlowProperties * wfp = 0;
    for(WorkFlowAssoc it = i_list.begin(); it != i_list.end(); ++it)
    {
        // for each target / workFlow assoc,
        // initialize the workFlow progress indicator
        // to indicate that no work has been done yet
        // for the target
        wfp = new WorkFlowProperties();
        wfp->assoc = it;
        wfp->workItem = getWorkFlow(it).begin();
        wfp->status = IN_PROGRESS;
        wfp->log = 0;
        wfp->timer = 0;
        wfp->timeoutCnt = 0;
        wfp->data = NULL;
        wfp->chipUnit = it->first->getAttr<ATTR_CHIP_UNIT>();
        iv_workFlowProperties.push_back(wfp);
    }
    iv_done = false;
}

void StateMachine::wait()
{
    while(!iv_done && !iv_shutdown)
    {
        sync_cond_wait(&iv_cond, &iv_mutex);
    }
}

void StateMachine::run(const WorkFlowAssocMap & i_list /* target type is TYPE_MCBIST*/)
{
    // load the workflow properties
    setup(i_list);
    // start work items
    start();
    // wait for all work items to finish
    wait();
}

void sync_cond_wait(sync_cond_t * i_cond, mutex_t * i_mutex)
{
    uint64_t seq = i_cond->sequence;
    if(i_cond->mutex != i_mutex)
    {
        if(i_cond->mutex) return;

        // Atomically set mutex
        __sync_bool_compare_and_swap(&i_cond->mutex, NULL, i_mutex);
        if(i_cond->mutex != i_mutex) return;
    }
    mutex_unlock(i_mutex);
    futex_wait( &(i_cond->sequence), seq);
    while(0 != __sync_lock_test_and_set(&(i_mutex->iv_val), 2))
    {
        futex_wait(&(i_mutex->iv_val),2);
    }
}

void memDiag(TargetHandleList i_targetList /*TYPE_MCBIST*/) {
    // get the workflow for each target mba passed in.
    // associate each workflow with the target handle.
    WorkFlowAssocMap list;
    TargetHandleList::const_iterator tit;
    for(tit = i_targetList.begin(); tit != i_targetList.end(); ++tit)
    {
        list[*tit].push_back(RESTORE_DRAM_REPAIRS);
        list[*tit].push_back(START_PATTERN_0);
        list[*tit].push_back(START_SCRUB);
    }
    Singleton<StateMachine>::instance().run(list);

    // If this step completes without the need for a reconfig due to an RCD
    // parity error, clear all RCD parity error counters.
    // if (!(top->getAttr<ATTR_RECONFIGURE_LOOP>() & RECONFIGURE_LOOP_RCD_PARITY_ERROR))
    // {
    //     TargetHandleList trgtList; getAllChiplets(trgtList, TYPE_MCA);
    //     for (auto & trgt : trgtList)
    //     {
    //         if (0 != trgt->getAttr<ATTR_RCD_PARITY_RECONFIG_LOOP_COUNT>())
    //             trgt->setAttr<ATTR_RCD_PARITY_RECONFIG_LOOP_COUNT>(0);
    //     }
    // }
}

void after_memdiags(uint64_t i_target)
{
    uint64_t dsm0_buffer;
    uint64_t rd_tag_delay;
    uint64_t wr_done_delay;
    uint64_t rcd_protect_time;

    scom_and_for_chiplet(
        i_target, MCBIST_MCBISTFIRACT1,
        ~MCBIST_MCBISTFIRQ_MCBIST_BRODCAST_OUT_OF_SYNC);

    for(unsigned int port = 0; port < 4; ++port)
    {
        scom_or(ECC_REG | port << PORT_OFFSET, RECR_ENABLE_UE_NOISE_WINDOW);

        dsm0_buffer = scom_read(DSM0Q_REG | (port << PORT_OFFSET));
        wr_done_delay = (dsm0_buffer & 0xFC00000000) >> 34;
        rd_tag_delay = (dsm0_buffer & 0xFC00000) >> 22;
        rcd_protect_time = min(wr_done_delay, rd_tag_delay);
        scom_and_or(MBA_FARB0Q | (port << PORT_OFFSET), ~0xFC00, (rcd_protect_time & 0x3F) << 10);

        scom_and(
            FIR_ACTION0 | (port << PORT_OFFSET),
            ~(MCA_FIR_MAINLINE_AUE | MCA_FIR_MAINLINE_UE
           | MCA_FIR_MAINLINE_IAUE | MCA_FIR_MAINLINE_IUE
           | MCA_FIR_MAINTENANCE_IUE));

        scom_and_or(
            FIR_ACTION1 | (port << PORT_OFFSET),
            ~(MCA_FIR_MAINLINE_AUE | MCA_FIR_MAINLINE_IAUE),
              MCA_FIR_MAINLINE_UE  | MCA_FIR_MAINLINE_IUE
            | MCA_FIR_MAINTENANCE_IUE);

        scom_and(
            FIR_MASK | (port << PORT_OFFSET),
            ~(MCA_FIR_MAINLINE_AUE | MCA_FIR_MAINLINE_UE
           | MCA_FIR_MAINLINE_IAUE | MCA_FIR_MAINLINE_IUE
           | MCA_FIR_MAINTENANCE_IUE));

        scom_and(
            MBACALFIR_ACTION0 | (port << PORT_OFFSET),
            ~(MCA_MBACALFIRQ_PORT_FAIL));
        scom_or(
            MBACALFIR_ACTION1 | (port << PORT_OFFSET),
            MCA_MBACALFIRQ_PORT_FAIL);
        scom_and(
            MBACALFIR_MASK | (port << PORT_OFFSET),
            ~(MCA_MBACALFIRQ_PORT_FAIL));

        if(CHIP_EC == 0x20)
        {
            scom_and(FIR_ACTION0 | (port << PORT_OFFSET), ~MCA_FIR_MAINLINE_RCD);
            scom_and(FIR_ACTION1 | (port << PORT_OFFSET), ~(MCA_FIR_MAINLINE_UE | MCA_FIR_MAINLINE_RCD));
            scom_and(FIR_MASK | (port << PORT_OFFSET), ~MCA_FIR_MAINLINE_RCD);
            scom_and(MBACALFIR_ACTION1 | (port << PORT_OFFSET), ~(MCA_MBACALFIRQ_PORT_FAIL));
        }

        scom_and(MCA_ACTION1 | (port << PORT_OFFSET), ~0x48000000);
        scom_and(MBA_FARB0Q | (port << PORT_OFFSET), ~0x240);
    }
}

void istep_14_1(void)
{
    printk(BIOS_EMERG, "starting istep 14.1\n");
    report_istep(14, 1);
    TargetHandleList trgtList;
    getAllChiplets(trgtList, TYPE_MCBIST);
    memDiag(trgtList);
    for(unsigned int targetIndex = 0; targetIndex < memdiagTargetsSize; ++targetIndex) {
        after_memdiags(memdiagTargets[targetIndex]);
        for(unsigned int port = 0; port < 4; ++port) {
            scom_or(RRQ0Q_REG | (port << PORT_OFFSET), MBA_RRQ0Q_CFG_RRQ_FIFO_MODE);
            scom_or(WRQ0Q_REG | (port << PORT_OFFSET), MBA_WRQ0Q_CFG_WRQ_FIFO_MODE);
        }
    }
    printk(BIOS_EMERG, "ending istep 14.1\n");
    return;
}
