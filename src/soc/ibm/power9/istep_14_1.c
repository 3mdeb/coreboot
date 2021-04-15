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

template<mss::mc_type MC = mss::mc_type::NIMBUS, fapi2::TargetType T = fapi2::TARGET_TYPE_MCBIST>
fapi2::ReturnCode sf_init(const fapi2::Target<fapi2::TARGET_TYPE_MCBIST>& i_target)
{
    using ET = mss::mcbistMCTraits<mss::mc_type::NIMBUS>;

    constraints<mss::mc_type::NIMBUS> l_const(PATTERN_0);
    sf_init_operation<mss::mc_type::NIMBUS> l_init_op(l_const, l_rc);
    return l_init_op.execute(i_target);
}

template< mss::mc_type MC = mss::mc_type::NIMBUS, fapi2::TargetType T = mss::mcbistMCTraits<MC>::MC_TARGET_TYPE , typename TT = mcbistTraits<MC, T> >
fapi2::ReturnCode sf_read(
    const fapi2::Target<T>& i_target,
    const stop_conditions<MC>& i_stop)
{
    using ET = mss::mcbistMCTraits<MC>;
    fapi2::ReturnCode l_rc;
    constraints<mss::mc_type::NIMBUS> l_constraints(
        i_stop,
        speed::LUDICROUS,
        end_boundary::STOP_AFTER_SLAVE_RANK,
        mss::mcbist::address(),
        mss::mcbist::address(TT::LARGEST_ADDRESS));
    sf_read_operation<mss::mc_type::NIMBUS> l_read_op(i_target, l_constraints);
    return l_read_op.execute();
}

inline void select_ports(const uint64_t i_ports)
{
    if (TT::MULTI_PORTS == mss::states::YES)
    {
        iv_control.insertFromRight<TT::PORT_SEL, TT::PORT_SEL_LEN>(i_ports);
    }
}


inline void change_end_boundary(const end_boundary i_end)
{
    // If there's no change, just get outta here
    if (i_end == 0xFF)
    {
        return;
    }
    // The values of the enum were crafted so that we can simply insertFromRight into the register.
    // We take note of whether to set the slave or master rank indicator and set that as well.
    // The hardware has to have a 1 or a 0 - so there is no choice for the rank detection. So it
    // doesn't matter that we're processing other end boundaries here - they'll just look like we
    // asked for a master rank detect.
    iv_config.insertFromRight<TT::CFG_PAUSE_ON_ERROR_MODE, TT::CFG_PAUSE_ON_ERROR_MODE_LEN>(i_end);
    const uint64_t l_detect_slave = fapi2::buffer<uint64_t>(i_end).getBit<TT::SLAVE_RANK_INDICATED_BIT>();
    iv_addr_gen.writeBit<TT::MAINT_DETECT_SRANK_BOUNDARIES>( l_detect_slave );
}

inline uint64_t get_port()
{
    return iv_address & 0xC000000000000000;
}

template<mss::mc_type MC = DEFAULT_MC_TYPE, fapi2::TargetType T, typename TT = mcbistTraits<MC, T>>
inline fapi2::ReturnCode config_address_range( const fapi2::Target<T>& i_target,
        const uint64_t i_start,
        const uint64_t i_end,
        const uint64_t i_index)
{
    fapi2::putScom(i_target, TT::address_pairs[i_index].first, i_start << 26);
    fapi2::putScom(i_target, TT::address_pairs[i_index].second, i_end << 26);
}

const mss::states is_broadcast_capable_helper(
    const uint8_t l_bc_mode_force_off,
    const uint8_t l_bc_mode_enable,
    const bool l_chip_bc_capable)
{
    if(!l_chip_bc_capable
     || l_bc_mode_force_off == fapi2::ENUM_ATTR_MSS_MRW_FORCE_BCMODE_OFF_YES
     || l_bc_mode_enable == fapi2::ENUM_ATTR_MSS_OVERRIDE_MEMDIAGS_BCMODE_DISABLE)
    {
        return mss::states::NO;
    }
    return mss::states::YES;
}

const mss::states is_broadcast_capable(const std::vector<mss::dimm::kind<>>& i_kinds)
{
    // If we don't have any DIMM kinds exit out
    if(i_kinds.size() == 0)
    {
        return mss::states::NO;
    }

    // Now the fun begins
    // First, get the starting kind - the 0th kind in the vector
    const auto l_expected_kind = i_kinds[0];

    // Now, find if we have any kinds that differ from our first kind
    // Note: starts on the next DIMM kind due to the fact that something always equals itself
    const auto l_kind_it = std::find_if(
        i_kinds.begin() + 1,
        i_kinds.end(),
        [&l_expected_kind]( const mss::dimm::kind<>& i_rhs) -> bool
        {
            return (l_expected_kind != i_rhs);
        });

    // If no DIMM kind was found that differs, then we are broadcast capable
    if(l_kind_it == i_kinds.end())
    {
        return mss::states::YES;
    }
    // Otherwise, note the MCA that differs and return that this is not broadcast capable
    return mss::states::NO;
}

template<>
const mss::states is_broadcast_capable(const fapi2::Target<fapi2::TARGET_TYPE_MCBIST>& i_target)
{
    // Chip is BC capable IFF the MCBIST end of rank bug is not present
    const auto l_chip_bc_capable = !mss::chip_ec_feature_mcbist_end_of_rank(i_target);

    // If BC mode is disabled in the MRW, then it's disabled here
    uint8_t l_bc_mode_enable = 0;
    uint8_t l_bc_mode_force_off = 0;
    mss::override_memdiags_bcmode(l_bc_mode_enable);
    mss::mrw_force_bcmode_off(l_bc_mode_force_off);

    // Check if the chip and attributes allows memdiags/mcbist to be in broadcast mode
    const auto l_state = is_broadcast_capable_helper(l_bc_mode_force_off, l_bc_mode_enable, l_chip_bc_capable);
    if(!l_chip_bc_capable
     || l_bc_mode_force_off == fapi2::ENUM_ATTR_MSS_MRW_FORCE_BCMODE_OFF_YES
     || l_bc_mode_enable == fapi2::ENUM_ATTR_MSS_OVERRIDE_MEMDIAGS_BCMODE_DISABLE)
    {
        return mss::states::NO;
    }

    // Now that we are guaranteed to have a chip that could run broadcast mode and the system is allowed to do so,
    // do the following steps to check whether our config is broadcast capable:

    // Steps to determine if this MCBIST is broadcastable
    // 1) Check the number of DIMM's on each MCA - true only if they all match
    // 2) Check that all of the DIMM kinds are equal - if they are, then we can do broadcast mode
    // 3) check the timing attributes (all of them must be equal)
    // 3) if both 1 and 2 are true, then broadcast capable, otherwise false

    // 1) Check the number of DIMM's on each MCA - if they don't match, then no
    const auto& l_mcas = mss::find_targets<fapi2::TARGET_TYPE_MCA>(i_target);
    const auto l_mca_check = is_broadcast_capable(l_mcas);

    // 2) Check that all of the DIMM kinds are equal - if they are, then we can do broadcast mode
    const auto l_dimms = mss::find_targets<fapi2::TARGET_TYPE_DIMM>(i_target);
    const auto l_dimm_kinds = mss::dimm::kind<>::vector(l_dimms);
    const auto l_dimm_kind_check = is_broadcast_capable(l_dimm_kinds);

    // 3) check the timing attributes (all of them must be equal)
    mss::states l_timing_check = mss::states::YES;
    is_broadcast_capable_timings(l_mcas, l_timing_check);

    // 4) if both 1-3 are true, then broadcastable, otherwise false
    return l_mca_check == mss::states::YES
        && l_dimm_kind_check == mss::states::YES
        && l_timing_check == mss::states::YES
        ? mss::states::YES : mss::states::NO;
}

template<>
const mss::states is_broadcast_capable(const std::vector<fapi2::Target<fapi2::TARGET_TYPE_MCA>>& i_targets)
{
    // If we don't have MCA's exit out
    if(i_targets.size() == 0)
    {
        return mss::states::NO;
    }

    // Now the fun begins
    // First, get the number of DIMM's on the 0th MCA
    const uint64_t l_first_mca_num_dimm = mss::count_dimm(i_targets[0]);

    // Now, find if we have any MCA's that have a different number of DIMM's
    // Note: starts on the next MCA target due to the fact that something always equals itself
    const auto l_mca_it = std::find_if(
        i_targets.begin() + 1,
        i_targets.end(),
        [l_first_mca_num_dimm](const fapi2::Target<fapi2::TARGET_TYPE_MCA>& i_rhs) -> bool
        {
            const uint64_t l_num_dimm = mss::count_dimm(i_rhs);
            return (l_first_mca_num_dimm != l_num_dimm);
        });

    if(l_mca_it == i_targets.end())
    {
        return mss::states::YES;
    }
    return mss::states::NO;
}

template<
    mss::mc_type MC = mss::mc_type::NIMBUS,
    fapi2::TargetType T = mss::mcbistMCTraits<mss::mc_type::NIMBUS>::MC_TARGET_TYPE,
    typename TT = mcbistTraits<mss::mc_type::NIMBUS, mss::mcbistMCTraits<mss::mc_type::NIMBUS>>
>
struct sf_read_operation : public operation<mss::mc_type::NIMBUS>
{

    sf_read_operation( const fapi2::Target<T>& i_target,
                       const constraints<MC> i_const,
                       fapi2::ReturnCode& o_rc):
        operation<MC>(i_target, mss::mcbist::read_subtest<MC>(), i_const)
    {
        o_rc = this->multi_port_init();
    }

    sf_read_operation() = delete;
};

template< mss::mc_type MC = DEFAULT_MC_TYPE, fapi2::TargetType T = mss::mcbistMCTraits<MC>::MC_TARGET_TYPE , typename TT = mcbistTraits<MC, T> >
struct constraints
{
    ///
    /// @brief constraints default constructor
    ///
    constraints():
        iv_stop(),
        iv_pattern(NO_PATTERN),
        iv_end_boundary(NONE),
        iv_speed(LUDICROUS),
        iv_start_address(0),
        iv_end_address(TT::LARGEST_ADDRESS)
    {
    }

    constraints( const uint64_t i_pattern ):
        constraints()
    {
        iv_pattern = i_pattern;
    }

    constraints(const stop_conditions<MC, T, TT>& i_stop,
                const speed i_speed,
                const end_boundary i_end_boundary,
                const address& i_start_address,
                const address& i_end_address = mcbist::address(TT::LARGEST_ADDRESS)):
        constraints(i_stop, i_start_address)
    {
        iv_end_boundary = i_end_boundary;
        iv_speed = i_speed;
        iv_end_address = i_end_address;
        iv_stop = i_stop;
        iv_start_address = i_start_address;

        if (iv_start_address > iv_end_address)
        {
            iv_end_address = iv_start_address;
        }
    }

    stop_conditions<MC, T, TT> iv_stop;
    uint64_t iv_pattern;
    end_boundary iv_end_boundary;
    speed iv_speed;
    mcbist::address iv_start_address;
    mcbist::address iv_end_address;
};

template<mss::mc_type MC = DEFAULT_MC_TYPE, fapi2::TargetType T, typename TT = portTraits<mss::mc_type::NIMBUS>>
inline fapi2::ReturnCode configure_wrq(const fapi2::Target<const fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>& i_target)
{
    for(const auto& l_port : mss::find_targets<TT::PORT_TYPE>(i_target))
    {
        fapi2::buffer<uint64_t> l_data;
        mss::getScom(l_port, portTraits<mss::mc_type::NIMBUS>::WRQ_REG, l_data);
        l_data.writeBit<portTraits<mss::mc_type::NIMBUS>::WRQ_FIFO_MODE>(1);
        mss::putScom(l_port, portTraits<mss::mc_type::NIMBUS>::WRQ_REG, l_data);
    }
}

template< mss::mc_type MC = DEFAULT_MC_TYPE, fapi2::TargetType T, typename TT = portTraits<mss::mc_type::NIMBUS>>
inline fapi2::ReturnCode configure_rrq(const fapi2::Target<const fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>& i_target)
{
    for(const auto& l_port : mss::find_targets<TT::PORT_TYPE>(i_target))
    {
        fapi2::buffer<uint64_t> l_data;
        mss::getScom(l_port, portTraits<mss::mc_type::NIMBUS>::RRQ_REG, l_data);
        l_data.writeBit<portTraits<mss::mc_type::NIMBUS>::RRQ_FIFO_MODE>(1);
        mss::putScom(l_port, portTraits<mss::mc_type::NIMBUS>::RRQ_REG, l_data);
    }
}

template<
    mss::mc_type MC = mss::mc_type::NIMBUS,
    fapi2::TargetType T = fapi2::Target<fapi2::TARGET_TYPE_MCBIST>,
    typename TT = mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>
>
inline void load_fifo_mode(
    const fapi2::Target<fapi2::TARGET_TYPE_MCBIST>& i_target,
    const mcbist::program<mss::mc_type::NIMBUS>& i_program )
{
    const auto l_subtest_it = std::find_if(
        i_program.iv_subtests.begin(),
        i_program.iv_subtests.end(),
        [](
            const mss::mcbist::subtest_t<mss::mc_type::NIMBUS,
            fapi2::Target<fapi2::TARGET_TYPE_MCBIST>,
            mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>>& i_rhs) -> bool
        {
            return i_rhs.fifo_mode_required();
        });

    if(l_subtest_it == i_program.iv_subtests.end())
    {
        return;
    }

    configure_wrq(i_target);
    configure_rrq(i_target);
}

class operation
{
    uint64_t iv_parameters; // class program
    uint64_t iv_address; // class address
    constraints<mss::mc_type::NIMBUS> iv_const;

    inline fapi2::ReturnCode execute(const fapi2::Target<const fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>& i_target)
    {
        fapi2::buffer<uint64_t> l_status;
        poll_parameters l_poll_parameters;

        fapi2::current_err = fapi2::FAPI2_RC_SUCCESS;

        fapi2::putScom(i_target, mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>::MCBSTATQ_REG, 0);
        fapi2::putScom(i_target, mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>::SRERR0_REG, 0);
        fapi2::putScom(i_target, mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>::SRERR1_REG, 0);
        fapi2::putScom(i_target, mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>::FIRQ_REG, 0);

        load_fifo_mode<mss::mc_type::NIMBUS>(i_target, ic_program);

        fapi2::putScom(i_target, mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>::MCBAGRAQ_REG, iv_program.iv_addr_gen);
        fapi2::putScom(i_target, mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>::MCBPARMQ_REG, iv_program.iv_parameters);
        fapi2::putScom(i_target, mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>::MCBAMR0A0Q_REG, iv_program.iv_addr_map0);
        fapi2::putScom(i_target, mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>::MCBAMR1A0Q_REG, iv_program.iv_addr_map1);
        fapi2::putScom(i_target, mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>::MCBAMR2A0Q_REG, iv_program.iv_addr_map2);
        fapi2::putScom(i_target, mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>::MCBAMR3A0Q_REG, iv_program.iv_addr_map3);

        load_config<mss::mc_type::NIMBUS>(i_target, iv_program);

        fapi2::putScom(i_target, mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>::CNTLQ_REG, iv_program.iv_control);

        load_data_config<mss::mc_type::NIMBUS>(i_target, iv_program);

        fapi2::putScom(i_target, mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>::THRESHOLD_REG, iv_program);

        load_mcbmr<mss::mc_type::NIMBUS>(i_target, iv_program);

        fapi2::buffer<uint64_t> l_buf;
        fapi2::getScom(i_target, mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>::CNTLQ_REG, l_buf);
        fapi2::putScom(i_target, mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>::CNTLQ_REG, l_buf.setBit<TT::MCBIST_START>());

        mss::poll(
            i_target,
            TT::STATQ_REG,
            l_poll_parameters,
            [&l_status](const size_t poll_remaining, const fapi2::buffer<uint64_t>& stat_reg) -> bool
            {
                l_status = stat_reg;
                return (l_status.getBit<mcbistTraits<mss::mc_type::NIMBUS, fapi2::Target<fapi2::TARGET_TYPE_MCBIST>>::MCBIST_IN_PROGRESS>() == true)
                    || (l_status.getBit<TT::MCBIST_DONE>() == true);
            });
        return mcbist::poll(i_target, iv_program);
    }

    template< mss::mc_type MC, fapi2::TargetType T, typename TT >
    inline void operation<MC, T, TT>::single_port_init()
    {
        using ET = mcbistMCTraits<MC>;
        const uint64_t l_relative_port_number = iv_const.iv_start_address.get_port();
        const uint64_t l_dimm_number = iv_const.iv_start_address.get_dimm();

        mss::mcbist::subtest_t<MC> l_subtest = iv_subtest;
        l_subtest.enable_port(l_relative_port_number);
        l_subtest.enable_dimm(l_dimm_number);
        iv_program.iv_subtests.push_back(l_subtest);
        if (iv_const.iv_end_address == TT::LARGEST_ADDRESS)
        {
            iv_const.iv_start_address.template get_range<mss::mcbist::address::DIMM>(iv_const.iv_end_address);
        }
        mss::mcbist::config_address_range0<MC>(iv_target, iv_const.iv_start_address, iv_const.iv_end_address);
        base_init();
    }

    template<>
    fapi2::ReturnCode operation<DEFAULT_MC_TYPE>::multi_port_addr()
    {
        using TT = mcbistTraits<>;

        mss::mcbist::address l_end_of_start_port;
        mss::mcbist::address l_end_of_complete_port(TT::LARGEST_ADDRESS);
        mss::mcbist::address l_start_of_end_port;

        // The last address in the start port is the start address thru the "DIMM range" (all addresses left on this DIMM)
        iv_const.iv_start_address.get_range<mss::mcbist::address::DIMM>(l_end_of_start_port);

        // The first address in the end port is the 0th address of the 0th DIMM on said port.
        l_start_of_end_port.set_port(iv_const.iv_end_address.get_port());

        // We know we have three address configs: start address -> end of DIMM, 0 -> end of DIMM and 0 -> end address.
        config_address_range<DEFAULT_MC_TYPE>(iv_target, iv_const.iv_start_address, l_end_of_start_port, 0);
        config_address_range<DEFAULT_MC_TYPE>(iv_target, mss::mcbist::address(), l_end_of_complete_port, 1);
        config_address_range<DEFAULT_MC_TYPE>(iv_target, l_start_of_end_port, iv_const.iv_end_address, 2);
    }

    template< mss::mc_type MC, fapi2::TargetType T, typename TT >
    inline void operation<MC, T, TT>::base_init()
    {
        memdiags::stop<MC>(iv_target);
        iv_pattern = iv_const.iv_pattern;
        iv_program.change_end_boundary(iv_const.iv_end_boundary);
        iv_thresholds = iv_const.iv_stop;
        iv_parameters.insertFromRight<TT::MIN_CMD_GAP, TT::MIN_CMD_GAP_LEN>(0);
        iv_parameters.writeBit<TT::MIN_GAP_TIMEBASE>(mss::OFF);
        iv_program.select_ports(0xF);
        iv_async = mss::ON;
    }

    template<>
    void operation<DEFAULT_MC_TYPE>::multi_port_init_internal()
    {
        auto l_dimms = mss::find_targets<fapi2::TARGET_TYPE_DIMM>(iv_target);
        const uint64_t l_portdimm_start_address = iv_const.iv_start_address.get_port_dimm();
        const uint64_t l_portdimm_end_address = iv_const.iv_end_address.get_port_dimm();

        if (l_portdimm_start_address == l_portdimm_end_address)
        {
            single_port_init();
            return;
        }
        if(mss::mcbist::is_broadcast_capable(iv_target) == mss::YES)
        {
            broadcast_mode_start_address_check(iv_target, l_portdimm_start_address, l_dimms);
        }
        multi_port_addr();
        configure_multiport_subtests(l_dimms);
        std::sort(
            iv_program.iv_subtests.begin(),
            iv_program.iv_subtests.end(),
            [](const decltype(iv_subtest)& a, const decltype(iv_subtest)& b) -> bool
            {
                const uint64_t l_a_portdimm = (a.get_port() << 1) | a.get_dimm();
                const uint64_t l_b_portdimm = (b.get_port() << 1) | b.get_dimm();
                return l_a_portdimm < l_b_portdimm;
            });
        base_init();
        mss::mcbist::configure_broadcast_mode(iv_target, iv_program);
    }

    template< mss::mc_type MC, fapi2::TargetType T, typename TT >
    inline void operation<MC, T, TT>::multi_port_init()
    {
        const auto l_port = mss::find_targets<TT::PORT_TYPE>(iv_target);
        // Make sure we have ports, if we don't then exit out
        if(l_port.size() == 0)
        {
            // Cronus can have no ports under an MCBIST, FW deconfigures by association
            return;
        }
        // Let's assume we are going to send out all subtest unless we are in broadcast mode,
        // where we only send up to 2 subtests under an port ( 1 for each DIMM) which is why no const
        auto l_dimms = mss::find_targets<fapi2::TARGET_TYPE_DIMM>(iv_target);
        if(l_dimms.size() == 0)
        {
            // Cronus can have no DIMMS under an MCBIST, FW deconfigures by association
            return;
        }
        multi_port_init_internal();
    }
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
}

void restoreDramRepairs(TargetHandle_t i_trgt)
{
    // Will need the chip and system objects initialized for several parts
    // of this function and sub-functions.
    if(false == g_initialized || nullptr == systemPtr)
    {
        noLock_initialize();
    }

    std::vector<MemRank> ranks;
    getMasterRanks<fapi2::TARGET_TYPE_MCA>(i_trgt, ranks);

    uint8_t rankMask = 0, dimmMask = 0;
    mssRestoreDramRepairs<fapi2::TARGET_TYPE_MCA>(i_trgt, rankMask, dimmMask);

    // Callout DIMMs with too many bad bits and not enough repairs available
    RDR::processBadDimms<fapi2::TARGET_TYPE_MCA>(i_trgt, dimmMask)

    // Check repaired ranks for RAS policy violations.
    RDR::processRepairedRanks<fapi2::TARGET_TYPE_MCA>(i_trgt, rankMask)
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

bool StateMachine::executeWorkItem(WorkFlowProperties * i_wfp)
{
    switch(*i_wfp->workItem)
    {
        case RESTORE_DRAM_REPAIRS:
            // Get the connected MCAs.
            TargetHandleList mcaList;
            getChildAffinityTargets(mcaList, i_wfp.assoc->first, CLASS_UNIT, fapi2::TARGET_TYPE_MCA);
            for (auto & mca : mcaList)
            {
                PRDF::restoreDramRepairs<fapi2::TARGET_TYPE_MCA>(mca);
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
