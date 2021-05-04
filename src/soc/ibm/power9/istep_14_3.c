#include <cpu/power/istep_14.h>

static void p9_pcie_config(chiplet_id_t i_target)
{
	for (auto l_pec_chiplet : l_pec_chiplets_vec)
	{
		scom_and_for_chiplet(l_pec_chiplet, P9N2_PEC_ADDREXTMASK_REG, ~PPC_BITMASK(0, 6))
		scom_or_for_chiplet(
			l_pec_chiplet, PEC_PBCQHWCFG_REG,
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
			| PPC_BIT(PEC_PBCQHWCFG_REG_PE_DISABLE_TCE_VG))
		scom_or_for_chiplet(
			l_pec_chiplet,
			PEC_NESTTRC_REG,
			PPC_BIT(0) | PPC_BIT(3));
		write_scom_for_chiplet(
			l_pec_chiplet,
			PEC_PBAIBHWCFG_REG,
			0xe00000 | PPC_BIT(PEC_PBAIBHWCFG_REG_PE_PCIE_CLK_TRACE_EN))
	}

	for (auto l_phb_chiplet : l_phb_chiplets_vec)
	{
		fapi2::ATTR_PROC_PCIE_BAR_ENABLE_Type l_bar_enables;
		uint8_t l_phb_id = 0;
		FAPI_ATTR_GET(fapi2::ATTR_CHIP_UNIT_POS, l_phb_chiplet, l_phb_id);
		if (l_phb_id >= 0 && l_phb_id <= 5)
		{
			continue;
		}
		write_scom_for_chiplet(l_phb_chiplet, PHB_CERR_RPT0_REG, 0xFFFFFFFFFFFFFFFF);
		write_scom_for_chiplet(l_phb_chiplet, PHB_CERR_RPT1_REG, 0xFFFFFFFFFFFFFFFF);
		write_scom_for_chiplet(l_phb_chiplet, PHB_NFIR_REG, 0);
		write_scom_for_chiplet(l_phb_chiplet, PHB_NFIRWOF_REG, 0);
		write_scom_for_chiplet(l_phb_chiplet, PHB_NFIRACTION0_REG, 0x5b0f81e000000000);
		write_scom_for_chiplet(l_phb_chiplet, PHB_NFIRACTION1_REG, 0x7f0f81e000000000);
		write_scom_for_chiplet(l_phb_chiplet, PHB_NFIRMASK_REG, 0x30001c00000000);
		write_scom_for_chiplet(l_phb_chiplet, PHB_PE_DFREEZE_REG, 0);
		write_scom_for_chiplet(l_phb_chiplet, PHB_PBAIB_CERR_RPT_REG, 0);
		write_scom_for_chiplet(l_phb_chiplet, PHB_PFIR_REG, 0);
		write_scom_for_chiplet(l_phb_chiplet, PHB_PFIRWOF_REG, 0);
		write_scom_for_chiplet(l_phb_chiplet, PHB_PFIRACTION0_REG, 0);
		write_scom_for_chiplet(l_phb_chiplet, PHB_PFIRACTION1_REG, 0);
		write_scom_for_chiplet(l_phb_chiplet, PHB_PFIRMASK_REG, 0);

		uint64_t l_mmio0_bar =
			PPC_BIT(FABRIC_ADDR_MSEL_START_BIT) | PPC_BIT(FABRIC_ADDR_MSEL_END_BIT);
		uint64_t l_mmio1_bar =
			PPC_BIT(FABRIC_ADDR_MSEL_START_BIT) | PPC_BIT(FABRIC_ADDR_MSEL_END_BIT);
		uint64_t l_register_bar =
			PPC_BIT(FABRIC_ADDR_MSEL_START_BIT) | PPC_BIT(FABRIC_ADDR_MSEL_END_BIT);

		l_mmio0_bar = l_mmio0_bar << P9_PCIE_CONFIG_BAR_SHIFT;
		write_scom_for_chiplet(l_phb_chiplet, PHB_MMIOBAR0_REG, l_mmio0_bar);
		write_scom_for_chiplet(l_phb_chiplet, PHB_MMIOBAR0_MASK_REG, 0);
		l_mmio1_bar = l_mmio1_bar << P9_PCIE_CONFIG_BAR_SHIFT;
		write_scom_for_chiplet(l_phb_chiplet, PHB_MMIOBAR1_REG, l_mmio1_bar);
		write_scom_for_chiplet(l_phb_chiplet, PHB_MMIOBAR1_MASK_REG, 0);
		l_register_bar = l_register_bar << P9_PCIE_CONFIG_BAR_SHIFT;
		write_scom_for_chiplet(l_phb_chiplet, PHB_PHBBAR_REG, l_register_bar);

		uint64_t l_buf = 0;
		if (ATTR_PROC_PCIE_BAR_ENABLE[0])
		{
			l_buf |= PPC_BIT(PHB_BARE_REG_PE_MMIO_BAR0_EN);
		}
		if (ATTR_PROC_PCIE_BAR_ENABLE[1])
		{
			l_buf |= PPC_BIT(PHB_BARE_REG_PE_MMIO_BAR1_EN);
		}
		if (ATTR_PROC_PCIE_BAR_ENABLE[2])
		{
			l_buf |= PPC_BIT(PHB_BARE_REG_PE_PHB_BAR_EN);
		}
		write_scom_for_chiplet(l_phb_chiplet, PHB_BARE_REG, l_buf);
		write_scom_for_chiplet(l_phb_chiplet, PHB_PHBRESET_REG, 0);
		write_scom_for_chiplet(l_phb_chiplet, PHB_ACT0_REG, 0);
		write_scom_for_chiplet(l_phb_chiplet, PHB_ACTION1_REG, 0);
		write_scom_for_chiplet(l_phb_chiplet, PHB_MASK_REG, 0xFFFFFFFFFFFFFFFF);
	}
}

void istep_14_3(void)
{
	getAllChips(l_procChips, TYPE_PROC);
	for (const auto & l_procChip: l_procChips)
	{
		p9_pcie_config(l_fapi_cpu_target);
	}
}
