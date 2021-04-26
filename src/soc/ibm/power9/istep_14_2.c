#include <cpu/power/istep_14_2.h>

void* call_mss_thermal_init ()
{
    nimbus_call_mss_thermal_init();
    run_proc_throttle_sync();
}

void nimbus_call_mss_thermal_init()
{
    TARGETING::TargetHandleList l_mcsTargetList;
    getAllChiplets(l_mcsTargetList, TYPE_MCS);
    //  --------------------------------------------------------------------
    //  run mss_thermal_init on all functional MCS chiplets
    //  --------------------------------------------------------------------
    for (const auto & l_pMcs : l_mcsTargetList)
    {
        fapi2::Target<fapi2::TARGET_TYPE_MCS> l_fapi_pMcs(l_pMcs);
        p9_mss_thermal_init(l_fapi_pMcs);
    }

}

void run_proc_throttle_sync()
{
    // Run proc throttle sync
    // Get all functional proc chip targets
    // Use targeting code to get a list of all processors
    TARGETING::TargetHandleList l_procChips;
    getAllChips(l_procChips, TARGETING::TYPE_PROC);
    for (const auto & l_procChip: l_procChips)
    {
        //C onvert the TARGETING::Target into a fapi2::Target by passing
        // l_procChip into the fapi2::Target constructor
        fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>l_fapi2CpuTarget(l_procChip);
        p9_throttle_sync(l_fapi2CpuTarget);
    }
}

void p9_mss_thermal_init(const fapi2::Target<TARGET_TYPE_MCS>& i_target)
{
    // Prior to starting OCC, we go into "safemode" throttling
    // After OCC is started, they can change throttles however they want
    for (const auto& l_mca : mss::find_targets<TARGET_TYPE_MCA>(i_target))
    {
        scom_and_or_for_chiplet(
          l_mca, MCA_MBA_FARB3Q,
          ~PPC_BITMASK(0, 45),
          PPC_BIT(10) | PPC_BIT(25) | PPC_BIT(37))
    }
    scom_and_for_chiplet(i_target, MCS_MCMODE0, PPC_BIT(21));
}

fapi2::ReturnCode p9_throttle_sync(
    const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target)
{
    auto l_mcsChiplets = i_target.getChildren<fapi2::TARGET_TYPE_MCS>();
    // Get the functional MCS on this proc
    if (l_mcsChiplets.size() > 0)
    {
        throttleSync(l_mcsChiplets);
    }
}

template <fapi2::TargetType T>
struct mcSideInfo_t
{
    bool masterMcFound = false;
    fapi2::Target<T> masterMc;   // Master MC for this MC side
};

template< fapi2::TargetType T>
fapi2::ReturnCode throttleSync(
    const std::vector< fapi2::Target<T> >& i_mcTargets)
{
    // MAX_MC_SIDES_PER_PROC = 2
    mcSideInfo_t<T> l_mcSide[MAX_MC_SIDES_PER_PROC];
    uint8_t l_sideNum = 0;
    uint8_t l_pos = 0;
    uint8_t l_numMasterProgrammed = 0;

    // Initialization
    // MAX_MC_SIDES_PER_PROC = 2
    for (l_sideNum = 0; l_sideNum < MAX_MC_SIDES_PER_PROC; l_sideNum++)
    {
        l_mcSide[l_sideNum].masterMcFound = false;
    }

    // ---------------------------------------------------------------------
    // 1. Pick the first MCS/MI with DIMMS as potential master
    //    for both MC sides (MC01/MC23)
    // ---------------------------------------------------------------------
    for (auto l_mc : i_mcTargets)
    {
        uint8_t l_num_dimms = findNumDimms(l_mc);

        if (l_num_dimms > 0)
        {
            // This MCS or MI has DIMMs attached, find out which MC side it
            // belongs to:
            //    l_sideNum = 0 --> MC01
            //                1 --> MC23
            // fapi2::ATTR_CHIP_UNIT_POS by default 0
            FAPI_ATTR_GET(fapi2::ATTR_CHIP_UNIT_POS, l_mc, l_pos);
            l_sideNum = l_pos / MAX_MC_SIDES_PER_PROC;
            // If there's no master MCS or MI marked for this side yet, mark
            // this MCS as master
            if (l_mcSide[l_sideNum].masterMcFound == false)
            {
                l_mcSide[l_sideNum].masterMcFound = true;
                l_mcSide[l_sideNum].masterMc = l_mc;
            }
        }
        progMCMODE0(l_mc, i_mcTargets);
    }

    // --------------------------------------------------------------
    // 2. Program the master MCS or MI
    // --------------------------------------------------------------
    // MAX_MC_SIDES_PER_PROC = 2
    for (l_sideNum = 0; l_sideNum < MAX_MC_SIDES_PER_PROC; l_sideNum++)
    {
        // If there is a potential master MCS or MI found for this side
        if (l_mcSide[l_sideNum].masterMcFound == true)
        {
            // No master MCS or MI programmed for either side yet,
            // go ahead and program this MCS or MI as master.
            if (l_numMasterProgrammed == 0)
            {
                progMaster(l_mcSide[l_sideNum].masterMc);
                l_numMasterProgrammed++;
            }
        }
    }
}

template< fapi2::TargetType T>
fapi2::ReturnCode progMaster(const fapi2::Target<T>& i_mcTarget)
{
    scom_and_for_chiplet(i_mcTarget, MCS_MCSYNC, ~PPC_BIT(MCS_MCSYNC_SYNC_GO_CH0));
    scom_and_or_for_chiplet(i_mcTarget, MCS_MCSYNC, ~PPC_BIT(SUPER_SYNC_BIT), PPC_BITMASK(0, 16));
    scom_and_for_chiplet(i_mcTarget, MCS_MCSYNC, ~PPC_BIT(MBA_REFRESH_SYNC_BIT));
}

template< fapi2::TargetType T>
fapi2::ReturnCode progMCMODE0(
    fapi2::Target<T>& i_mcTarget,
    const std::vector< fapi2::Target<T> >& i_mcTargets)
{
    // --------------------------------------------------------------
    // Setup MCMODE0 for disabling MC SYNC to other-side and same-side
    // partner unit.
    // BIT27: set if other-side MC is non-functional, 0<->2, 1<->3
    // BIT28: set if same-side MC is non-functional, 0<->1, 2<->3
    // --------------------------------------------------------------
    fapi2::buffer<uint64_t> l_scomData(0);
    fapi2::buffer<uint64_t> l_scomMask(0);
    bool l_other_side_functional = false;
    bool l_same_side_functional = false;
    uint8_t l_current_pos = 0;
    uint8_t l_other_side_pos = 0;
    uint8_t l_same_side_pos = 0;

    // fapi2::ATTR_CHIP_UNIT_POS by default 0
    FAPI_ATTR_GET(fapi2::ATTR_CHIP_UNIT_POS, i_mcTarget, l_current_pos);
    // Calculate the peer MC in the other side and in the same side.
    // MAX_MC_PER_PROC = 4
    // MAX_MC_PER_SIDE = 2
    l_other_side_pos = (l_current_pos + MAX_MC_PER_SIDE) % MAX_MC_PER_PROC;
    // MAX_MC_SIDES_PER_PROC = 2
    // MAX_MC_PER_SIDE = 2
    l_same_side_pos = ((l_current_pos / MAX_MC_SIDES_PER_PROC) * MAX_MC_PER_SIDE)
                     + ((l_current_pos % MAX_MC_PER_SIDE) + 1) % MAX_MC_PER_SIDE;
    // Determine side functionality
    for (auto l_mc : i_mcTargets)
    {
        uint8_t l_tmp_pos = 0;
        // fapi2::ATTR_CHIP_UNIT_POS by default 0
        FAPI_ATTR_GET(fapi2::ATTR_CHIP_UNIT_POS, l_mc, l_tmp_pos);
        if (l_tmp_pos == l_other_side_pos)
        {
            l_other_side_functional = true;
        }
        if (l_tmp_pos == l_same_side_pos)
        {
            l_same_side_functional = true;
        }
    }
    l_scomData = 0;
    l_scomMask =
        PPC_BIT(MCS_MCMODE0_DISABLE_MC_SYNC)
      | PPC_BIT(MCS_MCMODE0_DISABLE_MC_PAIR_SYNC);
    if (!l_other_side_functional)
    {
        l_scomData |= PPC_BIT(MCS_MCMODE0_DISABLE_MC_SYNC);
    }
    if (!l_same_side_functional)
    {
        l_scomData |= PPC_BIT(MCS_MCMODE0_DISABLE_MC_PAIR_SYNC);
    }
    scom_and_or_for_chiplet(
      i_mcTarget, MCS_MCMODE0,
      ~l_scomMask, l_scomData & l_scomMask)
}
