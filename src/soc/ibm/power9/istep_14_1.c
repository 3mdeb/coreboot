/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/istep_14_1.h>
#include <cpu/power/scom.h>

errlHndl_t StateMachine::doMaintCommand(WorkFlowProperties & i_wfp)
{
    uint64_t workItem;
    TargetHandle_t target;

    // starting a maint cmd ...  register a timeout monitor
    uint64_t maintCmdTO = getTimeoutValue();

    i_wfp.timeoutCnt = 0; // reset for new work item
    workItem = *i_wfp.workItem;

    target = getTarget(i_wfp);

    TYPE trgtType = target->getAttr<ATTR_TYPE>();
    // new command...use the full range
    if (TYPE_MBA == trgtType)
    {
        uint32_t stopCondition =
            mss_MaintCmd::STOP_END_OF_RANK
          | mss_MaintCmd::STOP_ON_MPE
          | mss_MaintCmd::STOP_ON_UE
          | mss_MaintCmd::STOP_ON_END_ADDRESS
          | mss_MaintCmd::ENABLE_CMD_COMPLETE_ATTENTION;

        fapi2::buffer<uint64_t> startAddr, endAddr;
        mss_MaintCmd * cmd = static_cast<mss_MaintCmd *>(i_wfp.data);
        fapi2::Target<fapi2::TARGET_TYPE_MBA> fapiMba(target);

        // We will always do ce setup though CE calculation
        // is only done during MNFG. This will give use better ffdc.
        ceErrorSetup<TYPE_MBA>(target);

        mss_get_address_range(fapiMba, endAddr);
        if(workItem == START_SCRUB)
        {
            cmd = new mss_SuperFastRead(
                fapiMba,
                0,
                endAddr,
                stopCondition,
                false);
        } else // PATTERNS
        {
            cmd = new mss_SuperFastInit(
                fapiMba,
                0,
                endAddr,
                static_cast<mss_MaintCmd::PatternIndex>(workItem),
                stopCondition,
                false);
        }
        i_wfp.data = cmd;
        cmd->setupAndExecuteCmd();
    }
    else if(TYPE_MCBIST == trgtType)
    {
        fapi2::Target<fapi2::TARGET_TYPE_MCBIST> fapiMcbist(target);
        mss::mcbist::stop_conditions<mss::mc_type::NIMBUS> stopCond;
        switch(workItem)
        {
            case START_SCRUB:
                //set stop conditions
                stopCond.set_pause_on_mpe(mss::ON);
                stopCond.set_pause_on_ue(mss::ON);
                stopCond.set_pause_on_aue(mss::ON);
                stopCond.set_nce_inter_symbol_count_enable(mss::ON);
                stopCond.set_nce_soft_symbol_count_enable(mss::ON);
                stopCond.set_nce_hard_symbol_count_enable(mss::ON);
                nim_sf_read(fapiMcbist, stopCond);
                break;
            case START_PATTERN_0:
                sf_init(fapiMcbist, workItem);
                break;
        }
    }
}

fapi2::ReturnCode nim_sf_init( const fapi2::Target<fapi2::TARGET_TYPE_MCBIST>& i_target,
                               const uint64_t i_pattern )
{
    return mss::memdiags::sf_init<mss::mc_type::NIMBUS>(i_target, i_pattern);
}

fapi2::ReturnCode nim_sf_read(const fapi2::Target<fapi2::TARGET_TYPE_MCBIST>& i_target,
                              const mss::mcbist::stop_conditions<mss::mc_type::NIMBUS>& i_stop,
                              const mss::mcbist::address& i_address,
                              const mss::mcbist::end_boundary i_end,
                              const mss::mcbist::address& i_end_address)
{
    return mss::memdiags::sf_read<mss::mc_type::NIMBUS>(i_target, i_stop, i_address, i_end, i_end_address);
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


template< mss::mc_type MC, fapi2::TargetType T = mss::mcbistMCTraits<MC>::MC_TARGET_TYPE , typename TT = mcbistTraits<MC, T> >
fapi2::ReturnCode sf_read(const fapi2::Target<T>& i_target,
                          const stop_conditions<MC>& i_stop,
                          const mss::mcbist::address& i_address = mss::mcbist::address(),
                          const end_boundary i_end = end_boundary::STOP_AFTER_SLAVE_RANK,
                          const mss::mcbist::address& i_end_address = mss::mcbist::address(TT::LARGEST_ADDRESS))
{
    using ET = mss::mcbistMCTraits<MC>;
    fapi2::ReturnCode l_rc;
    constraints<MC> l_const(i_stop, speed::LUDICROUS, i_end, i_address, i_end_address);
    sf_read_operation<MC> l_read_op(i_target, l_const, l_rc);
    return l_read_op.execute();
}

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

template<  mss::mc_type MC = DEFAULT_MC_TYPE, fapi2::TargetType T >
fapi2::ReturnCode sf_init(const fapi2::Target<T>& i_target,
                          const uint64_t i_pattern = PATTERN_0 )
{
    using ET = mss::mcbistMCTraits<MC>;
    constraints<MC> l_const(i_pattern);
    sf_init_operation<MC> l_init_op(l_const, l_rc);
    return l_init_op.execute(i_target);
}

uint8_t getDimmPort( TARGETING::TargetHandle_t i_dimmTrgt )
{

}

//------------------------------------------------------------------------------

uint8_t getDimmSlct( TargetHandle_t i_trgt )
{
    return dimm->getAttr<ATTR_POS_ON_MEM_PORT>();
}

template<>
bool processBadDimms<TYPE_MBA>( TargetHandle_t i_trgt, uint8_t i_badDimmMask )
{
    // The bits in i_badDimmMask represent DIMMs that have exceeded the
    // available repairs. Callout these DIMMs.

    bool o_calloutMade  = false;
    // Iterate the list of all DIMMs be
    TargetHandleList dimms = getConnected(i_trgt, TYPE_DIMM);
    for (auto & dimm : dimms)
    {
        uint8_t portSlct = dimm->getAttr<ATTR_MEM_PORT>();
        uint8_t dimmSlct = dimm->getAttr<ATTR_POS_ON_MEM_PORT>();

        // The 4 bits of i_badDimmMask is defined as p0d0, p0d1, p1d0, and p1d1.
        uint8_t mask = 0x8 >> (portSlct * 2 + dimmSlct);

        if(i_badDimmMask & mask)
        {
            __calloutDimm<TYPE_MBA>( errl, i_trgt, dimm );
            o_calloutMade = true;
        }
    }

    return o_calloutMade;
}

template<TARGETING::TYPE T>
uint32_t restoreDramRepairs( TargetHandle_t i_trgt )
{
    bool calloutMade = false;
    // Will need the chip and system objects initialized for several parts
    // of this function and sub-functions.
    if(false == g_initialized || nullptr == systemPtr)
    {
        noLock_initialize();
    }

    std::vector<MemRank> ranks;
    getMasterRanks<T>(i_trgt, ranks);

    uint8_t rankMask = 0, dimmMask = 0;
    if(SUCCESS != mssRestoreDramRepairs<T>(i_trgt, rankMask, dimmMask))
    {
        return FAIL;
    }
    // Callout DIMMs with too many bad bits and not enough repairs available
    if(RDR::processBadDimms<T>(i_trgt, dimmMask))
        calloutMade = true;

    // Check repaired ranks for RAS policy violations.
    if(RDR::processRepairedRanks<T>(i_trgt, rankMask))
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

bool StateMachine::scheduleWorkItem(WorkFlowProperties & i_wfp)
{
    if(i_wfp.workItem == getWorkFlow(i_wfp).end())
    {
        i_wfp.status = COMPLETE;
    }

    if(i_wfp.status != IN_PROGRESS && allWorkFlowsComplete())
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
            //create same number of tasks in the pool as there are cpu threads
            const size_t l_num_tasks = cpu_thread_count();
            Util::ThreadPoolManager::setThreadCount(l_num_tasks);
            iv_tp = new Util::ThreadPool<WorkItem>();
            iv_tp->start();
        }

        TargetHandle_t target = getTarget(i_wfp);
        iv_tp->insert(new WorkItem(*this, &i_wfp, priority, i_wfp.chipUnit));
        return true;
    }
    return false;
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
        iv_workFlowProperties.push_back(p);
    }

    if(iv_workFlowProperties.empty())
    {
        iv_done = true;
    }
    else
    {
        iv_done = false;
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

void StateMachine::wait()
{
    while(!iv_done && !iv_shutdown)
    {
        sync_cond_wait(&iv_cond, &iv_mutex);
    }
}

void getWorkFlow(
    DiagMode i_mode,
    WorkFlow & o_wf,
    const Globals & i_globals)
{
    o_wf.push_back(RESTORE_DRAM_REPAIRS);
    o_wf.push_back(START_PATTERN_0);
    o_wf.push_back(START_SCRUB);
}

void memDiag(TargetHandleList i_targetList /*TYPE_MCBIST*/) {

    // memory diagnostics ipl step entry point
    Globals globals;
    TargetHandle_t top = nullptr;
    targetService().getTopLevelTarget(top);

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
