/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_8_11.h>
#include <cpu/power/scom.h>

void call_proc_xbus_enable_ridi(void)
{
    p9_chiplet_scominit();
    // && p9_io_obus_firmask_save(target, i_target_chip.getChildren<fapi2::TARGET_TYPE_OBUS>(fapi2::TARGET_STATE_FUNCTIONAL));
    // && p9_psi_scom(target)
    // && p9_io_obus_scominit(target)
    // && p9_npu_scominit(target)
    // && p9_chiplet_enable_ridi(target);
}

void p9_chiplet_scominit(void)
{
    fapi2::buffer<uint64_t> l_scom_data;
    char l_procTargetStr[fapi2::MAX_ECMD_STRING_LEN];
    char l_chipletTargetStr[fapi2::MAX_ECMD_STRING_LEN];
    fapi2::Target<fapi2::TARGET_TYPE_SYSTEM> FAPI_SYSTEM;
    uint8_t l_xbus_present[P9_FBC_UTILS_MAX_ELECTRICAL_LINKS] = {0};
    uint8_t l_xbus_functional[P9_FBC_UTILS_MAX_ELECTRICAL_LINKS] = {0};
    std::vector<fapi2::Target<fapi2::TARGET_TYPE_OBUS>> l_obus_chiplets;
    std::vector<fapi2::Target<fapi2::TARGET_TYPE_MCS>>  l_mcs_targets;
    std::vector<fapi2::Target<fapi2::TARGET_TYPE_MI>>   l_mi_targets;
    std::vector<fapi2::Target<fapi2::TARGET_TYPE_MC>>   l_mc_targets;
    std::vector<fapi2::Target<fapi2::TARGET_TYPE_DMI>>  l_dmi_targets;
    std::vector<fapi2::Target<fapi2::TARGET_TYPE_CAPP>> l_capp_targets;
    std::vector<fapi2::Target<fapi2::TARGET_TYPE_MCC>>  l_mcc_targets;
    std::vector<fapi2::Target<fapi2::TARGET_TYPE_OMI>>  l_omi_targets;

    l_obus_chiplets = i_target.getChildren<fapi2::TARGET_TYPE_OBUS>();
    l_mcs_targets = i_target.getChildren<fapi2::TARGET_TYPE_MCS>();
    l_mi_targets = i_target.getChildren<fapi2::TARGET_TYPE_MI>();
    l_dmi_targets = i_target.getChildren<fapi2::TARGET_TYPE_DMI>();
    l_mc_targets = i_target.getChildren<fapi2::TARGET_TYPE_MC>();
    l_omi_targets = i_target.getChildren<fapi2::TARGET_TYPE_OMI>();

    if(l_mcs_targets.size())
    {
        for (const auto &l_mcs_target : l_mcs_targets)
        {
            FAPI_EXEC_HWP(l_rc, p9n_mcs_scom, l_mcs_target, FAPI_SYSTEM, i_target, l_mcs_target.getParent<fapi2::TARGET_TYPE_MCBIST>());
        }
    }
    else if(l_mc_targets.size())
    {

        for (auto l_dmi_target : l_dmi_targets)
        {
            //--------------------------------------------------
            //-- Cumulus
            //--------------------------------------------------
            FAPI_EXEC_HWP(l_rc, p9c_dmi_scom, l_dmi_target, FAPI_SYSTEM, i_target);
        }
    }

    // read spare FBC FIR bit -- ifset, SBE has configured XBUS FIR resources for all
    // present units, and code here will be run to mask resources associated with
    // non-functional units
    l_scom_data = read_scom(PU_PB_CENT_SM0_PB_CENT_FIR_REG);

    if(l_scom_data.getBit<PU_PB_CENT_SM0_PB_CENT_FIR_MASK_REG_SPARE_13>())
    {
        for (auto &l_cplt_target : i_target.getChildren<fapi2::TARGET_TYPE_PERV>(static_cast<fapi2::TargetFilter>(fapi2::TARGET_FILTER_XBUS), fapi2::TARGET_STATE_FUNCTIONAL))
        {
            fapi2::buffer<uint16_t> l_attr_pg = 0;
            // obtain partial good information to determine viable X links
            l_attr_pg = fapi2::ATTR_PG[l_cplt_target];

            for (uint8_t ii = 0; ii < P9_FBC_UTILS_MAX_ELECTRICAL_LINKS; ii++)
            {
                l_xbus_present[ii] = !l_attr_pg.getBit(X_PG_IOX0_REGION_BIT + ii) && !l_attr_pg.getBit(X_PG_PBIOX0_REGION_BIT + ii);
            }
        }

        uint8_t l_unit_pos;
        l_unit_pos = fapi2::ATTR_CHIP_UNIT_POS[6];
        l_xbus_functional[l_unit_pos] = 1;

        if(l_xbus_present[0] && !l_xbus_functional[0])
        {
            // XBUS0 FBC DL
            write_scom(XBUS_0_LL0_LL0_LL0_IOEL_FIR_MASK_REG, FBC_IOE_DL_FIR_MASK_NF);
            // XBUS0 PHY
            write_scom(XBUS_FIR_MASK_REG, XBUS_PHY_FIR_MASK_NF);
        }

        if(!l_xbus_functional[0])
        {
            // XBUS0 FBC TL
            write_scom(PU_PB_IOE_FIR_MASK_REG_OR, FBC_IOE_TL_FIR_MASK_X0_NF);
            // XBUS0 EXTFIR
            write_scom(PU_PB_CENT_SM1_EXTFIR_MASK_REG_OR, FBC_EXT_FIR_MASK_X0_NF);
        }

        if(l_xbus_present[1] && !l_xbus_functional[1])
        {
            // XBUS1 FBC DL
            write_scom(XBUS_1_LL1_LL1_LL1_IOEL_FIR_MASK_REG, FBC_IOE_DL_FIR_MASK_NF);
            // XBUS1 PHY
            write_scom(XBUS_1_FIR_MASK_REG, XBUS_PHY_FIR_MASK_NF);
        }

        if(!l_xbus_functional[1])
        {
            // XBUS1 FBC TL
            write_scom(PU_PB_IOE_FIR_MASK_REG_OR, FBC_IOE_TL_FIR_MASK_X1_NF);
            // XBUS1 EXTFIR
            write_scom(PU_PB_CENT_SM1_EXTFIR_MASK_REG_OR, FBC_EXT_FIR_MASK_X1_NF);
        }

        if(l_xbus_present[2] && !l_xbus_functional[2])
        {
            // XBUS2 FBC DL
            write_scom(XBUS_2_LL2_LL2_LL2_IOEL_FIR_MASK_REG, FBC_IOE_DL_FIR_MASK_NF);
            // XBUS2 PHY
            write_scom(XBUS_2_FIR_MASK_REG, XBUS_PHY_FIR_MASK_NF);
        }

        if(!l_xbus_functional[2])
        {
            // XBUS2 FBC TL
            write_scom(PU_PB_IOE_FIR_MASK_REG_OR, FBC_IOE_TL_FIR_MASK_X2_NF);
            // XBUS2 EXTFIR
            write_scom(PU_PB_CENT_SM1_EXTFIR_MASK_REG_OR, FBC_EXT_FIR_MASK_X2_NF);
        }
    }

    FAPI_EXEC_HWP(l_rc, p9_fbc_ioo_tl_scom, i_target, FAPI_SYSTEM);

    if(l_obus_chiplets.size())
    {
        write_scom(PU_IOE_PB_IOO_FIR_ACTION0_REG, FBC_IOO_TL_FIR_ACTION0);
        write_scom(PU_IOE_PB_IOO_FIR_ACTION1_REG, FBC_IOO_TL_FIR_ACTION1);
        write_scom(PU_IOE_PB_IOO_FIR_MASK_REG, FBC_IOO_TL_FIR_MASK);
    }

    for (auto l_iter = l_obus_chiplets.begin();
         l_iter != l_obus_chiplets.end();
         l_iter++)
    {
        // configure action registers & unmask
        fapi2::putScom(*l_iter, OBUS_LL0_PB_IOOL_FIR_ACTION0_REG, FBC_IOO_DL_FIR_ACTION0);
        fapi2::putScom(*l_iter, OBUS_LL0_PB_IOOL_FIR_ACTION1_REG, FBC_IOO_DL_FIR_ACTION1);
        fapi2::putScom(*l_iter, OBUS_LL0_LL0_LL0_PB_IOOL_FIR_MASK_REG, FBC_IOO_DL_FIR_MASK);
        FAPI_EXEC_HWP(l_rc, p9_fbc_ioo_dl_scom, *l_iter, i_target, FAPI_SYSTEM);
    }
    // Invoke NX SCOM initfile
    FAPI_EXEC_HWP(l_rc, p9_nx_scom, i_target, FAPI_SYSTEM);
    // Invoke CXA SCOM initfile
    l_capp_targets = i_target.getChildren<fapi2::TARGET_TYPE_CAPP>();
    for (auto l_capp : l_capp_targets)
    {
        FAPI_EXEC_HWP(l_rc, p9_cxa_scom, l_capp, FAPI_SYSTEM, i_target);
    }
    // Invoke INT SCOM initfile
    FAPI_EXEC_HWP(l_rc, p9_int_scom, i_target, FAPI_SYSTEM);
    // Invoke VAS SCOM initfile
    FAPI_EXEC_HWP(l_rc, p9_vas_scom, i_target, FAPI_SYSTEM);

    // Setup NMMU epsilon write cycles
    l_scom_data = read_scom(PU_NMMU_MM_EPSILON_COUNTER_VALUE);
    l_scom_data.insertFromRight<PU_NMMU_MM_EPSILON_COUNTER_VALUE_WR_TIER_1_CNT_VAL, PU_NMMU_MM_EPSILON_COUNTER_VALUE_WR_TIER_1_CNT_VAL_LEN>(7);
    l_scom_data.insertFromRight<PU_NMMU_MM_EPSILON_COUNTER_VALUE_WR_TIER_2_CNT_VAL, PU_NMMU_MM_EPSILON_COUNTER_VALUE_WR_TIER_2_CNT_VAL_LEN>(67);
    write_scom(PU_NMMU_MM_EPSILON_COUNTER_VALUE, l_scom_data);
}

fapi2::ReturnCode p9_nx_scom(const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& TGT0,
                             const fapi2::Target<fapi2::TARGET_TYPE_SYSTEM>& TGT1)
{
    {
        fapi2::ATTR_EC_Type   l_chip_ec;
        fapi2::ATTR_NAME_Type l_chip_id;
        FAPI_ATTR_GET_PRIVILEGED(fapi2::ATTR_NAME, TGT0, l_chip_id);
        FAPI_ATTR_GET_PRIVILEGED(fapi2::ATTR_EC, TGT0, l_chip_ec);
        fapi2::ATTR_CHIP_EC_FEATURE_HW403701_Type l_TGT0_ATTR_CHIP_EC_FEATURE_HW403701;
        FAPI_ATTR_GET(fapi2::ATTR_CHIP_EC_FEATURE_HW403701, TGT0, l_TGT0_ATTR_CHIP_EC_FEATURE_HW403701);
        fapi2::ATTR_CHIP_EC_FEATURE_HW414700_Type l_TGT0_ATTR_CHIP_EC_FEATURE_HW414700;
        FAPI_ATTR_GET(fapi2::ATTR_CHIP_EC_FEATURE_HW414700, TGT0, l_TGT0_ATTR_CHIP_EC_FEATURE_HW414700);
        fapi2::ATTR_PROC_FABRIC_PUMP_MODE_Type l_TGT1_ATTR_PROC_FABRIC_PUMP_MODE;
        FAPI_ATTR_GET(fapi2::ATTR_PROC_FABRIC_PUMP_MODE, TGT1, l_TGT1_ATTR_PROC_FABRIC_PUMP_MODE);
        fapi2::ATTR_FABRIC_ADDR_EXTENSION_GROUP_ID_Type l_TGT1_ATTR_FABRIC_ADDR_EXTENSION_GROUP_ID;
        FAPI_ATTR_GET(fapi2::ATTR_FABRIC_ADDR_EXTENSION_GROUP_ID, TGT1, l_TGT1_ATTR_FABRIC_ADDR_EXTENSION_GROUP_ID);
        fapi2::ATTR_FABRIC_ADDR_EXTENSION_CHIP_ID_Type l_TGT1_ATTR_FABRIC_ADDR_EXTENSION_CHIP_ID;
        FAPI_ATTR_GET(fapi2::ATTR_FABRIC_ADDR_EXTENSION_CHIP_ID, TGT1, l_TGT1_ATTR_FABRIC_ADDR_EXTENSION_CHIP_ID);
        fapi2::ATTR_SMF_CONFIG_Type l_TGT1_ATTR_SMF_CONFIG;
        FAPI_ATTR_GET(fapi2::ATTR_SMF_CONFIG, TGT1, l_TGT1_ATTR_SMF_CONFIG);
        fapi2::buffer<uint64_t> l_scom_buffer;
        // l_NX_DMA_CH3_SYM_ENABLE_ON
        // l_NX_DMA_CH2_SYM_ENABLE_ON
        // l_NX_DMA_CH4_GZIP_ENABLE_ON
        // l_NX_DMA_CH1_EFT_ENABLE_ON
        // l_NX_DMA_CH0_EFT_ENABLE_ON
        scom_or(0x2011041, PPC_BITMASK(57, 58) | PPC_BITMASK(61, 63));
        // l_NX_DMA_GZIPCOMP_MAX_INRD_MAX_15_INRD
        // l_NX_DMA_GZIP_COMP_PREFETCH_ENABLE_ON
        // l_NX_DMA_GZIP_DECOMP_PREFETCH_ENABLE_ON
        // l_NX_DMA_EFT_COMP_PREFETCH_ENABLE_ON
        // l_NX_DMA_EFT_DECOMP_PREFETCH_ENABLE_ON
        // l_NX_DMA_SYM_MAX_INRD_MAX_3_INRD
        // l_NX_DMA_EFTCOMP_MAX_INRD_MAX_15_INRD
        // l_NX_DMA_EFTDECOMP_MAX_INRD_MAX_15_INRD
        // l_NX_DMA_EFT_SPBC_WRITE_ENABLE_OFF
        scom_or(0x2011042, PPC_BITMASK(8, 11) | PPC_BITMASK(16, 17) | PPC_BITMASK(23, 25) | PPC_BIT(33) | PPC_BIT(37) | PPC_BIT(56))
        fapi2::getScom(TGT0, 0x201105C, l_scom_buffer);
        l_scom_buffer.insert<0, 1, 63, uint64_t>(1); // l_NX_DMA_CH0_WATCHDOG_TIMER_ENBL_ON
        l_scom_buffer.insert<1, 4, 60, uint64_t>(0x9); // l_NX_DMA_CH0_WATCHDOG_REF_DIV_DIVIDE_BY_512
        l_scom_buffer.insert<5, 1, 63, uint64_t>(1); // l_NX_DMA_CH1_WATCHDOG_TIMER_ENBL_ON
        l_scom_buffer.insert<6, 4, 60, uint64_t>(0x9); // l_NX_DMA_CH1_WATCHDOG_REF_DIV_DIVIDE_BY_512
        l_scom_buffer.insert<10, 1, 63, uint64_t>(1); // l_NX_DMA_CH2_WATCHDOG_TIMER_ENBL_ON
        l_scom_buffer.insert<11, 4, 60, uint64_t>(0x9); // l_NX_DMA_CH2_WATCHDOG_REF_DIV_DIVIDE_BY_512
        l_scom_buffer.insert<15, 1, 63, uint64_t>(1); // l_NX_DMA_CH3_WATCHDOG_TIMER_ENBL_ON
        l_scom_buffer.insert<16, 4, 60, uint64_t>(0x9); // l_NX_DMA_CH3_WATCHDOG_REF_DIV_DIVIDE_BY_512
        l_scom_buffer.insert<20, 1, 63, uint64_t>(1); // l_NX_DMA_CH4_WATCHDOG_TIMER_ENBL_ON
        l_scom_buffer.insert<21, 4, 60, uint64_t>(0x9); // l_NX_DMA_CH4_WATCHDOG_REF_DIV_DIVIDE_BY_512
        l_scom_buffer.insert<25, 1, 63, uint64_t>(1); // l_NX_DMA_DMA_HANG_TIMER_ENBL_ON
        l_scom_buffer.insert<26, 4, 60, uint64_t>(0x8); // l_NX_DMA_DMA_HANG_TIMER_REF_DIV_DIVIDE_BY_1024
        fapi2::putScom(TGT0, 0x201105C, l_scom_buffer);
        fapi2::getScom(TGT0, 0x2011083, l_scom_buffer);
        l_scom_buffer.insert<0, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<1, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<2, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<3, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<4, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<5, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<6, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<7, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<8, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<9, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<10, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<11, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<12, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<13, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<14, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<15, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<16, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<17, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<18, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<19, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<20, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<21, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<22, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<23, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<24, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<25, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<26, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<27, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<28, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<29, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<30, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<31, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<32, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<33, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<34, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<35, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<36, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<37, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<38, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<39, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<40, 2, 62, uint64_t>(3);
        l_scom_buffer.insert<42, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<43, 1, 63, uint64_t>(1);
        fapi2::putScom(TGT0, 0x2011083, l_scom_buffer);
        fapi2::getScom(TGT0, 0x2011086, l_scom_buffer);
        l_scom_buffer.insert<0, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<1, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<2, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<3, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<4, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<5, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<6, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<7, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<8, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<9, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<10, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<11, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<12, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<13, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<14, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<15, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<16, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<17, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<18, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<19, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<20, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<21, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<22, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<23, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<24, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<25, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<26, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<27, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<28, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<29, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<30, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<31, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<32, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<33, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<34, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<35, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<36, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<37, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<38, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<39, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<40, 2, 62, uint64_t>(0);
        l_scom_buffer.insert<42, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<43, 1, 63, uint64_t>(0);
        fapi2::putScom(TGT0, 0x2011086, l_scom_buffer);
        fapi2::getScom(TGT0, 0x2011087, l_scom_buffer);
        l_scom_buffer.insert<0, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<1, 1, 63, uint64_t>(1);
        if(l_chip_id == 0x5 && l_chip_ec == 0x20)
        {
            l_scom_buffer.insert<2, 1, 63, uint64_t>(0);
        }
        else
        {
            l_scom_buffer.insert<2, 1, 63, uint64_t>(1);
        }
        l_scom_buffer.insert<3, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<4, 1, 63, uint64_t>(1);
        if(l_chip_id == 0x5 && l_chip_ec == 0x20)
        {
            l_scom_buffer.insert<5, 1, 63, uint64_t>(1);
        }
        else
        {
            l_scom_buffer.insert<5, 1, 63, uint64_t>(0);
        }
        l_scom_buffer.insert<6, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<7, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<8, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<9, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<10, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<11, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<12, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<13, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<14, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<15, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<16, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<17, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<18, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<19, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<20, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<21, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<22, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<23, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<24, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<25, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<26, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<27, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<28, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<29, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<30, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<31, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<32, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<33, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<34, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<35, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<36, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<37, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<38, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<39, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<40, 2, 62, uint64_t>(0);
        l_scom_buffer.insert<42, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<43, 1, 63, uint64_t>(0);
        fapi2::putScom(TGT0, 0x2011087, l_scom_buffer);
        fapi2::getScom(TGT0, 0x2011095, l_scom_buffer);
        l_scom_buffer.insert<24, 1, 63, uint64_t>(1); // l_NX_PBI_CQ_WRAP_NXCQ_SCOM_SKIP_G_ON
        l_scom_buffer.insert<56, 4, 60, uint64_t>(l_TGT1_ATTR_FABRIC_ADDR_EXTENSION_GROUP_ID);
        l_scom_buffer.insert<60, 3, 61, uint64_t>(l_TGT1_ATTR_FABRIC_ADDR_EXTENSION_CHIP_ID);
        if((l_chip_id == 0x5 && l_chip_ec == 0x22)
        || (l_chip_id == 0x5 && l_chip_ec == 0x23)
        {
            if(l_TGT1_ATTR_SMF_CONFIG == fapi2::ENUM_ATTR_SMF_CONFIG_ENABLED)
            {
                l_scom_buffer.insert<34, 2, 62, uint64_t>(2);
            }
        }
        l_scom_buffer.insert<1, 1, 63, uint64_t>(1); // l_NX_PBI_CQ_WRAP_NXCQ_SCOM_DMA_WR_DISABLE_GROUP_ON
        l_scom_buffer.insert<5, 1, 63, uint64_t>(1); // l_NX_PBI_CQ_WRAP_NXCQ_SCOM_DMA_RD_DISABLE_GROUP_ON
        l_scom_buffer.insert<9, 1, 63, uint64_t>(1); // l_NX_PBI_CQ_WRAP_NXCQ_SCOM_UMAC_WR_DISABLE_GROUP_ON
        l_scom_buffer.insert<13, 1, 63, uint64_t>(1); // l_NX_PBI_CQ_WRAP_NXCQ_SCOM_UMAC_RD_DISABLE_GROUP_ON
        l_scom_buffer.insert<2, 1, 63, uint64_t>(1); // l_NX_PBI_CQ_WRAP_NXCQ_SCOM_DMA_WR_DISABLE_VG_NOT_SYS_ON
        l_scom_buffer.insert<6, 1, 63, uint64_t>(1); // l_NX_PBI_CQ_WRAP_NXCQ_SCOM_DMA_RD_DISABLE_VG_NOT_SYS_ON
        l_scom_buffer.insert<10, 1, 63, uint64_t>(1); // l_NX_PBI_CQ_WRAP_NXCQ_SCOM_UMAC_WR_DISABLE_VG_NOT_SYS_ON
        l_scom_buffer.insert<14, 1, 63, uint64_t>(1); // l_NX_PBI_CQ_WRAP_NXCQ_SCOM_UMAC_RD_DISABLE_VG_NOT_SYS_ON
        l_scom_buffer.insert<22, 1, 63, uint64_t>(1); // l_NX_PBI_CQ_WRAP_NXCQ_SCOM_RD_GO_M_QOS_ON
        l_scom_buffer.insert<23, 1, 63, uint64_t>(0); // l_NX_PBI_CQ_WRAP_NXCQ_SCOM_ADDR_BAR_MODE_OFF
        l_scom_buffer.insert<25, 2, 62, uint64_t>(1);
        l_scom_buffer.insert<40, 8, 56, uint64_t>(0xFC);
        l_scom_buffer.insert<48, 8, 56, uint64_t>(0xFC);
        fapi2::putScom(TGT0, 0x2011095, l_scom_buffer);
        fapi2::getScom(TGT0, 0x20110A8, l_scom_buffer);
        l_scom_buffer.insert<4, 4, 60, uint64_t>(8);
        l_scom_buffer.insert<8, 4, 60, uint64_t>(8);
        l_scom_buffer.insert<12, 4, 60, uint64_t>(8);
        l_scom_buffer.insert<16, 4, 60, uint64_t>(8);
        fapi2::putScom(TGT0, 0x20110A8, l_scom_buffer);
        fapi2::getScom(TGT0, 0x20110C3, l_scom_buffer);
        l_scom_buffer.insert<27, 9, 55, uint64_t>(8);
        fapi2::putScom(TGT0, 0x20110C3, l_scom_buffer);
        fapi2::getScom(TGT0, 0x20110C4, l_scom_buffer);
        l_scom_buffer.insert<27, 9, 55, uint64_t>(8);
        fapi2::putScom(TGT0, 0x20110C4, l_scom_buffer);
        fapi2::getScom(TGT0, 0x20110C5, l_scom_buffer);
        l_scom_buffer.insert<27, 9, 55, uint64_t>(8);
        fapi2::putScom(TGT0, 0x20110C5, l_scom_buffer);
        fapi2::getScom(TGT0, 0x20110D5, l_scom_buffer);
        l_scom_buffer.insert<1, 1, 63, uint64_t>(1); // l_NX_PBI_PBI_UMAC_CRB_READS_ENBL_ON
        fapi2::putScom(TGT0, 0x20110D5, l_scom_buffer);
        fapi2::getScom(TGT0, 0x20110D6, l_scom_buffer);
        l_scom_buffer.insert<6, 1, 63, uint64_t>(1); // l_NX_PBI_DISABLE_PROMOTE_ON
        l_scom_buffer.insert<9, 3, 61, uint64_t>(2);
        fapi2::putScom(TGT0, 0x20110D6, l_scom_buffer);
        fapi2::getScom(TGT0, 0x2011103, l_scom_buffer);
        l_scom_buffer.insert<0, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<1, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<2, 2, 62, uint64_t>(3);
        l_scom_buffer.insert<4, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<5, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<6, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<7, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<8, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<9, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<10, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<11, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<12, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<13, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<14, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<15, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<16, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<17, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<18, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<19, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<20, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<21, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<22, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<23, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<24, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<25, 6, 58, uint64_t>(0x3F);
        l_scom_buffer.insert<31, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<32, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<33, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<34, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<35, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<36, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<37, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<38, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<39, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<40, 8, 56, uint64_t>(0xFF);
        l_scom_buffer.insert<48, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<49, 1, 63, uint64_t>(1);
        fapi2::putScom(TGT0, 0x2011103, l_scom_buffer);
        fapi2::getScom(TGT0, 0x2011106, l_scom_buffer);
        l_scom_buffer.insert<0, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<1, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<2, 2, 62, uint64_t>(0);
        l_scom_buffer.insert<4, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<5, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<6, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<7, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<8, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<9, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<10, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<11, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<12, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<13, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<14, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<15, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<16, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<17, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<18, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<19, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<20, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<21, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<22, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<23, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<24, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<25, 6, 58, uint64_t>(0);
        l_scom_buffer.insert<31, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<32, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<33, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<34, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<35, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<36, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<37, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<38, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<39, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<40, 8, 56, uint64_t>(0);
        l_scom_buffer.insert<48, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<49, 1, 63, uint64_t>(0);
        fapi2::putScom(TGT0, 0x2011106, l_scom_buffer);
        fapi2::getScom(TGT0, 0x2011107, l_scom_buffer);
        l_scom_buffer.insert<0, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<1, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<2, 2, 62, uint64_t>(0);
        l_scom_buffer.insert<4, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<6, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<8, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<9, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<10, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<11, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<13, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<14, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<15, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<16, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<19, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<20, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<21, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<22, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<23, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<24, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<25, 6, 58, uint64_t>(0);
        l_scom_buffer.insert<31, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<32, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<33, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<34, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<35, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<36, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<37, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<38, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<39, 1, 63, uint64_t>(1);
        l_scom_buffer.insert<40, 8, 56, uint64_t>(0);
        l_scom_buffer.insert<48, 1, 63, uint64_t>(0);
        l_scom_buffer.insert<49, 1, 63, uint64_t>(0);
        if(l_chip_id == 0x5 && l_chip_ec == 0x20)
        {
            l_scom_buffer.insert<5, 1, 63, uint64_t>(0);
            l_scom_buffer.insert<7, 1, 63, uint64_t>(0);
            l_scom_buffer.insert<12, 1, 63, uint64_t>(0);
            l_scom_buffer.insert<17, 1, 63, uint64_t>(0);
            l_scom_buffer.insert<18, 1, 63, uint64_t>(0);
        }
        fapi2::putScom(TGT0, 0x2011107, l_scom_buffer);
    };
}
