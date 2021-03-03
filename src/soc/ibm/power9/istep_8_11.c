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

    l_obus_chiplets = i_target.getChildren<fapi2::TARGET_TYPE_OBUS>();
    l_mcs_targets = i_target.getChildren<fapi2::TARGET_TYPE_MCS>();
    l_mi_targets = i_target.getChildren<fapi2::TARGET_TYPE_MI>();
    l_dmi_targets = i_target.getChildren<fapi2::TARGET_TYPE_DMI>();
    l_mc_targets = i_target.getChildren<fapi2::TARGET_TYPE_MC>();

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
    p9_nx_scom();
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

void p9_cxa_scom(void)
{
    // CXA FIR Mask Register
    // Error mask register.
    // (Action0, mask) = Action select
    // (0, 0) = Recoverable error
    // (0, 1) = Masked
    // (1, x) = Checkstop error
    // [0] BAR_PE_MASK: Informational PE Mask. Generic bucket for errors that
    //     are informational.
    // [1] REGISTER_PE_MASK: System checkstop PE mask. Generic bucket for errors
    //     that are system checkstop.
    // [2] MASTER_ARRAY_CE_MASK: Correctable error on master array. Includes uOP
    //     and CRESP arrays Mask.
    // [3] MASTER_ARRAY_UE_MASK: Uncorrectable error on master array. Includes
    //     uOP and CRESP arrays Mask.
    // [4] TIMER_EXPIRED_RECOV_ERROR_MASK: Precise directory epoch timeout Mask.
    //     Coarse directory epoch timeout mask. rtagPool epoxh data hang timeout
    //     mask. Recovery sequencer hang detection mask.
    // [5] TIMER_EXPIRED_XSTOP_ERROR_MASK: Recovery sequencer hang detection
    //     mask.
    // [6] PSL_CMD_UE_MASK: XPT detected uncorrectable error on processor bus
    //     data determined to be a PSL command Mask. The PSL command does not
    //     propagate past XPT.
    // [7] PSL_CMD_SUE_MASK: XPT detected Special uncorrectable error on
    //     processor bus data determined to be a PSL command mask. The PSL
    //     command does not propagate past XPT.
    // [8] SNOOP_ARRAY_CE_MASK: Correctable error on Snooper array. Includes
    //     precise directory, coarse directory, and uOP mask.
    // [9] SNOOP_ARRAY_UE_MASK: Uncorrectable error on Snooper array. Includes
    //     precise directory, coarse directory, and uOP Mask.
    // [10] RECOVERY_FAILED_MASK: CAPP recovery failed mask.
    // [11] ILLEGAL_LPC_BAR_ACCESS_MASK: Error detected when a snooper or master
    //      write operation is attempted to the fine directories in LPC-only
    //      mode, or when the operation hits the LPC BAR region in LPC disjoint
    //      mode. Also, activates if a directory hit is found on a read
    //      operation within the LPC BAR range.
    // [12] XPT_RECOVERABLE_ERROR_MASK: Recoverable errors detected in xpt mask.
    // [13] MASTER_RECOVERABLE_ERROR_MASK: Recoverable errors detected in Master
    //      mask.
    // [14] SNOOPER_RECOVERABLE_ERROR_MASK: Spare FIR bits allocated for future
    //      use mask.
    // [15] SECURE_SCOM_ERROR_MASK: Error detected in secure SCOM satellite
    //      Mask.
    // [16] MASTER_SYS_XSTOP_ERROR_MASK: System checkstop errors detected in
    //      Master (invalid state, control checker) mask.
    // [17] SNOOPER_SYS_XSTOP_ERROR_MASK: System checkstop errors detected in
    //      Snooper (invalid state, control checker) mask.
    // [18] XPT_SYS_XSTOP_ERROR_MASK: System checkstop errors detected in
    //      Transport (invalid state, control checker) Mask.
    // [19] MUOP_ERROR_1_MASK: Master uOP FIR 1 mask.
    // [20] MUOP_ERROR_2_MASK: Master uOP FIR 2 mask.
    // [21] MUOP_ERROR_3_MASK: Master uOP FIR 3 mask.
    // [22] SUOP_ERROR_1_MASK: Snooper uOP FIR 1 mask.
    // [23] SUOP_ERROR_2_MASK: Snooper uOP FIR 2 mask.
    // [24] SUOP_ERROR_3_MASK: Snooper uOP FIR 3 mask.
    // [25] POWERBUS_MISC_ERROR_MASK: Miscellaneous informational processor bus
    //      errors mask including unsolicited processor bus data and unsolicited
    //      cResp.
    // [26] POWERBUS_INTERFACE_PE_MASK: Parity error on processor bus interface
    //      (address/aTag/tTag/rTag APC, SNP TLBI) mask.
    // [27] POWERBUS_DATA_HANG_ERROR_MASK: Any processor bus data hang poll
    //      error mask.
    // [28] POWERBUS_HANG_ERROR_MASK: Any processor bus command hang error
    //      (domestic address range) mask.
    // [29] LD_CLASS_CMD_ADDR_ERR_MASK: processor bus address error detected by
    //      APC on a load-class command mask.
    // [30] ST_CLASS_CMD_ADDR_ERR_MASK: Processor bus address error detected by
    //      APC on a store-class command mask.
    // [31] PHB_LINK_DOWN_MASK: PPHB0 or PHB1 interface has asserted linkdown
    //      Mask.
    // [32] LD_CLASS_CMD_FOREIGN_LINK_FAIL_MASK: APC received ack_dead or
    //      ack_ed_dead from the foreign interface on a load-class command mask.
    // [33] FOREIGN_LINK_HANG_ERROR_MASK: Any processor bus command hang error
    //      (foreign address range) mask.
    // [34] XPT_POWERBUS_CE_MASK: CE on data received from processor bus and
    //      destined for either XPT data array (and back to processor bus) or
    //      PSL command or link delay response packet mask.
    // [35] XPT_POWERBUS_UE_MASK: UE on data received from processor bus and
    //      destined for XPT data array (and back to processor bus) or link
    //      delay response packet mask.
    // [36] XPT_POWERBUS_SUE_MASK: SUE on data received from processor bus and
    //      destined for XPT data array (and back to processor bus) or link
    //      delay response packet mask.
    // [37] TLBI_TIMEOUT_MASK: TLBI Timeout error. TLBI requires re-IPL to
    //      recover mask.
    // [38] TLBI_SOT_ERR_MASK: Illegal SOT op detected in POWER8 BC mode.
    //      TLBI requires re-IPL to recover mask.
    // [39] TLBI_BAD_OP_ERR_MASK: TLBI bad op error. TLBI requires re-IPL to
    //      recover mask.
    // [40] TLBI_SEQ_NUM_PARITY_ERR_MASK: Parity error detected on TLBI sequence
    //      number mask.
    // [41] ST_CLASS_CMD_FOREIGN_LINK_FAIL_MASK: APC received ack_dead or
    //      ack_ed_dead from the foreign interface on a store-class command
    //      mask.
    // [42] TIME_BASE_ERR_MASK: An error has occurred with timebase. This is an
    //      indication that the time-base value can no longer be assumed to be
    //      correct.
    // [43] TRANSPORT_INFORMATIONAL_ERR_MASK: Transport informational error
    //      mask.
    // [44] APC_ARRAY_CMD_CE_ERPT_MASK: CE on PSL command queue array in APC
    //      mask.
    // [45] APC_ARRAY_CMD_UE_ERPT_MASK: UE on PSL command queue array in APC
    //      mask.
    // [46] PSL_CREDIT_TIMEOUT_ERR_MASK: PSL credit timeout error mask.
    // [47] SPARE_2_MASK: Spare FIR bits allocated for future use mask.
    // [48] SPARE_3_MASK: Spare FIR bits allocated for future use mask.
    // [49] HYPERVISOR_MASK: Hypervisor mask.
    // [50] SCOM_ERR2_MASK: Local FIR parity error RAS duplicate mask.
    // [51] SCOM_ERR_MASK: Local FIR parity error of ACTION/MASK registers mask.
    // [52:63] constant = 0b000000000000
    if(CHIP_EC == 0x20)
    {
        scom_and_or(0x2010803, ~PPC_BITMASK(0, 53), 0x801B1F98C8717000)
    }
    else
    {
        scom_and_or(0x2010803, ~PPC_BITMASK(0, 53), 0x801B1F98D8717000)
    }
    // Pervasive FIR Action 0 Register
    // FIR_ACTION0: MSB of action select for corresponding bit in FIR.
    // (Action0,Action1) = Action Select.
    // (0, 0) = Checkstop.
    // (0, 1) = Recoverable.
    // (1, 0) = Recoverable interrupt.
    // (1, 1) = Machine check.
    scom_and(0x2010806, ~PPC_BITMASK(0, 53));
    // Pervasive FIR Action 1 Register
    // FIR_ACTION1: LSB of action select for corresponding bit in FIR.
    // (Action0, Action1, Mask) = Action select.
    // (x, x, 1) = Masked.
    // (0, 0, 0) = Checkstop.
    // (0, 1, 0) = Recoverable Error.
    // (1, 0, 0) = Recoverable Interrupt.
    // (1, 1, 0) = Machine Check.
    scom_or(0x2010807, PPC_BIT(2) | PPC_BIT(8) | PPC_BIT(34) | PPC_BIT(44));
    // APC Master Processor Bus Control Register
    // [1] APCCTL_ADR_BAR_MODE: addr_bar_mode. Set to 1 when chip equals group.
    //     Small system address map. Reduces the number of group ID bits to two
    //     and eliminates the chip ID bits. All chips have an ID of 0. Nn scope
    //     is not available in this mode.
    // [3] APCCTL_DISABLE_VG_NOT_SYS: disable_vg_not_sys scope.
    // [4] APCCTL_DISABLE_G: disable_group scope.
    // [6] APCCTL_SKIP_G: Skip increment to group scope.
    //     Only used by read machines.
    // [19:38] constant = 0b00000000000000000000
    //         WARNING: These are read-only bits,
    //         but they are written to by hsotboot
    if(CHIP_EC >= 0x22 && SMF_CONFIG)
    {
        l_scom_buffer.insert<19, 2, 62, uint64_t>(2);
        scom_and_or(0x2010818, ~(PPC_BIT(1) | PPC_BIT(20) | PPC_BIT(25)), PPC_BITMASK(3, 4) | PPC_BIT(6) | PPC_BIT(19) | PPC_BIT(21));
    }
    else
    {
        scom_and_or(0x2010818, ~(PPC_BIT(1) | PPC_BIT(25)), PPC_BITMASK(3, 4) | PPC_BIT(6) | PPC_BIT(21));
    }
    // APC Master Configuration Register
    // [4:7] HANG_POLL_SCALE: Determines the number of hang polls that must
    //       be detected to indicate a hang poll to the logic.
    scom_and(0x2010819, ~PPC_BITMASK(4, 7));
    // Snoop Control Register
    // [45:47] CXA_SNP_ADDRESS_PIPELINE_MASTERWAIT_COUNT: Maximum number of
    //         cycles an APC master can wait before swapping the arbitration
    //         priority between APC and the snooper.
    // [48:51] CXA_SNP_DATA_HANG_POLL_SCALE: Number of dhang_polls it takes to
    //         increment the DHANG counter in snoop. Actual count is scale + 1.
    //         Value of 0000 = decimal 1 counts.
    scom_and_or(
        0x201081B,
        ~PPC_BITMASK(48, 51),
        PPC_BITMASK(45, 47) | PPC_BIT(50));
    // Transport Control Register
    // [18:20] TLBI_DATA_POLL_PULSE_DIV: Number of data polls received before
    //         signaling TLBI hang-detection timer has expired.
    scom_and_or(0x201081C, ~PPC_BITMASK(18, 20), PPC_BIT(21));
}

void p9_nx_scom()
{
    // DMA Engine Enable Register
    // [57] CH3_SYM_ENABLE: Enables channel 3 SYM engine.
    //      Cannot be set when allow_crypto = 0.
    // [58] CH2_SYM_ENABLE: Enables channel 2 SYM engine.
    //      Cannot be set when allow_crypto = 0.
    // [61] CH4_GZIP_ENABLE: Enables channel 4 GZIP engine.
    // [62] CH1_EFT_ENABLE: Enables channel 1 842 engine.
    // [63] CH0_EFT_ENABLE: Enables channel 0 842 engine.
    scom_or(0x2011041, PPC_BITMASK(57, 58) | PPC_BITMASK(61, 63));
    // DMA Configuration Register
    // [8:11] GZIPCOMP_MAX_INRD: Maximum number of outstanding inbound PBI read
    //        requests per channel for GZIP compression
    // [16] GZIP_COMP_PREFETCH_ENABLE: Enable for compression read prefetch
    //      hint for GZIP.
    // [17] GZIP_DECOMP_PREFETCH_ENABLE: Enable for decompression read prefetch
    //      hint for GZIP.
    // [23] EFT_COMP_PREFETCH_ENABLE: Enable for compression read prefetch hint
    //      for 842.
    // [24] EFT_DECOMP_PREFETCH_ENABLE: Enable for decompression read prefetch
    //      hint for 842.
    // [25:28] SYM_MAX_INRD: Maximum number of outstanding inbound PBI read
    //         requests per symmetric channel.
    // [33:36] EFTCOMP_MAX_INRD: Maximum number of outstanding inbound PBI read
    //         requests per channel for 842 compression/bypass.
    // [37:40] EFTDECOMP_MAX_INRD: Maximum number of outstanding inbound PBI
    //         read requests per channel for 842 decompression.
    // [56] EFT_SPBC_WRITE_ENABLE: Enable SPBC write for 842.
    scom_or(
        0x2011042,
        PPC_BITMASK(8, 11) | PPC_BITMASK(16, 17) | PPC_BITMASK(23, 25)
        | PPC_BIT(33)        | PPC_BIT(37)         | PPC_BIT(56));
    // [0] CH0_WATCHDOG_TIMER_ENBL: Channel 0 watchdog timer is enabled.
    // [1:4] CH0_WATCHDOG_REF_DIV: This field allows the watchdog timer reference oscillator to be divided by powers of two.
    // [5] CH1_WATCHDOG_TIMER_ENBL: Channel 1 watchdog timer is enabled.
    // [6:9] CH1_WATCHDOG_REF_DIV: This field allows the watchdog timer reference oscillator to be divided by powers of two.
    // [10] CH2_WATCHDOG_TIMER_ENBL: Channel 2 watchdog timer is enabled.
    // [11:14] CH2_WATCHDOG_REF_DIV: This field allows the watchdog timer reference oscillator to be divided by powers of two.
    // [15] CH3_WATCHDOG_TIMER_ENBL: Channel 3 Watchdog Timer is enabled.
    // [16:19] CH3_WATCHDOG_REF_DIV: This field allows the watchdog timer reference oscillator to be divided by powers of two.
    // [20] CH4_WATCHDOG_TIMER_ENBL: Channel 4 watchdog timer is enabled.
    // [21:24] CH4_WATCHDOG_REF_DIV: This field allows the watchdog timer reference oscillator to be divided by powers of two.
    // [25] DMA_HANG_TIMER_ENBL: DMA hang timer is enabled.
    // [26:29] DMA_HANG_TIMER_REF_DIV: This field allows the DMA hang timer reference oscillator to be divided by powers of two.
    scom_and_or(
        0x201105C,
        ~PPC_BITMASK(0, 29),
        PPC_BITMASK(0, 1)   | PPC_BITMASK(4, 6)   | PPC_BITMASK(9, 11)
        | PPC_BITMASK(14, 16) | PPC_BITMASK(19, 21) | PPC_BITMASK(24, 26));
    // PBI CQ FIR Mask Register
    // [0:43] NX_CQ_FIR_MASK:
    scom_and_or(
        0x2011083,
        ~PPC_BITMASK(0, 43),
        PPC_BIT(3) | PPC_BIT(7) | PPC_BITMASK(13, 15) | PPC_BITMASK(25, 26)
        | PPC_BITMASK(30, 31)     | PPC_BIT(38)         | PPC_BITMASK(41, 43));
    // PBI CQ FIR Action0 Register
    // [0:43] This register is an action select for the corresponding bit in FIR.
    // (Action0, Action1) = Action select
    // (0, 0) = Checkstop
    // (0, 1) = Recoverable
    // (1, 0) = Unused
    // (1, 1) = Local checkstop
    scom_and(0x2011086, ~PPC_BITMASK(0, 43))
    // PBI CQ FIR Action1 Register
    // This register is an action select for the corresponding bit in FIR.
    // (Action0, Action1) = Action select
    // (0, 0) = Checkstop
    // (0, 1) = Recoverable
    // (1, 0) = Unused
    // (1, 1) = Local checkstop
    if(CHIP_EC == 0x20)
    {
        scom_and_or(0x2011087,
        ~PPC_BITMASK(0, 43),
        PPC_BIT(1)  | PPC_BITMASK(4, 5) | PPC_BIT(11)
        | PPC_BIT(18) | PPC_BITMASK(32, 33))
    }
    else
    {
        scom_and_or(
            0x2011087,
            ~PPC_BITMASK(0, 43),
            PPC_BITMASK(1, 2) | PPC_BIT(4) | PPC_BIT(11)
            | PPC_BIT(18)       | PPC_BITMASK(32, 33))
    }
    // Processor Bus MODE Register
    // [1] DMA_WR_DISABLE_GROUP: For DMA write machine requests:
    //     0 = Master can request G scope
    //     1 = Master shall not request G scope
    // [2] DMA_WR_DISABLE_VG_NOT_SYS: For DMA write machine requests:
    //     0 = Master can request Vg scope
    //     1 = Master shall not request Vg scope, but can use Vg (SYS).
    // [3] DMA_WR_DISABLE_NN_RN: For DMA write machine requests:
    //     0 = Master can request Nn or Rn scope
    //     1 = Master shall not request Nn or Rn scope
    // [5] DMA_RD_DISABLE_GROUP: For DMA read machine requests:
    //     0 = Master can request G scope
    //     1 = Master shall not request G scope
    // [6] DMA_RD_DISABLE_VG_NOT_SYS: For DMA read machine requests:
    //     0 = Master can request Vg scope
    //     1 = Master shall not request Vg scope, but can use Vg (SYS)
    // [9] UMAC_WR_DISABLE_GROUP: For UMAC write machine requests:
    //     0 = Master can request G scope
    //     1 = Master shall not request G scope
    // [10] UMAC_WR_DISABLE_VG_NOT_SYS: For UMAC write machine requests:
    //      0 = Master can request Vg scope
    //      1 = Master shall not request Vg scope, but can use Vg (SYS)
    // [13] UMAC_RD_DISABLE_GROUP: For UMAC read machine requests:
    //      0 = Master can request G scope
    //      1 = Master shall not request G scope
    // [14] UMAC_RD_DISABLE_VG_NOT_SYS: For UMAC read machine requests:
    //      0 = Master can request Vg scope
    //      1 = Master shall not request Vg scope, but can use Vg (SYS)
    // [22] RD_GO_M_QOS: Controls the setting of the quality of service (q) bit
    //      in the secondary encode of the rd_go_m command. This command is only
    //      issued as the read (R) of the WRP protocol. If this bit = 1,
    //      the q bit = 1. This means high quality of service, that is,
    //      the memory controller must treat this as a high-priority request.
    // [23] ADDR_BAR_MODE: Specifies the address mapping mode in use
    //      for the system.
    //      0 = Small system address map. Reduces the number of group ID bits to
    //          2 and eliminates the chip ID bits. All chips have an ID of 0.
    //          Nn scope is not available in this mode.
    //      1 = Large system address map. Uses 4 bits for the group ID and
    //          3 bits for the chip ID.
    // [24] SKIP_G: Scope mode control set by firmware when the topology
    //      is chip = group.
    //      Note: This CS does not disable the use of group scope.
    //      It modifies the progression of scope when the
    //      command starts at nodal scope.
    //      0 = The progression from nodal to group scope is followed when the
    //          combined response indicates rty_inc or sfStat (as applicable to
    //          the command) is set.
    //      1 = When the scope of the command is nodal and the command is in the
    //          Read, RWITM, or is an atomic RMW and fetch command (found in the
    //          Ack_BK group), and the combined response is rty_inc or the
    //          sfStat is set in the data, the command scope progression skips
    //          group and goes to Vg scope.
    // [25:26] DATA_ARB_LFSR_CONFIG: Data arbitration priority percentage:
    //         00 = Choose XLATE 50% of the time
    //         01 = Choose XLATE 75% of the time
    //         10 = Choose XLATE 88% of the time
    //         11 = Choose XLATE 100% of the time
    // [34:39] Unused
    //         WARNING: According to documentation, these bits are unused,
    //         but hostboot is writing to it.
    // [40:47] DMA_RD_VG_RESET_TIMER_MASK: Mask for timer to reset read Vg scope predictor:
    //         FF = 64 K cycles
    //         FE = 32 K cycles
    //         FC = 16 K cycles
    //         F8 = 8 K cycles
    //         80 = 512 cycles.
    // [48:55] DMA_WR_VG_RESET_TIMER_MASK: Mask for timer to reset write Vg scope predictor:
    //         FF = 64 K cycles
    //         FE = 32 K cycles
    //         FC = 16 K cycles
    //         F8 = 8 K cycles
    //         80 = 512 cycles
    // [56:63] constant = 0b00000000
    //         WARNING: according to documentation, bits [56:63] are read-only,
    //         but hostboot is writing to it. It's better to assume, that we can
    //         indeed write to it.
    if(CHIP_EC != 0x20 && SMF_CONFIG)
    {
        scom_and_or(
            0x2011095,
            PPC_BITMASK(1, 3)   | PPC_BITMASK(5, 6)   | PPC_BITMASK(9, 10)
          | PPC_BITMASK(13, 14) | PPC_BITMASK(22, 26) | PPC_BITMASK(34, 35)
          | PPC_BITMASK(40, 62),
            PPC_BIT(1)          | PPC_BIT(3)          | PPC_BITMASK(5, 6)
          | PPC_BITMASK(9, 10)  | PPC_BITMASK(13, 14) | PPC_BIT(22)
          | PPC_BITMASK(24, 25) | PPC_BIT(34)         | PPC_BITMASK(40, 45)
          | PPC_BITMASK(48, 53) | PPC_BIT(59));
    }
    else
    {
        scom_and_or(
            0x2011095,
            PPC_BITMASK(1, 3)   | PPC_BITMASK(5, 6)   | PPC_BITMASK(9, 10)
          | PPC_BITMASK(13, 14) | PPC_BITMASK(22, 26) | PPC_BITMASK(40, 62),
            PPC_BIT(1)          | PPC_BIT(3)          | PPC_BITMASK(5, 6)
          | PPC_BITMASK(9, 10)  | PPC_BITMASK(13, 14) | PPC_BIT(22)
          | PPC_BITMASK(24, 25) | PPC_BITMASK(40, 45) | PPC_BITMASK(48, 53)
          | PPC_BIT(59));
    }
    // NX Miscellaneous Control Register
    // [4:7] HANG_POLL_SCALE: Determines how many hang polls must be detected to
    //       indicate a hang poll to the logic.
    // [8:11] HANG_DATA_SCALE: Determines how many data polls must be detected
    //        to indicate a data poll to the logic.
    // [12:15] HANG_SHM_SCALE: Determines how many data polls must be detected to indicate a shim poll to the logic. A shim poll is created by the CQ logic to detect hangs of SMs while waiting on exchanges with the shim logic.
    // [16:19] ERAT_DATA_POLL_SCALE: Determines how many data polls must be detected to indicate a data poll to the ERAT logic.
    scom_and_or(
        0x20110A8,
        ~PPC_BITMASK(4, 19),
        PPC_BIT(4) | PPC_BIT(8) | PPC_BIT(12) | PPC_BIT(16));
    // UMAC EFT High-Priority Receive FIFO Control Register
    // [27:35] EFT_HI_PRIORITY_RCV_FIFO_HI_PRIMAX: EFT high-priority receive
    //         FIFO maximum CRBs.
    scom_and_or(0x20110C3, ~PPC_BITMASK(27, 35), PPC_BIT(32));
    // UMAC SYM High-Priority Receive FIFO Control Register
    // [27:35] SYM_HI_PRIORITY_RCV_FIFO_HI_PRIMAX: SYM high-priority receive
    //         FIFO maximum CRBs.
    scom_and_or(0x20110C4, ~PPC_BITMASK(27, 35), PPC_BIT(32));
    // UMAC GZIP High-Priority Receive FIFO Control Register
    // [27:35] GZIP_HI_PRIORITY_RCV_FIFO_HI_PRIMAX: GZIP high-priority receive
    //         FIFO maximum CRBs.
    scom_and_or(0x20110C5, ~PPC_BITMASK(27, 35), PPC_BIT(32));
    // UMAC Status And Control Register
    // [1] CRB_READS_ENBL: Enables CRB reads.
    scom_or(0x20110D5, PPC_BIT(1));
    // ERAT Status and Control Register
    // [6] DISABLE_PROMOTE: Chicken switch: Disables promote, force checkin.
    // [9:11] SPECULATIVE_CHECKIN_COUNT: Speculative checkin count.
    scom_and_or(
        0x20110D6,
        ~(PPC_BIT(6) | PPC_BITMASK(9, 11)),
        PPC_BIT(6) | PPC_BIT(10));
    // DMA and Engine FIR Mask Register
    // [0:49] NX_DMA_ENG_FIR_MASK_BITS: DMA and Engine FIR Mask Register.
    scom_and_or(
        0x2011103,
        ~PPC_BITMASK(0, 49),
        PPC_BITMASK(2, 3)   | PPC_BIT(8)  | PPC_BIT(14) | PPC_BIT(19)
        | PPC_BITMASK(25, 30) | PPC_BIT(33) | PPC_BITMASK(40, 49));
    scom_and(0x2011106, ~PPC_BITMASK(0, 49))
    // DMA and Engine FIR Action 1 Register
    // [0:49] NX_DMA_ENG_FIR_ACTION1_BITS: DMA and Engine FIR Action 1 Register.
    if(CHIP_EC == 0x20)
    {
        scom_and_or(
            0x2011107,
            ~PPC_BITMASK(0, 49),
            PPC_BIT(4)  | PPC_BIT(6)          | PPC_BITMASK(9, 11)
            | PPC_BIT(13) | PPC_BITMASK(34, 37) | PPC_BIT(39));
    }
    else
    {
        scom_and_or(
            0x2011107,
            ~(PPC_BITMASK(0, 4) | PPC_BITMASK(8, 16) | PPC_BITMASK(19, 49)),
            PPC_BIT(4)          | PPC_BIT(6) | PPC_BITMASK(9, 13)
            | PPC_BITMASK(34, 37) | PPC_BIT(39));
    }
}
