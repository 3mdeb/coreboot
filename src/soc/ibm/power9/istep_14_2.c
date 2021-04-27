#include <cpu/power/istep_14_2.h>

void istep_14_2(void)
{
    thermalInit();
    run_proc_throttle_sync();
}

void thermalInit(void)
{
    for(size_t MCSIndex = 0; MCSIndex < LEN(MCSTargets); ++MCSIndex) {
      chiplet_id_t MCSTarget = MCSTargets[MCSIndex];
      // MCA targets have the same id as MCS targets
      scom_and_or_for_chiplet(
        MCSTarget, MCA_MBA_FARB3Q,
        ~PPC_BITMASK(0, 45),
        PPC_BIT(10) | PPC_BIT(25) | PPC_BIT(37));
      scom_and_for_chiplet(MCSTarget, MCS_MCMODE0, PPC_BIT(21));
    }
}

void run_proc_throttle_sync(void)
{
    TARGETING::TargetHandleList l_procChips;
    getAllChips(l_procChips, TARGETING::TYPE_PROC);
    for(const auto & l_procChip: l_procChips)
    {
        throttleSync();
    }
}

template< fapi2::TargetType T>
fapi2::ReturnCode throttleSync(chiplet_id_t MCSTarget)
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
    for(size_t MCSIndex = 0; MCSIndex < LEN(MCSTargets); ++MCSIndex) {
    {
        chiplet_id_t MCSTarget = MCSTargets[MCSIndex];
        // This MCS or MI has DIMMs attached, find out which MC side it
        // belongs to:
        //    l_sideNum = 0 --> MC01
        //                1 --> MC23
        l_mcSide[0].masterMcFound = true;
        l_mcSide[0].masterMc = l_mc;
        progMCMODE0(MCSTarget, MCSTargets);
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

// template <fapi2::TargetType T>
// struct mcSideInfo_t
// {
//     bool masterMcFound = false;
//     fapi2::Target<T> masterMc;   // Master MC for this MC side
// };
//
// template< fapi2::TargetType T>
// fapi2::ReturnCode progMaster(const fapi2::Target<T>& i_mcTarget)
// {
//     scom_and_for_chiplet(i_mcTarget, MCS_MCSYNC, ~PPC_BIT(MCS_MCSYNC_SYNC_GO_CH0));
//     scom_and_or_for_chiplet(i_mcTarget, MCS_MCSYNC, ~PPC_BIT(SUPER_SYNC_BIT), PPC_BITMASK(0, 16));
//     scom_and_for_chiplet(i_mcTarget, MCS_MCSYNC, ~PPC_BIT(MBA_REFRESH_SYNC_BIT));
// }
//
template< fapi2::TargetType T>
fapi2::ReturnCode progMCMODE0(
    chiplet_id_t MCSTTarget,
    chiplet_id_t MCSTTargets[])
{
    uint64_t l_scomMask =
        PPC_BIT(MCS_MCMODE0_DISABLE_MC_SYNC)
      | PPC_BIT(MCS_MCMODE0_DISABLE_MC_PAIR_SYNC);
    uint64_t l_scomData =
        PPC_BIT(MCS_MCMODE0_DISABLE_MC_SYNC)
      | PPC_BIT(MCS_MCMODE0_DISABLE_MC_PAIR_SYNC);
    scom_and_or_for_chiplet(
      i_mcTarget, MCS_MCMODE0,
      ~l_scomMask, l_scomData & l_scomMask)
}
