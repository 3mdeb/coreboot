#include <cpu/power/istep_14_2.h>

#define LEN(a) (sizeof(a)/sizeof(*a))
chiplet_id_t MCSTargets[] = {MC01_CHIPLET_ID, MC23_CHIPLET_ID};

void istep_14_2(void)
{
	report_istep(14, 2);
	thermalInit();
	throttleSync();
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
void throttleSync(void)
{
	for(size_t MCSIndex = 0; MCSIndex < LEN(MCSTargets); ++MCSIndex)
	{
		progMCSMode0(MCSTargets[MCSIndex]);
	}
	progMaster(MCSTargets[0]);
}

void progMaster(chiplet_id_t MCSTTarget)
{
	scom_and_for_chiplet(MCSTTarget, MCS_MCSYNC, ~PPC_BIT(MCS_MCSYNC_SYNC_GO_CH0));
	scom_and_or_for_chiplet(MCSTTarget, MCS_MCSYNC, ~PPC_BIT(SUPER_SYNC_BIT), PPC_BITMASK(0, 16));
	scom_and_for_chiplet(MCSTTarget, MCS_MCSYNC, ~PPC_BIT(MBA_REFRESH_SYNC_BIT));
}

void progMCSMode0(chiplet_id_t MCSTarget)
{
	uint64_t l_scomMask =
		PPC_BIT(MCS_MCMODE0_DISABLE_MC_SYNC)
		| PPC_BIT(MCS_MCMODE0_DISABLE_MC_PAIR_SYNC);
	uint64_t l_scomData =
		PPC_BIT(MCS_MCMODE0_DISABLE_MC_SYNC)
		| PPC_BIT(MCS_MCMODE0_DISABLE_MC_PAIR_SYNC);
	scom_and_or_for_chiplet(
		MCSTarget, MCS_MCMODE0,
		~l_scomMask, l_scomData & l_scomMask);
}
