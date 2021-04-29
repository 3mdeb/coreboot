#include <cpu/power/istep_14.h>
#include <cpu/power/istep_13.h>

static void thermalInit(void)
{
    for(size_t MCSIndex = 0; MCSIndex < MCS_PER_PROC; ++MCSIndex) {
      chiplet_id_t MCSTarget = mcs_ids[MCSIndex];
      // MCA targets have the same id as MCS targets
      scom_and_or_for_chiplet(
        MCSTarget, MCA_MBA_FARB3Q,
        ~PPC_BITMASK(0, 45),
        PPC_BIT(10) | PPC_BIT(25) | PPC_BIT(37));
      scom_and_for_chiplet(MCSTarget, MCS_MCMODE0, PPC_BIT(21));
    }
}

static void progMCMode0(chiplet_id_t MCSTTarget)
{
    uint64_t l_scomMask =
        PPC_BIT(MCS_MCMODE0_DISABLE_MC_SYNC)
      | PPC_BIT(MCS_MCMODE0_DISABLE_MC_PAIR_SYNC);
    uint64_t l_scomData =
        PPC_BIT(MCS_MCMODE0_DISABLE_MC_SYNC)
      | PPC_BIT(MCS_MCMODE0_DISABLE_MC_PAIR_SYNC);
    scom_and_or_for_chiplet(
      MCSTTarget, MCS_MCMODE0,
      ~l_scomMask, l_scomData & l_scomMask);
}

static void progMaster(chiplet_id_t MCSTarget)
{
    scom_and_for_chiplet(MCSTarget, MCS_MCSYNC, ~PPC_BIT(MCS_MCSYNC_SYNC_GO_CH0));
    scom_and_or_for_chiplet(MCSTarget, MCS_MCSYNC, ~PPC_BIT(SUPER_SYNC_BIT), PPC_BITMASK(0, 16));
    scom_and_for_chiplet(MCSTarget, MCS_MCSYNC, ~PPC_BIT(MBA_REFRESH_SYNC_BIT));
}

static void throttleSync(void)
{
    for(size_t MCSIndex = 0; MCSIndex < MCS_PER_PROC; ++MCSIndex)
    {
        progMCMode0(mcs_ids[MCSIndex]);
    }
		progMaster(mcs_ids[0]);
}

void istep_14_2(void)
{
	report_istep(14, 2);
	thermalInit();
	throttleSync();
}
