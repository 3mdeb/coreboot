#include <cpu/power/istep_14.h>

static void p9_pcie_config(void)
{
	for(size_t PECIndex = 0; PECIndex < PEC_PER_PROC; ++PECIndex)
	{
		scom_and_for_chiplet(
			pec_ids[PECIndex], P9N2_PEC_ADDREXTMASK_REG,
			~PPC_BITMASK(0, 6));
		scom_or_for_chiplet(
			pec_ids[PECIndex], PEC_PBCQHWCFG_REG,
			PPC_BIT(3) | PPC_BIT(7) | PPC_BIT(11)
			|	PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_OOO_MODE)
			| PPC_BIT(PEC_PBCQHWCFG_REG_PE_CHANNEL_STREAMING_EN)
			| PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_WR_VG)
			| PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_WR_SCOPE_GROUP)
			| PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_INTWR_VG)
			| PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_INTWR_SCOPE_GROUP)
			| PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_RD_VG)
			| PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_RD_SCOPE_GROUP)
			| PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_TCE_SCOPE_GROUP)
			| PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_TCE_VG));
		scom_or_for_chiplet(
			pec_ids[PECIndex], PEC_NESTTRC_REG,
			PPC_BIT(0) | PPC_BIT(3));
		write_scom_for_chiplet(
			pec_ids[PECIndex], PEC_PBAIBHWCFG_REG,
			0xe00000 | PPC_BIT(PEC_PBAIBHWCFG_REG_PE_PCIE_CLK_TRACE_EN));
	}
}

void istep_14_3(void)
{
	report_istep(14, 3);
	p9_pcie_config();
}
