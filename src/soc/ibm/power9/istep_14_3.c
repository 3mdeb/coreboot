#include <cpu/power/istep_14.h>

void* call_proc_pcie_config (void *io_pArgs)
{
	TARGETING::TargetHandleList l_procChips;
	getAllChips(l_procChips, TYPE_PROC);
	for (const auto & l_procChip: l_procChips)
	{
		p9_pcie_config(l_fapi_cpu_target);
	}
}

void p9_pcie_config(chiplet_id_t i_target)
{
  // variables should be stored in non-volatile memory
	uint64_t l_mmio_bar0_offsets[6] = {0, 0, 0, 0, 0, 0};
	uint64_t l_mmio_bar1_offsets[6] = {0, 0, 0, 0, 0, 0};
	uint64_t l_register_bar_offsets[6] = {0, 0, 0, 0, 0, 0};
	// NOTE 0 values doesn't seem right, but seem to be the default
	fapi2::ATTR_PROC_PCIE_BAR_SIZE_Type l_bar_sizes[3] = {0, 0, 0};

	fapi2::Target<fapi2::TARGET_TYPE_SYSTEM> FAPI_SYSTEM;

	fapi2::buffer<uint64_t> l_buf = 0;
	uint8_t pec1_iovalid_bits = 0;
	uint8_t pec2_iovalid_bits = 0;
	std::vector<uint64_t> l_base_addr_nm0, l_base_addr_nm1, l_base_addr_m;
	uint64_t l_base_addr_mmio;

	auto l_pec_chiplets_vec = i_target.getChildren<fapi2::TARGET_TYPE_PEC>(fapi2::TARGET_STATE_FUNCTIONAL);
	auto l_phb_chiplets_vec = i_target.getChildren<fapi2::TARGET_TYPE_PHB>(fapi2::TARGET_STATE_FUNCTIONAL);

	// determine base address of chip MMIO range
	p9_fbc_utils_get_chip_base_address(i_target, l_base_addr_nm0, l_base_addr_nm1, l_base_addr_m, l_base_addr_mmio);
	for (auto l_pec_chiplet : l_pec_chiplets_vec)
	{
		fapi2::getScom(l_pec_chiplet, P9N2_PEC_ADDREXTMASK_REG, l_buf);
		l_buf.insertFromRight<0,7>((fapi2::ATTR_FABRIC_ADDR_EXTENSION_GROUP_ID << 3) | fapi2::ATTR_FABRIC_ADDR_EXTENSION_CHIP_ID);
		fapi2::putScom(l_pec_chiplet, P9N2_PEC_ADDREXTMASK_REG, l_buf);

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
		fapi2::buffer<uint64_t> l_mmio0_bar = l_base_addr_mmio;
		fapi2::buffer<uint64_t> l_mmio1_bar = l_base_addr_mmio;
		fapi2::buffer<uint64_t> l_register_bar = l_base_addr_mmio;
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
		FAPI_ATTR_GET(fapi2::ATTR_PROC_PCIE_BAR_ENABLE, l_phb_chiplet, l_bar_enables);
		l_mmio0_bar += l_mmio_bar0_offsets[l_phb_id];
		l_mmio0_bar = l_mmio0_bar << P9_PCIE_CONFIG_BAR_SHIFT;
		fapi2::putScom(l_phb_chiplet, PHB_MMIOBAR0_REG, l_mmio0_bar);
		fapi2::putScom(l_phb_chiplet, PHB_MMIOBAR0_MASK_REG, l_bar_sizes[0]);
		l_mmio1_bar += l_mmio_bar1_offsets[l_phb_id];
		l_mmio1_bar = l_mmio1_bar << P9_PCIE_CONFIG_BAR_SHIFT;
		fapi2::putScom(l_phb_chiplet, PHB_MMIOBAR1_REG, l_mmio1_bar);
		fapi2::putScom(l_phb_chiplet, PHB_MMIOBAR1_MASK_REG, l_bar_sizes[1]);
		l_register_bar += l_register_bar_offsets[l_phb_id];
		l_register_bar = l_register_bar << P9_PCIE_CONFIG_BAR_SHIFT;
		fapi2::putScom(l_phb_chiplet, PHB_PHBBAR_REG, l_register_bar);
		l_buf = 0;
		if (l_bar_enables[0])
		{
			l_buf |= PPC_BIT(PHB_BARE_REG_PE_MMIO_BAR0_EN);
		}
		if (l_bar_enables[1])
		{
			l_buf |= PPC_BIT(PHB_BARE_REG_PE_MMIO_BAR1_EN);
		}
		if (l_bar_enables[2])
		{
			l_buf |= PPC_BIT(PHB_BARE_REG_PE_PHB_BAR_EN);
		}
		fapi2::putScom(l_phb_chiplet, PHB_BARE_REG, l_buf);
		fapi2::putScom(l_phb_chiplet, PHB_PHBRESET_REG, 0);
		fapi2::putScom(l_phb_chiplet, PHB_ACT0_REG, 0);
		fapi2::putScom(l_phb_chiplet, PHB_ACTION1_REG, 0);
		fapi2::putScom(l_phb_chiplet, PHB_MASK_REG, 0xFFFFFFFFFFFFFFFF);
	}
}

void check_proc0_memory_config(void)
{
    TargetHandleList l_procsList;
    getAllChips(l_procsList, TYPE_PROC);

    procIds_t l_procIds[l_procsList.size()];
    uint8_t i = 0;
    uint8_t l_proc0 = INVALID_PROC;
    uint8_t l_victim = INVALID_PROC;
    for (const auto & l_procChip : l_procsList)
    {
        l_procIds[i].proc = l_procChip;

        l_procIds[i].groupIdDflt = l_procChip->getAttr<ATTR_FABRIC_GROUP_ID>();
        l_procIds[i].groupIdEff = l_procChip->getAttr<ATTR_PROC_EFF_FABRIC_GROUP_ID>();
        l_procIds[i].groupId = l_procIds[i].groupIdDflt;

        l_procIds[i].chipIdDflt = l_procChip->getAttr<ATTR_FABRIC_CHIP_ID>();
        l_procIds[i].chipIdEff = l_procChip->getAttr<ATTR_PROC_EFF_FABRIC_CHIP_ID>();
        l_procIds[i].chipId = l_procIds[i].chipIdDflt;

        if(l_proc0 == INVALID_PROC)
        {
            l_proc0 = i;
        }
        else if (
          PIR_t::createChipId(l_procIds[i].groupId, l_procIds[i].chipId)
          < PIR_t::createChipId(l_procIds[l_proc0].groupId, l_procIds[l_proc0].chipId))
        {
            l_proc0 = i;
        }
        i++;
    }

    // Get the functional DIMMs for proc0
    PredicateHwas l_functional;
    l_functional.functional(true);
    TargetHandleList l_dimms;
    PredicateCTM l_dimm(CLASS_LOGICAL_CARD, TYPE_DIMM);
    PredicatePostfixExpr l_checkExprFunctional;
    l_checkExprFunctional.push(&l_dimm).push(&l_functional).And();
    targetService().getAssociated(
      l_dimms, l_procIds[l_proc0].proc, TargetService::CHILD_BY_AFFINITY,
      TargetService::ALL, &l_checkExprFunctional);

    if(l_dimms.empty())
    {
        for (i = 0; i < l_procsList.size(); i++)
        {
            if(i == l_proc0)
            {
                continue;
            }

            targetService().getAssociated(
              l_dimms, l_procIds[i].proc, TargetService::CHILD_BY_AFFINITY,
              TargetService::ALL, &l_checkExprFunctional);

            if(l_dimms.empty())
            {
                continue;
            }
            l_victim = i;

            l_procIds[l_proc0].groupId = l_procIds[l_victim].groupIdDflt;
            l_procIds[l_proc0].chipId = l_procIds[l_victim].chipIdDflt;

            l_procIds[l_victim].groupId = l_procIds[l_proc0].groupIdDflt;
            l_procIds[l_victim].chipId = l_procIds[l_proc0].chipIdDflt;
            break;
        }
    }
    for (i = 0; i < l_procsList.size(); i++)
    {
        if((l_procIds[i].groupId != l_procIds[i].groupIdEff) ||
           (l_procIds[i].chipId != l_procIds[i].chipIdEff) )
        {
            (l_procIds[i].proc)->setAttr<ATTR_PROC_EFF_FABRIC_GROUP_ID>(l_procIds[i].groupId);
            (l_procIds[i].proc)->setAttr<ATTR_PROC_EFF_FABRIC_CHIP_ID>(l_procIds[i].chipId);
        }
    }
}

void p9_fbc_utils_get_chip_base_address_no_aliases(
    const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target,
    uint64_t& o_base_address_nm0,
    uint64_t& o_base_address_nm1,
    uint64_t& o_base_address_m,
    uint64_t& o_base_address_mmio)
{
    uint32_t l_fabric_system_id;
    uint8_t l_fabric_group_id;
    uint8_t l_fabric_chip_id;
    uint8_t l_mirror_policy;
    fapi2::buffer<uint64_t> l_base_address;
    fapi2::buffer<uint8_t> l_proc_chip_mem_to_use = 0;
    const fapi2::Target<fapi2::TARGET_TYPE_SYSTEM> FAPI_SYSTEM;

    FAPI_ATTR_GET(fapi2::ATTR_PROC_EFF_FABRIC_GROUP_ID, i_target, l_fabric_group_id);
    FAPI_ATTR_GET(fapi2::ATTR_PROC_EFF_FABRIC_CHIP_ID, i_target, l_fabric_chip_id);

    l_base_address.insertFromRight<8,5>(0);
    l_base_address.insertFromRight<15,4>(l_fabric_group_id);
    l_base_address.insertFromRight<19,3>(l_fabric_chip_id);
    o_base_address_nm0 = l_base_address;

    l_base_address |= PPC_BIT(FABRIC_ADDR_MSEL_END_BIT>();
    o_base_address_nm1 = l_base_address;

    l_base_address |= PPC_BIT(FABRIC_ADDR_MSEL_START_BIT);
    l_base_address &= ~PPC_BIT(FABRIC_ADDR_MSEL_END_BIT>);
    o_base_address_m = l_base_address;

    l_base_address |= PPC_BIT(FABRIC_ADDR_MSEL_END_BIT);
    o_base_address_mmio = l_base_address;
}

void p9_fbc_utils_get_chip_base_address(
    const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target,
    std::vector<uint64_t>& o_base_address_nm0,
    std::vector<uint64_t>& o_base_address_nm1,
    std::vector<uint64_t>& o_base_address_m,
    uint64_t& o_base_address_mmio)
{
    uint64_t l_base_address_nm0 = 0;
    uint64_t l_base_address_nm1 = 0;
    uint64_t l_base_address_m = 0;
    uint8_t l_addr_extension_group_id;
    std::vector<uint8_t> l_alias_bit_positions;
    fapi2::Target<fapi2::TARGET_TYPE_SYSTEM> FAPI_SYSTEM;

    p9_fbc_utils_get_chip_base_address_no_aliases(i_target,
             l_base_address_nm0,
             l_base_address_nm1,
             l_base_address_m,
             o_base_address_mmio);

    o_base_address_nm0.push_back(l_base_address_nm0;
    o_base_address_m.push_back(l_base_address_m);
    o_base_address_m.push_back(l_base_address_m + MAX_INTERLEAVE_GROUP_SIZE / 2);
    o_base_address_nm1.push_back(l_base_address_nm1);
}
