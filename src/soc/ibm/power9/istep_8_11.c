/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_8_11.h>
#include <cpu/power/scom.h>

void call_proc_xbus_enable_ridi(void)
{
    p9_chiplet_scominit();
    p9_io_obus_firmask_save(target, i_target_chip.getChildren<fapi2::TARGET_TYPE_OBUS>(fapi2::TARGET_STATE_FUNCTIONAL));
    p9_psi_scom()
    p9_io_obus_scominit()
    p9_npu_scominit()
    p9_chiplet_enable_ridi();
}

void p9_chiplet_enable_ridi_net_ctrl_action_function(chiplet_id_t chiplet)
{
    if (read_scom_for_chiplet(chiplet, PERV_NET_CTRL0) & PPC_BIT(0))
    {
        scom_write_for_chiplet(chiplet, PERV_NET_CTRL0_WOR, PPC_BITMASK(19, 21));
    }
}

void p9_chiplet_enable_ridi(const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP> &i_target_chip)
{
    for (auto l_target_cplt : i_target_chip.getChildren<fapi2::TARGET_TYPE_PERV>(
      static_cast<fapi2::TargetFilter>(
        fapi2::TARGET_FILTER_ALL_MC |
        fapi2::TARGET_FILTER_ALL_PCI |
        fapi2::TARGET_FILTER_ALL_OBUS),
        fapi2::TARGET_STATE_FUNCTIONAL))
    {
        p9_chiplet_enable_ridi_net_ctrl_action_function(l_target_cplt);
    }
}

fapi2::ReturnCode p9_npu_scominit(
    const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP> &i_target)
{

    uint8_t l_npu_enabled;

    // check to see ifNPU region in N3 chiplet is enabled
    l_npu_enabled = fapi2::ATTR_PROC_NPU_REGION_ENABLED[i_target];

    if(l_npu_enabled)
    {
        fapi2::ReturnCode l_rc;
        fapi2::buffer<uint64_t> l_atrmiss = 0;

        // apply NPU SCOM inits from initfile
        FAPI_EXEC_HWP(l_rc, p9_npu_scom, i_target, fapi2::Target<fapi2::TARGET_TYPE_SYSTEM>());

        // apply additional SCOM inits
        l_atrmiss.setBit<PU_NPU_SM2_XTS_ATRMISS_FLAG_MAP>().setBit<PU_NPU_SM2_XTS_ATRMISS_ENA>();

        fapi2::putScomUnderMask(
            i_target,
            PU_NPU_SM2_XTS_ATRMISS_POST_P9NDD1,
            l_atrmiss,
            l_atrmiss);

        // enable NVLINK refclocks
        FAPI_EXEC_HWP(l_rc, p9_nv_ref_clk_enable, i_target);
    }
}

void p9_io_obus_scominit(const fapi2::Target<fapi2::TARGET_TYPE_OBUS> &i_target)
{
    const uint8_t GROUP_00 = 0;
    const uint8_t LANE_00 = 0;
    const uint8_t SET_RESET = 1;
    const uint8_t CLEAR_RESET = 0;
    // get system target
    const fapi2::Target<fapi2::TARGET_TYPE_SYSTEM> l_system_target;
    // get a proc target
    fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP> l_proc_target = i_target.getParent<fapi2::TARGET_TYPE_PROC_CHIP>();
    // assert IO reset to power-up bus endpoint logic
    io::rmw(OPT_IORESET_HARD_BUS0, i_target, GROUP_00, LANE_00, SET_RESET);
    // Bus Reset is relatively fast, only needing < a hundred cycles to allow the signal to propogate.
    sleep(10ns);
    io::rmw(OPT_IORESET_HARD_BUS0, i_target, GROUP_00, LANE_00, CLEAR_RESET);
    p9_obus_scom(i_target);

    scom_write(OBUS_FIR_ACTION0_REG, OBUS_PHY_FIR_ACTION0);
    scom_write(OBUS_FIR_ACTION1_REG, OBUS_PHY_FIR_ACTION1);
    scom_write(OBUS_FIR_MASK_REG, OBUS_PHY_FIR_MASK);
}

void p9_psi_scom()
{
    scom_or(0x4011803, PPC_BITMASK(0, 6));
    scom_and(0x4011806, ~PPC_BITMASK(0, 6));
    scom_and(0x4011807, ~PPC_BITMASK(0, 6));
    scom_and_or(0x5012903, 0x00000007FFFFFFFF, 0x7E040DF << 35)
    scom_and(0x5012906, ~PPC_BITMASK(0, 29));
    scom_and_or(0x5012907, 0x00000007FFFFFFFF, 0x18050020 << 35)
    scom_and(0x501290F, ~(PPC_BITMASK(16, 27) | PPC_BITMASK(48, 52)))
}

void p9_io_obus_firmask_save(const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target_chip,
        const std::vector<fapi2::Target<fapi2::TARGET_TYPE_OBUS>> i_obus_targets)
{
    scom_or(PU_IOE_PB_IOO_FIR_MASK_REG, PPC_BITMASK(28, 35) | PPC_BITMASK(52, 59))
    for (size_t obus_index = 0; obus_index < LEN(obus_chiplets); obus_index++)
    {
        uint64_t obus_target = obus_chiplets[obus_index]
        uint64_t firMaskReg = read_scom_for_chiplet(obus_target, OBUS_LL0_LL0_LL0_PB_IOOL_FIR_MASK_REG);
        uint64_t action0Buffer = read_scom_for_chiplet(obus_target, OBUS_LL0_PB_IOOL_FIR_ACTION0_REG);
        uint64_t action1Buffer = read_scom_for_chiplet(obus_target, OBUS_LL0_PB_IOOL_FIR_ACTION1_REG);

        for(size_t bit_index = PB_IOOL_FIR_MASK_REG_FIR_LINK0_NO_SPARE_MASK;
          bit_index < OBUS_LL0_LL0_LL0_PB_IOOL_FIR_MASK_REG_LINK1_TOO_MANY_CRC_ERRORS+1;
          ++bit_index)
        {
            if((action0Buffer & PPC_BIT(i) == 0) && (action1Buffer & PPC_BIT(i)))
            {
                firMaskReg |= PPC_BIT(i);
            }
        }
        for(size_t bit_index = OBUS_LL0_LL0_LL0_PB_IOOL_FIR_MASK_REG_LINK0_CORRECTABLE_ARRAY_ERROR;
          bit_index < OBUS_LL0_LL0_LL0_PB_IOOL_FIR_MASK_REG_LINK1_TOO_MANY_CRC_ERRORS+1;
          ++bit_index)
        {
            if((action0Buffer & PPC_BIT(i) == 0) && (action1Buffer & PPC_BIT(i)))
            {
                firMaskReg |= PPC_BIT(i);
            }
        }
        write_scom_for_chiplet(
          obus_target,
          OBUS_LL0_LL0_LL0_PB_IOOL_FIR_MASK_REG,
          firMaskReg);
    }
}

void p9_chiplet_scominit(void)
{
    // i_target_proc_chip = TYPE_PROC or PROC_CHIP
    fapi2::Target<fapi2::TARGET_TYPE_SYSTEM> FAPI_SYSTEM;
    std::vector<fapi2::Target<fapi2::TARGET_TYPE_MCS>>  l_mcs_targets;
    std::vector<fapi2::Target<fapi2::TARGET_TYPE_CAPP>> l_capp_targets;

    l_mcs_targets = i_target_proc_chip.getChildren<fapi2::TARGET_TYPE_MCS>();
    for (const auto &l_mcs_target : l_mcs_targets)
    {
        p9n_mcs_scom(l_mcs_target);
    }

    // read spare FBC FIR bit -- ifset, SBE has configured XBUS FIR resources for all
    // present units, and code here will be run to mask resources associated with
    // non-functional units

    uint64_t l_scom_data = read_scom(PU_PB_CENT_SM0_PB_CENT_FIR_REG);
    if(l_scom_data & PU_PB_CENT_SM0_PB_CENT_FIR_MASK_REG_SPARE_13)
    {
        write_scom(XBUS_1_LL1_LL1_LL1_IOEL_FIR_MASK_REG, FBC_IOE_DL_FIR_MASK_NF);
        write_scom(XBUS_1_FIR_MASK_REG, XBUS_PHY_FIR_MASK_NF);
        write_scom(PU_PB_IOE_FIR_MASK_REG_OR, FBC_IOE_TL_FIR_MASK_X1_NF);
        write_scom(PU_PB_CENT_SM1_EXTFIR_MASK_REG_OR, FBC_EXT_FIR_MASK_X1_NF);


        write_scom(XBUS_2_LL2_LL2_LL2_IOEL_FIR_MASK_REG, FBC_IOE_DL_FIR_MASK_NF);
        write_scom(XBUS_2_FIR_MASK_REG, XBUS_PHY_FIR_MASK_NF);
        write_scom(PU_PB_IOE_FIR_MASK_REG_OR, FBC_IOE_TL_FIR_MASK_X2_NF);
        write_scom(PU_PB_CENT_SM1_EXTFIR_MASK_REG_OR, FBC_EXT_FIR_MASK_X2_NF);
    }

    p9_fbc_ioo_tl_scom(i_target_proc_chip);

    write_scom(PU_IOE_PB_IOO_FIR_ACTION0_REG, FBC_IOO_TL_FIR_ACTION0);
    write_scom(PU_IOE_PB_IOO_FIR_ACTION1_REG, FBC_IOO_TL_FIR_ACTION1);
    write_scom(PU_IOE_PB_IOO_FIR_MASK_REG, FBC_IOO_TL_FIR_MASK);

    for(size_t obus_index = 0; obus_index < LEN(obus_chiplets); ++obus_index)
    {
        // configure action registers & unmask
        write_scom_for_chiplet(obus_chiplets[obus_index], OBUS_LL0_PB_IOOL_FIR_ACTION0_REG, FBC_IOO_DL_FIR_ACTION0);
        write_scom_for_chiplet(obus_chiplets[obus_index], OBUS_LL0_PB_IOOL_FIR_ACTION1_REG, FBC_IOO_DL_FIR_ACTION1);
        write_scom_for_chiplet(obus_chiplets[obus_index], OBUS_LL0_LL0_LL0_PB_IOOL_FIR_MASK_REG, FBC_IOO_DL_FIR_MASK);
        p9_fbc_ioo_dl_scom(obus_chiplets[obus_index], i_target_proc_chip);
    }
    // Invoke NX SCOM initfile
    p9_nx_scom();
    // Invoke CXA SCOM initfile
    l_capp_targets = i_target_proc_chip.getChildren<fapi2::TARGET_TYPE_CAPP>();
    for (auto l_capp : l_capp_targets)
    {
        p9_cxa_scom(l_capp, FAPI_SYSTEM, i_target_proc_chip);
    }
    // Invoke INT SCOM initfile
    p9_int_scom();
    // Invoke VAS SCOM initfile
    p9_vas_scom();

    // Setup NMMU epsilon write cycles
    scom_and_or(
      PU_NMMU_MM_EPSILON_COUNTER_VALUE_REG,
      ~(PPC_BITMASK(0, 11) | PPC_BITMASK(16, 27)),
      PPC_BITMASK(9, 11) | PPC_BIT(21) | PPC_BITMASK(26, 27))
}

void p9n_mcs_scom(const fapi2::Target<fapi2::TARGET_TYPE_MCS>& TGT0)
{
    scom_and_or_for_chiplet(TGT0, 0x5010810, 0xFFFFFFFF01FC3E00, 0x3201C07D);
    scom_or_for_chiplet(
      TGT0, 0x5010811,
      PPC_BIT(8) | PPC_BIT(9) | PPC_BIT(17)
      | PPC_BIT(20) | PPC_BIT(27) | PPC_BIT(28))
    scom_and_for_chiplet(TGT0, 0x5010811, ~PPC_BIT(7));
    scom_and_or_for_chiplet(
      TGT0, 0x5010812, ~PPC_BITMASK(33, 52), PPC_BIT(10) | PPC_BIT(45));
    scom_or_for_chiplet(
      TGT0, 0x5010813,
      ~(PPC_BITMASK(0, 13) | PPC_BITMASK(24, 39)),
      PPC_BITMASK(4, 5) | PPC_BIT(36))
    scom_and_or_for_chiplet(
      TGT0, 0x501081B,
      ~(PPC_BIT(1) | PPC_BITMASK(24, 31)),
      PPC_BITMASK(0, 2) | PPC_BITMASK(5, 7)
      | PPC_BIT(24) | PPC_BIT(32) | PPC_BIT(34))
}

void p9_fbc_ioo_dl_scom(
    const fapi2::Target<fapi2::TARGET_TYPE_OBUS>& obus_chiplet,
    const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& proc_chiplet)
{
    fapi2::ATTR_LINK_TRAIN_Type l_TGT0_ATTR_LINK_TRAIN;
    l_TGT0_ATTR_LINK_TRAIN = fapi2::ATTR_LINK_TRAIN
    fapi2::ATTR_PROC_NPU_REGION_ENABLED_Type l_TGT1_ATTR_PROC_NPU_REGION_ENABLED;
    l_TGT1_ATTR_PROC_NPU_REGION_ENABLED = fapi2::ATTR_PROC_NPU_REGION_ENABLED[proc_chiplet];

    // Power Bus OLL Configuration Register
    // PB.IOO.LL0.IOOL_CONFIG
    // Processor bus OLL configuration register
    if(l_TGT0_ATTR_LINK_TRAIN == fapi2::ENUM_ATTR_LINK_TRAIN_BOTH)
    {
        scom_and_or(
            0x901080A,
            ~(PPC_BIT(11) | PPC_BITMASK(32, 35)),
            PPC_BIT(0)          | PPC_BIT(2)          | PPC_BIT(4)
          | PPC_BITMASK(12, 15) | PPC_BITMASK(28, 31) | PPC_BITMASK(48, 51));
    }
    else
    {
        scom_and_or(
            0x901080A,
            ~(PPC_BIT(0) | PPC_BIT(11) | PPC_BITMASK(32, 35)),
            PPC_BIT(2)          | PPC_BIT(4)          | PPC_BITMASK(12, 15)
          | PPC_BITMASK(28, 31) | PPC_BITMASK(48, 51));
    }
    // Power Bus OLL PHY Training Configuration Register
    // PB.IOO.LL0.IOOL_PHY_CONFIG
    // Processor bus OLL PHY training configuration register
    if(l_TGT1_ATTR_PROC_NPU_REGION_ENABLED)
    {
        scom_and_or(
            0x901080C,
            ~(PPC_BITMASK(0, 15) | PPC_BITMASK(58, 59)),
            PPC_BIT(0) | PPC_BIT(2) | PPC_BITMASK(4, 7) | PPC_BITMASK(61, 63));
    }
    else
    {
        scom_and_or(
            0x901080C,
            ~(PPC_BITMASK(0, 15) | PPC_BITMASK(58, 59)),
            PPC_BIT(0) | PPC_BIT(2) | PPC_BITMASK(4, 7));
    }
    // Power Bus OLL Optical Configuration Register
    // PB.IOO.LL0.IOOL_OPTICAL_CONFIG
    // Processor bus OLL optical configuration register
    scom_and_or(
        0x901080F,
      ~(PPC_BITMASK(2, 7)   | PPC_BITMASK(9, 15)
      | PPC_BITMASK(20, 23) | PPC_BITMASK(25, 31) | PPC_BITMASK(56, 57)),
        PPC_BITMASK(2, 3)   | PPC_BIT(5)  | PPC_BIT(7)
      | PPC_BITMASK(12, 15) | PPC_BIT(21) | PPC_BIT(23)
      | PPC_BITMASK(26, 31) | PPC_BIT(37) | PPC_BIT(39)
      | PPC_BIT(42)         | PPC_BIT(43) | PPC_BIT(56));
    // Power Bus OLL Replay Threshold Register
    // PB.IOO.LL0.IOOL_REPLAY_THRESHOLD
    // Processor bus OLL replay threshold register
    scom_and_or(
        0x9010818,
        ~(PPC_BITMASK(0, 10)),
        PPC_BITMASK(1, 2) | PPC_BITMASK(4, 10));
    // Power Bus OLL SL ECC Threshold Register
    // PB.IOO.LL0.IOOL_SL_ECC_THRESHOLD
    // Processor bus OLL SL ECC Threshold register
    scom_and_or(0x9010819, ~(PPC_BITMASK(0, 9)), PPC_BITMASK(1, 9));
    // Power Bus OLL Retrain Threshold Register
    // PB.IOO.LL0.IOOL_RETRAIN_THRESHOLD
    // Processor bus OLL retrain threshold register
    scom_and_or(
        0x901081A,
        ~(PPC_BITMASK(0, 16)),
        PPC_BITMASK(1, 7) | PPC_BITMASK(14, 16));
}

void p9_vas_scom(void)
{
    scom_and_or(0x3011803, ~PPC_BITMASK(0, 53), 0x00210102540D7FFF);
    scom_and(0x3011806, ~PPC_BITMASK(0, 53));
    if(CHIP_EC == 0x20)
    {
        scom_and_or(0x3011807, ~PPC_BITMASK(0, 53), 0x00DD020180000000);
    }
    else
    {
        scom_and_or(0x3011807, ~PPC_BITMASK(0, 53), 0x00DF020180000000);
    }
    if(CHIP_EC >= 0x22 && SECURE_MEMORY)
    {
        scom_and_or(
            0x301184D,
            ~(PPC_BITMASK(0, 6) | PPC_BITMASK(11, 12)),
            PPC_BIT(3) | PPC_BIT(11));
    }
    else
    {
        scom_and_or(0x301184D, ~PPC_BITMASK(0, 6), PPC_BIT(3));
    }
    scom_and_or(
        0x301184E,
        ~(PPC_BIT(13) | PPC_BITMASK(20, 35)),
        PPC_BIT(1)    | PPC_BIT(2)          | PPC_BIT(5)           | PPC_BIT(6)
      | PPC_BIT(14)   | PPC_BITMASK(20, 25) | PPC_BITMASK(28, 33));
    if(CHIP_EC == 0x20)
    {
        scom_or(0x301184F, PPC_BIT(0));
    }
}

void p9_int_scom()
{
    if(CHIP_EC >= 0x22 && SECURE_MEMORY)
    {
        scom_and_or(
            0x501300A,
            ~(PPC_BIT(0) | PPC_BIT(3) | PPC_BIT(9)),
            PPC_BIT(1) | PPC_BIT(2) | PPC_BIT(5));
    }
    else
    {
        scom_and_or(
            0x501300A,
            ~(PPC_BIT(0) | PPC_BIT(9)),
            PPC_BIT(1) | PPC_BIT(5));
    }
    scom_or(0x5013021, PPC_BITMASK(46, 47) | PPC_BIT(49));
    if(CHIP_EC == 0x20)
    {
        scom_write(0x5013033, 0x2000005C040281C3);
    }
    else
    {
        scom_write(0x5013033, 0x0000005C040081C3);
    }
    scom_write(0x5013036, 0);
    scom_write(0x5013037, 0x9554021F80110E0C);
    scom_and_or(
        0x5013130,
        ~(PPC_BITMASK(2, 7) | ~PPC_BITMASK(10, 15)),
        PPC_BITMASK(3, 4) | PPC_BITMASK(11, 12));
    scom_write(0x5013140, 0x050043EF00100020);
    scom_write(0x5013141, 0xFADFBB8CFFAFFFD7);
    scom_write(0x5013178, 0x0002000610000000);
    scom_write(0x501320E, 0x6262220242160000);
    scom_and_or(0x5013214, ~PPC_BITMASK(16, 31), 0x00005BBF00000000);
    scom_and_or(0x501322B, ~PPC_BITMASK(58, 63), PPC_BITMASK(59, 60));
    if(CHIP_EC == 0x20)
    {
        scom_and_or(0x5013272, ~PPC_BITMASK(0, 44), 0x0002C01800600000);
        scom_and_or(0x5013273, ~PPC_BITMASK(0, 44), 0xFFFCFFEFFFA00000);
    }

}

void p9_fbc_ioo_tl_scom()
{
    scom_or(0x501380A, 0x84000000840);
    scom_or(0x501380B, 0x84000000840);
    scom_or(0x501380C, 0x84000000840);
    scom_or(0x501380D, 0x84000000840);
    uint64_t scom_buffer = read_scom(0x5013823)
    // l_PB_IOO_SCOM_PB_CFG_IOO01_IS_LOGICAL_PAIR_OFF
    // l_PB_IOO_SCOM_PB_CFG_IOO23_IS_LOGICAL_PAIR_OFF
    // l_PB_IOO_SCOM_PB_CFG_IOO45_IS_LOGICAL_PAIR_OFF
    // l_PB_IOO_SCOM_PB_CFG_IOO67_IS_LOGICAL_PAIR_OFF
    // l_PB_IOO_SCOM_LINKS01_TOD_ENABLE_OFF
    // l_PB_IOO_SCOM_LINKS23_TOD_ENABLE_OFF
    // l_PB_IOO_SCOM_LINKS45_TOD_ENABLE_OFF
    // l_PB_IOO_SCOM_LINKS67_TOD_ENABLE_OFF
    scom_buffer &= ~(PPC_BITMASK(0, 3) | PPC_BITMASK(8, 11));
    if(ATTR_PROC_NPU_REGION_ENABLED)
    {
        // l_PB_IOO_SCOM_SEL_03_NPU_NOT_PB_ON
        // l_PB_IOO_SCOM_SEL_04_NPU_NOT_PB_ON
        // l_PB_IOO_SCOM_SEL_05_NPU_NOT_PB_ON
        scom_buffer |= PPC_BITMASK(13, 15);
    }
    write_scom(0x5013823, scom_buffer)
}

void p9_cxa_scom(chiplet_id_t capp_hiplet)
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
        scom_and_or_for_chiplet(
          capp_hiplet, 0x2010803, ~PPC_BITMASK(0, 53), 0x801B1F98C8717000)
    }
    else
    {
        scom_and_or_for_chiplet(
          capp_hiplet, 0x2010803, ~PPC_BITMASK(0, 53), 0x801B1F98D8717000)
    }
    // Pervasive FIR Action 0 Register
    // FIR_ACTION0: MSB of action select for corresponding bit in FIR.
    // (Action0,Action1) = Action Select.
    // (0, 0) = Checkstop.
    // (0, 1) = Recoverable.
    // (1, 0) = Recoverable interrupt.
    // (1, 1) = Machine check.
    scom_and_or_for_chiplet(capp_hiplet, 0x2010806, ~PPC_BITMASK(0, 53));
    // Pervasive FIR Action 1 Register
    // FIR_ACTION1: LSB of action select for corresponding bit in FIR.
    // (Action0, Action1, Mask) = Action select.
    // (x, x, 1) = Masked.
    // (0, 0, 0) = Checkstop.
    // (0, 1, 0) = Recoverable Error.
    // (1, 0, 0) = Recoverable Interrupt.
    // (1, 1, 0) = Machine Check.
    scom_and_or_for_chiplet(
      capp_hiplet,
      0x2010807,
      PPC_BIT(2) | PPC_BIT(8) | PPC_BIT(34) | PPC_BIT(44));
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

    scom_and_or_for_chiplet(capp_hiplet, 0x2010818,
      ~(PPC_BIT(1) | PPC_BIT(25)),
      PPC_BITMASK(3, 4) | PPC_BIT(6) | PPC_BIT(21));
    // APC Master Configuration Register
    // [4:7] HANG_POLL_SCALE: Determines the number of hang polls that must
    //       be detected to indicate a hang poll to the logic.
    scom_and_or_for_chiplet(capp_hiplet, 0x2010819, ~PPC_BITMASK(4, 7));
    // Snoop Control Register
    // [45:47] CXA_SNP_ADDRESS_PIPELINE_MASTERWAIT_COUNT: Maximum number of
    //         cycles an APC master can wait before swapping the arbitration
    //         priority between APC and the snooper.
    // [48:51] CXA_SNP_DATA_HANG_POLL_SCALE: Number of dhang_polls it takes to
    //         increment the DHANG counter in snoop. Actual count is scale + 1.
    //         Value of 0000 = decimal 1 counts.
    scom_and_or_for_chiplet(
      capp_hiplet,
      0x201081B,
      ~PPC_BITMASK(48, 51),
      PPC_BITMASK(45, 47) | PPC_BIT(50));
    // Transport Control Register
    // [18:20] TLBI_DATA_POLL_PULSE_DIV: Number of data polls received before
    //         signaling TLBI hang-detection timer has expired.
    scom_and_or_for_chiplet(
      capp_hiplet, 0x201081C, ~PPC_BITMASK(18, 20), PPC_BIT(21));
}

void p9_nx_scom(void)
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

fapi2::ReturnCode p9_obus_scom(
    const fapi2::Target<fapi2::TARGET_TYPE_OBUS>& TGT0)
{
    {
        fapi2::ATTR_EC_Type   l_chip_ec;
        fapi2::ATTR_NAME_Type l_chip_id;
        FAPI_ATTR_GET_PRIVILEGED(fapi2::ATTR_NAME, TGT2, l_chip_id);
        FAPI_ATTR_GET_PRIVILEGED(fapi2::ATTR_EC, TGT2, l_chip_ec);
        fapi2::ATTR_OPTICS_CONFIG_MODE_Type l_TGT0_ATTR_OPTICS_CONFIG_MODE;
        FAPI_ATTR_GET(fapi2::ATTR_OPTICS_CONFIG_MODE, TGT0, l_TGT0_ATTR_OPTICS_CONFIG_MODE);
        fapi2::ATTR_IO_O_CHANNEL_TYPE_Type l_TGT0_ATTR_IO_O_CHANNEL_TYPE;
        FAPI_ATTR_GET(fapi2::ATTR_IO_O_CHANNEL_TYPE, TGT0, l_TGT0_ATTR_IO_O_CHANNEL_TYPE);
        uint64_t l_def_OBUS_FBC_ENABLED = (l_TGT0_ATTR_OPTICS_CONFIG_MODE == fapi2::ENUM_ATTR_OPTICS_CONFIG_MODE_SMP);
        fapi2::buffer<uint64_t> l_scom_buffer;

        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address                8000000009010C3F (SCOM)
        //  Description            This register contains per-lane spare mode latches.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48          RWX         RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000000009010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_0_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_0_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_0_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000009010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address          8000000109010C3F (SCOM)
        //  Description      This register contains per-lane spare mode latches.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48        RWX     RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49        RWX     RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000000109010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_0_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_0_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_0_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000109010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address          8000000209010C3F (SCOM)
        //  Description      This register contains per-lane spare mode latches.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48        RWX     RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49        RWX     RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000000209010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_0_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_0_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_0_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000209010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address                8000000309010C3F (SCOM)
        //  Description            This register contains per-lane spare mode latches.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48          RWX         RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49          RWX         RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000000309010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_0_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_0_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_0_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000309010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address                8000000409010C3F (SCOM)
        //  Description            This register contains per-lane spare mode latches.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48          RWX         RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49          RWX         RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000000409010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_1_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_1_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_1_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000409010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address          8000000509010C3F (SCOM)
        //  Description      This register contains per-lane spare mode latches.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48        RWX     RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49        RWX     RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000000509010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_1_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_1_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_1_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000509010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address          8000000609010C3F (SCOM)
        //  Description      This register contains per-lane spare mode latches.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48        RWX     RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49        RWX     RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000000609010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_1_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_1_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_1_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000609010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address                8000000709010C3F (SCOM)
        //  Description            This register contains per-lane spare mode latches.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48          RWX         RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49          RWX         RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000000709010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_1_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_1_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_1_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000709010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address                8000000809010C3F (SCOM)
        //  Description            This register contains per-lane spare mode latches.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48          RWX         RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49          RWX         RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000000809010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_2_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_2_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_2_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000809010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address                8000000909010C3F (SCOM)
        //  Description            This register contains per-lane spare mode latches.
        fapi2::getScom(TGT0, 0x8000000909010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_2_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_2_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_2_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000909010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address          8000000A09010C3F (SCOM)
        //  Description      This register contains per-lane spare mode latches.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48        RWX     RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49        RWX     RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000000A09010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_2_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_2_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_2_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000A09010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address          8000000B09010C3F (SCOM)
        //  Description      This register contains per-lane spare mode latches.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48        RWX     RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49        RWX     RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000000B09010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_2_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_2_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_2_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000B09010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address                8000000C09010C3F (SCOM)
        //  Description            This register contains per-lane spare mode latches.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48          RWX         RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49          RWX         RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000000C09010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_3_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_3_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_3_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000C09010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address                8000000D09010C3F (SCOM)
        //  Description            This register contains per-lane spare mode latches.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48          RWX         RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49          RWX         RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000000D09010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_3_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_3_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_3_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000D09010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address          8000000E09010C3F (SCOM)
        //  Description      This register contains per-lane spare mode latches.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48        RWX     RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49        RWX     RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000000E09010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_3_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_3_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_3_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000E09010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address          8000000F09010C3F (SCOM)
        //  Description      This register contains per-lane spare mode latches.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48        RWX     RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49        RWX     RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000000F09010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_3_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_3_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_3_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000000F09010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address                8000001009010C3F (SCOM)
        //  Description            This register contains per-lane spare mode latches.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48          RWX         RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49          RWX         RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        // Advance                                                                               POWER9 Registers
        fapi2::getScom(TGT0, 0x8000001009010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_4_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_4_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_4_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000001009010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address                8000001109010C3F (SCOM)
        //  Description            This register contains per-lane spare mode latches.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48          RWX         RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49          RWX         RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000001109010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_4_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_4_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_4_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000001109010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address                8000001209010C3F (SCOM)
        //  Description            This register contains per-lane spare mode latches.
        fapi2::getScom(TGT0, 0x8000001209010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_4_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_4_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_4_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000001209010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address          8000001309010C3F (SCOM)
        //  Description      This register contains per-lane spare mode latches.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48        RWX     RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49        RWX     RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000001309010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_4_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_4_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_4_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000001309010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address          8000001409010C3F (SCOM)
        //  Description      This register contains per-lane spare mode latches.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48        RWX     RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49        RWX     RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000001409010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_5_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_5_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_5_RXPACK_RD_SLICE_0_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000001409010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address                8000001509010C3F (SCOM)
        //  Description            This register contains per-lane spare mode latches.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48          RWX         RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49          RWX         RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000001509010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_5_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_5_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_5_RXPACK_RD_SLICE_1_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000001509010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address                8000001609010C3F (SCOM)
        //  Description            This register contains per-lane spare mode latches.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48          RWX         RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49          RWX         RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000001609010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_5_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_5_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_5_RXPACK_RD_SLICE_2_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000001609010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DATA_DAC_SPARE_MODE_PL
        //  Address          8000001709010C3F (SCOM)
        //  Description      This register contains per-lane spare mode latches.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48        RWX     RX_PL_DATA_DAC_SPARE_MODE_0: Per-lane spare mode latch.
        // 49        RWX     RX_PL_DATA_DAC_SPARE_MODE_1: Per-lane spare mode latch.
        fapi2::getScom(TGT0, 0x8000001709010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_5_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
        l_scom_buffer.insert<54, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_5_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
        l_scom_buffer.insert<55, 1, 63, uint64_t>(0); // l_IOO0_IOO_CPLT_RX0_RXPACKS_5_RXPACK_RD_SLICE_3_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
        l_scom_buffer.insert<60, 4, 60, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000001709010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address          8000280009010C3F (SCOM)
        //  Description      This register contains the fifth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000280009010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280009010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address                8000280109010C3F (SCOM)
        //  Description            This register contains the fifth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000280109010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280109010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address                8000280209010C3F (SCOM)
        //  Description            This register contains the fifth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000280209010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280209010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address                8000280309010C3F (SCOM)
        //  Description            This register contains the fifth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000280309010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280309010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address          8000280409010C3F (SCOM)
        //  Description      This register contains the fifth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000280409010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280409010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address          8000280509010C3F (SCOM)
        //  Description      This register contains the fifth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000280509010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280509010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address          8000280609010C3F (SCOM)
        //  Description      This register contains the fifth set of EO DAC controls.
        fapi2::getScom(TGT0, 0x8000280609010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280609010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address                8000280709010C3F (SCOM)
        //  Description            This register contains the fifth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000280709010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280709010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address                8000280809010C3F (SCOM)
        //  Description            This register contains the fifth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000280809010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280809010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address          8000280909010C3F (SCOM)
        //  Description      This register contains the fifth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000280909010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280909010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address          8000280A09010C3F (SCOM)
        //  Description      This register contains the fifth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000280A09010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280A09010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address          8000280B09010C3F (SCOM)
        //  Description      This register contains the fifth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000280B09010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280B09010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address                8000280C09010C3F (SCOM)
        //  Description            This register contains the fifth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000280C09010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280C09010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address                8000280D09010C3F (SCOM)
        //  Description            This register contains the fifth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000280D09010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280D09010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address                8000280E09010C3F (SCOM)
        //  Description            This register contains the fifth set of EO DAC controls.
        fapi2::getScom(TGT0, 0x8000280E09010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280E09010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address          8000280F09010C3F (SCOM)
        //  Description      This register contains the fifth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000280F09010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000280F09010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address          8000281009010C3F (SCOM)
        //  Description      This register contains the fifth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000281009010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000281009010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address                8000281109010C3F (SCOM)
        //  Description            This register contains the fifth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000281109010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000281109010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address                8000281209010C3F (SCOM)
        //  Description            This register contains the fifth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000281209010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000281209010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address                8000281309010C3F (SCOM)
        //  Description            This register contains the fifth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000281309010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000281309010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address          8000281409010C3F (SCOM)
        //  Description      This register contains the fifth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000281409010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000281409010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address          8000281509010C3F (SCOM)
        //  Description      This register contains the fifth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000281509010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000281509010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address          8000281609010C3F (SCOM)
        //  Description      This register contains the fifth set of EO DAC controls.
        fapi2::getScom(TGT0, 0x8000281609010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000281609010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_EO_PL
        //  Address                8000281709010C3F (SCOM)
        //  Description            This register contains the fifth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000281709010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000281709010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address                8000300009010C3F (SCOM)
        //  Description            This register contains the sixth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52          RO          constant = 0b0
        fapi2::getScom(TGT0, 0x8000300009010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000300009010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address          8000300109010C3F (SCOM)
        //  Description      This register contains the sixth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52        RO      constant = 0b0
        fapi2::getScom(TGT0, 0x8000300109010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000300109010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address          8000300209010C3F (SCOM)
        //  Description      This register contains the sixth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52        RO      constant = 0b0
        fapi2::getScom(TGT0, 0x8000300209010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000300209010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address          8000300309010C3F (SCOM)
        //  Description      This register contains the sixth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52        RO      constant = 0b0
        fapi2::getScom(TGT0, 0x8000300309010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        if((l_TGT0_ATTR_OPTICS_CONFIG_MODE == fapi2::ENUM_ATTR_OPTICS_CONFIG_MODE_SMP))
        {
            l_scom_buffer.insert<48, 5, 59, uint64_t>(0x14);
        }
        else
        {
            l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        }
        fapi2::putScom(TGT0, 0x8000300309010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address                8000300409010C3F (SCOM)
        //  Description            This register contains the sixth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52          RO          constant = 0b0
        fapi2::getScom(TGT0, 0x8000300409010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000300409010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address                8000300509010C3F (SCOM)
        //  Description            This register contains the sixth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52          RO          constant = 0b0
        fapi2::getScom(TGT0, 0x8000300509010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        if((l_TGT0_ATTR_OPTICS_CONFIG_MODE == fapi2::ENUM_ATTR_OPTICS_CONFIG_MODE_SMP))
        {
            l_scom_buffer.insert<48, 5, 59, uint64_t>(0x14);
        }
        else
        {
            l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        }
        fapi2::putScom(TGT0, 0x8000300509010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address                8000300609010C3F (SCOM)
        //  Description            This register contains the sixth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52          RO          constant = 0b0
        fapi2::getScom(TGT0, 0x8000300609010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000300609010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address          8000300709010C3F (SCOM)
        //  Description      This register contains the sixth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52        RO      constant = 0b0
        fapi2::getScom(TGT0, 0x8000300709010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000300709010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address          8000300809010C3F (SCOM)
        //  Description      This register contains the sixth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52        RO      constant = 0b0
        fapi2::getScom(TGT0, 0x8000300809010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000300809010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address          8000300909010C3F (SCOM)
        //  Description      This register contains the sixth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52        RO      constant = 0b0
        fapi2::getScom(TGT0, 0x8000300909010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        if((l_TGT0_ATTR_OPTICS_CONFIG_MODE == fapi2::ENUM_ATTR_OPTICS_CONFIG_MODE_SMP))
        {
            l_scom_buffer.insert<48, 5, 59, uint64_t>(0x14);
        }
        else
        {
            l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        }
        fapi2::putScom(TGT0, 0x8000300909010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address                8000300A09010C3F (SCOM)
        //  Description            This register contains the sixth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52          RO          constant = 0b0
        fapi2::getScom(TGT0, 0x8000300A09010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000300A09010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address                8000300B09010C3F (SCOM)
        //  Description            This register contains the sixth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52          RO          constant = 0b0
        fapi2::getScom(TGT0, 0x8000300B09010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000300B09010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address                8000300C09010C3F (SCOM)
        //  Description            This register contains the sixth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52          RO          constant = 0b0
        fapi2::getScom(TGT0, 0x8000300C09010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000300C09010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address          8000300D09010C3F (SCOM)
        //  Description      This register contains the sixth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52        RO      constant = 0b0
        fapi2::getScom(TGT0, 0x8000300D09010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000300D09010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address          8000300E09010C3F (SCOM)
        //  Description      This register contains the sixth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52        RO      constant = 0b0
        fapi2::getScom(TGT0, 0x8000300E09010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000300E09010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address          8000300F09010C3F (SCOM)
        //  Description      This register contains the sixth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52        RO      constant = 0b0
        fapi2::getScom(TGT0, 0x8000300F09010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000300F09010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address                8000301009010C3F (SCOM)
        //  Description            This register contains the sixth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52          RO          constant = 0b0
        fapi2::getScom(TGT0, 0x8000301009010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000301009010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address                8000301109010C3F (SCOM)
        //  Description            This register contains the sixth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52          RO          constant = 0b0
        fapi2::getScom(TGT0, 0x8000301109010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000301109010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address                8000301209010C3F (SCOM)
        //  Description            This register contains the sixth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52          RO          constant = 0b0
        fapi2::getScom(TGT0, 0x8000301209010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000301209010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address          8000301309010C3F (SCOM)
        //  Description      This register contains the sixth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52        RO      constant = 0b0
        fapi2::getScom(TGT0, 0x8000301309010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000301309010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address          8000301409010C3F (SCOM)
        //  Description      This register contains the sixth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52        RO      constant = 0b0
        fapi2::getScom(TGT0, 0x8000301409010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000301409010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address          8000301509010C3F (SCOM)
        //  Description      This register contains the sixth set of EO DAC controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52        RO      constant = 0b0
        fapi2::getScom(TGT0, 0x8000301509010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000301509010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address                8000301609010C3F (SCOM)
        //  Description            This register contains the sixth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52          RO          constant = 0b0
        fapi2::getScom(TGT0, 0x8000301609010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000301609010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL6_EO_PL
        //  Address                8000301709010C3F (SCOM)
        //  Description            This register contains the sixth set of EO DAC controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_A_CTLE_COARSE: This is the CTLE coarse peak value. Only 4 bits are currently used.
        // 52          RO          constant = 0b0
        fapi2::getScom(TGT0, 0x8000301709010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000301709010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address          8000980009010C3F (SCOM)
        //  Description      This register contains the fourth set of O controls.
        fapi2::getScom(TGT0, 0x8000980009010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980009010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address                8000980109010C3F (SCOM)
        //  Description            This register contains the fourth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000980109010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980109010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address                8000980209010C3F (SCOM)
        //  Description            This register contains the fourth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000980209010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980209010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address          8000980309010C3F (SCOM)
        //  Description      This register contains the fourth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000980309010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980309010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address          8000980409010C3F (SCOM)
        //  Description      This register contains the fourth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000980409010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980409010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address          8000980509010C3F (SCOM)
        //  Description      This register contains the fourth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000980509010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980509010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address                8000980609010C3F (SCOM)
        //  Description            This register contains the fourth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000980609010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980609010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address                8000980709010C3F (SCOM)
        //  Description            This register contains the fourth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000980709010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980709010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address                8000980809010C3F (SCOM)
        //  Description            This register contains the fourth set of O controls.
        fapi2::getScom(TGT0, 0x8000980809010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980809010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address          8000980909010C3F (SCOM)
        //  Description      This register contains the fourth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000980909010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980909010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address          8000980A09010C3F (SCOM)
        //  Description      This register contains the fourth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000980A09010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980A09010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address                8000980B09010C3F (SCOM)
        //  Description            This register contains the fourth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000980B09010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980B09010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address                8000980C09010C3F (SCOM)
        //  Description            This register contains the fourth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000980C09010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980C09010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address                8000980D09010C3F (SCOM)
        //  Description            This register contains the fourth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000980D09010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980D09010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address          8000980E09010C3F (SCOM)
        //  Description      This register contains the fourth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000980E09010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980E09010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address          8000980F09010C3F (SCOM)
        //  Description      This register contains the fourth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000980F09010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000980F09010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address          8000981009010C3F (SCOM)
        //  Description      This register contains the fourth set of O controls.
        fapi2::getScom(TGT0, 0x8000981009010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000981009010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address                8000981109010C3F (SCOM)
        //  Description            This register contains the fourth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000981109010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000981109010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address                8000981209010C3F (SCOM)
        //  Description            This register contains the fourth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000981209010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000981209010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address          8000981309010C3F (SCOM)
        //  Description      This register contains the fourth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000981309010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000981309010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address          8000981409010C3F (SCOM)
        //  Description      This register contains the fourth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000981409010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000981409010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address          8000981509010C3F (SCOM)
        //  Description      This register contains the fourth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        fapi2::getScom(TGT0, 0x8000981509010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000981509010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address                8000981609010C3F (SCOM)
        //  Description            This register contains the fourth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000981609010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000981609010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL4_O_PL
        //  Address                8000981709010C3F (SCOM)
        //  Description            This register contains the fourth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        fapi2::getScom(TGT0, 0x8000981709010C3F, l_scom_buffer);
        l_scom_buffer.insert<52, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<57, 5, 59, uint64_t>(0x10);
        l_scom_buffer.insert<48, 4, 60, uint64_t>(0xB);
        fapi2::putScom(TGT0, 0x8000981709010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address                8000A00009010C3F (SCOM)
        //  Description            This register contains the fifth set of O controls.
        fapi2::getScom(TGT0, 0x8000A00009010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000A00009010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address          8000A00109010C3F (SCOM)
        //  Description      This register contains the fifth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_B_CTLE_GAIN: This is the CTLE gain setting.
        fapi2::getScom(TGT0, 0x8000A00109010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000A00109010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address          8000A00209010C3F (SCOM)
        //  Description      This register contains the fifth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_B_CTLE_GAIN: This is the CTLE gain setting.
        fapi2::getScom(TGT0, 0x8000A00209010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000A00209010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address          8000A00309010C3F (SCOM)
        //  Description      This register contains the fifth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_B_CTLE_GAIN: This is the CTLE gain setting.
        fapi2::getScom(TGT0, 0x8000A00309010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        if((l_TGT0_ATTR_OPTICS_CONFIG_MODE == fapi2::ENUM_ATTR_OPTICS_CONFIG_MODE_SMP))
        {
            l_scom_buffer.insert<48, 5, 59, uint64_t>(0x14);
        }
        else
        {
            l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        }
        fapi2::putScom(TGT0, 0x8000A00309010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address                8000A00409010C3F (SCOM)
        //  Description            This register contains the fifth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_B_CTLE_GAIN: This is the CTLE gain setting.
        fapi2::getScom(TGT0, 0x8000A00409010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        fapi2::putScom(TGT0, 0x8000A00409010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address                8000A00509010C3F (SCOM)
        //  Description            This register contains the fifth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_B_CTLE_GAIN: This is the CTLE gain setting.
        fapi2::getScom(TGT0, 0x8000A00509010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        if((l_TGT0_ATTR_OPTICS_CONFIG_MODE == fapi2::ENUM_ATTR_OPTICS_CONFIG_MODE_SMP))
        {
            l_scom_buffer.insert<48, 5, 59, uint64_t>(0x14);
        }
        else
        {
            l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        }
        fapi2::putScom(TGT0, 0x8000A00509010C3F, l_scom_buffer);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address                8000A00609010C3F (SCOM)
        //  Description            This register contains the fifth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_B_CTLE_GAIN: This is the CTLE gain setting.
        // Advance                                                                               POWER9 Registers
        scom_and_or_for_chiplet(TGT0, 0x8000A00609010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address                8000A00709010C3F (SCOM)
        //  Description            This register contains the fifth set of O controls.
        scom_and_or_for_chiplet(TGT0, 0x8000A00709010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address          8000A00809010C3F (SCOM)
        //  Description      This register contains the fifth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_B_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000A00809010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address          8000A00909010C3F (SCOM)
        //  Description      This register contains the fifth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_B_CTLE_GAIN: This is the CTLE gain setting.
        fapi2::getScom(TGT0, 0x8000A00909010C3F, l_scom_buffer);
        l_scom_buffer.insert<53, 4, 60, uint64_t>(0xA);
        if((l_TGT0_ATTR_OPTICS_CONFIG_MODE == fapi2::ENUM_ATTR_OPTICS_CONFIG_MODE_SMP))
        {
            l_scom_buffer.insert<48, 5, 59, uint64_t>(0x14);
        }
        else
        {
            l_scom_buffer.insert<48, 5, 59, uint64_t>(0x3);
        }
        fapi2::putScom(TGT0, 0x8000A00909010C3F, l_scom_buffer);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address          8000A00A09010C3F (SCOM)
        //  Description      This register contains the fifth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_B_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000A00A09010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address                8000A00B09010C3F (SCOM)
        //  Description            This register contains the fifth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_B_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000A00B09010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address                8000A00C09010C3F (SCOM)
        //  Description            This register contains the fifth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_B_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000A00C09010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address                8000A00D09010C3F (SCOM)
        //  Description            This register contains the fifth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_B_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000A00D09010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address                8000A00E09010C3F (SCOM)
        //  Description            This register contains the fifth set of O controls.
        scom_and_or_for_chiplet(TGT0, 0x8000A00E09010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address          8000A00F09010C3F (SCOM)
        //  Description      This register contains the fifth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_B_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000A00F09010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address          8000A01009010C3F (SCOM)
        //  Description      This register contains the fifth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_B_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000A01009010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address          8000A01109010C3F (SCOM)
        //  Description      This register contains the fifth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_B_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000A01109010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address                8000A01209010C3F (SCOM)
        //  Description            This register contains the fifth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_B_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000A01209010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address                8000A01309010C3F (SCOM)
        //  Description            This register contains the fifth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_B_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000A01309010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address                8000A01409010C3F (SCOM)
        //  Description            This register contains the fifth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_B_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000A01409010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address                8000A01509010C3F (SCOM)
        //  Description            This register contains the fifth set of O controls.
        scom_and_or_for_chiplet(TGT0, 0x8000A01509010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address          8000A01609010C3F (SCOM)
        //  Description      This register contains the fifth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_B_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000A01609010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL5_O_PL
        //  Address          8000A01709010C3F (SCOM)
        //  Description      This register contains the fifth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_B_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_B_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000A01709010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address                8000C00009010C3F (SCOM)
        //  Description            This register contains the ninth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C00009010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address                8000C00109010C3F (SCOM)
        //  Description            This register contains the ninth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C00109010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address                8000C00209010C3F (SCOM)
        //  Description            This register contains the ninth set of O controls.
        scom_and_or_for_chiplet(TGT0, 0x8000C00209010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address          8000C00309010C3F (SCOM)
        //  Description      This register contains the ninth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C00309010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address          8000C00409010C3F (SCOM)
        //  Description      This register contains the ninth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C00409010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address                8000C00509010C3F (SCOM)
        //  Description            This register contains the ninth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C00509010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address                8000C00609010C3F (SCOM)
        //  Description            This register contains the ninth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C00609010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address                8000C00709010C3F (SCOM)
        //  Description            This register contains the ninth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C00709010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address          8000C00809010C3F (SCOM)
        //  Description      This register contains the ninth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C00809010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address          8000C00909010C3F (SCOM)
        //  Description      This register contains the ninth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        scom_and_or_for_chiplet(TGT0, 0x8000C00909010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //                   adjustments.
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address          8000C00A09010C3F (SCOM)
        //  Description      This register contains the ninth set of O controls.
        scom_and_or_for_chiplet(TGT0, 0x8000C00A09010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address                8000C00B09010C3F (SCOM)
        //  Description            This register contains the ninth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C00B09010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address                8000C00C09010C3F (SCOM)
        //  Description            This register contains the ninth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C00C09010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address          8000C00D09010C3F (SCOM)
        //  Description      This register contains the ninth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C00D09010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address          8000C00E09010C3F (SCOM)
        //  Description      This register contains the ninth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C00E09010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address          8000C00F09010C3F (SCOM)
        //  Description      This register contains the ninth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C00F09010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address                8000C01009010C3F (SCOM)
        //  Description            This register contains the ninth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C01009010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address                8000C01109010C3F (SCOM)
        //  Description            This register contains the ninth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C01109010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address                8000C01209010C3F (SCOM)
        //  Description            This register contains the ninth set of O controls.
        scom_and_or_for_chiplet(TGT0, 0x8000C01209010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address          8000C01309010C3F (SCOM)
        //  Description      This register contains the ninth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C01309010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address          8000C01409010C3F (SCOM)
        //  Description      This register contains the ninth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                   adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C01409010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address                8000C01509010C3F (SCOM)
        //  Description            This register contains the ninth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C01509010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address                8000C01609010C3F (SCOM)
        //  Description            This register contains the ninth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C01609010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL9_O_PL
        //  Address                8000C01709010C3F (SCOM)
        //  Description            This register contains the ninth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_INTEG_COARSE_GAIN: This is the integrator coarse-gain control used in making common mode
        //                         adjustments.
        scom_and_or_for_chiplet(TGT0, 0x8000C01709010C3F, ~PPC_BITMASK(48, 61), 0x17080);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address          8000C80009010C3F (SCOM)
        //  Description      This register contains the tenth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C80009010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address          8000C80109010C3F (SCOM)
        //  Description      This register contains the tenth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C80109010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address          8000C80209010C3F (SCOM)
        //  Description      This register contains the tenth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C80209010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address                8000C80309010C3F (SCOM)
        //  Description            This register contains the tenth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_E_CTLE_GAIN: This is the CTLE gain setting.
        if(l_TGT0_ATTR_OPTICS_CONFIG_MODE == fapi2::ENUM_ATTR_OPTICS_CONFIG_MODE_SMP)
        {
            scom_and_or_for_chiplet(TGT0, 0x8000C80309010C3F, ~PPC_BITMASK(48, 56), 0x6780);
        }
        else
        {
            scom_and_or_for_chiplet(TGT0, 0x8000C80309010C3F, ~PPC_BITMASK(48, 56), 0x1F80);
        }

        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address                8000C80409010C3F (SCOM)
        //  Description            This register contains the tenth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C80409010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address                8000C80509010C3F (SCOM)
        //  Description            This register contains the tenth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_E_CTLE_GAIN: This is the CTLE gain setting.
        if(l_TGT0_ATTR_OPTICS_CONFIG_MODE == fapi2::ENUM_ATTR_OPTICS_CONFIG_MODE_SMP)
        {
            scom_and_or_for_chiplet(TGT0, 0x8000C80609010C3F, ~PPC_BITMASK(48, 56), 0x9500);
        }
        else
        {
            scom_and_or_for_chiplet(TGT0, 0x8000C80609010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        }
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address                8000C80609010C3F (SCOM)
        //  Description            This register contains the tenth set of O controls.
        scom_and_or_for_chiplet(TGT0, 0x8000C80609010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address          8000C80709010C3F (SCOM)
        //  Description      This register contains the tenth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C80709010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address          8000C80809010C3F (SCOM)
        //  Description      This register contains the tenth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C80809010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address          8000C80909010C3F (SCOM)
        //  Description      This register contains the tenth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_E_CTLE_GAIN: This is the CTLE gain setting.
        if((l_TGT0_ATTR_OPTICS_CONFIG_MODE == fapi2::ENUM_ATTR_OPTICS_CONFIG_MODE_SMP))
        {
            scom_and_or_for_chiplet(TGT0, 0x8000C80909010C3F, ~PPC_BITMASK(48, 52), 0x9710);
        }
        else
        {
            scom_and_or_for_chiplet(TGT0, 0x8000C80909010C3F, ~PPC_BITMASK(48, 52), 0x1F10);
        }
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address                8000C80A09010C3F (SCOM)
        //  Description            This register contains the tenth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C80A09010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address                8000C80B09010C3F (SCOM)
        //  Description            This register contains the tenth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C80B09010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address                8000C80C09010C3F (SCOM)
        //  Description            This register contains the tenth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C80C09010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address                8000C80D09010C3F (SCOM)
        //  Description            This register contains the tenth set of O controls.
        scom_and_or_for_chiplet(TGT0, 0x8000C80D09010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address          8000C80E09010C3F (SCOM)
        //  Description      This register contains the tenth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C80E09010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address          8000C80F09010C3F (SCOM)
        //  Description      This register contains the tenth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C80F09010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address          8000C81009010C3F (SCOM)
        //  Description      This register contains the tenth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C81009010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address                8000C81109010C3F (SCOM)
        //  Description            This register contains the tenth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C81109010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address                8000C81209010C3F (SCOM)
        //  Description            This register contains the tenth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C81209010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address                8000C81309010C3F (SCOM)
        //  Description            This register contains the tenth set of O controls.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55       RWX         RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C81309010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#0.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address                8000C81409010C3F (SCOM)
        //  Description            This register contains the tenth set of O controls.
        scom_and_or_for_chiplet(TGT0, 0x8000C81409010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#1.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address          8000C81509010C3F (SCOM)
        //  Description      This register contains the tenth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C81509010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#2.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address          8000C81609010C3F (SCOM)
        //  Description      This register contains the tenth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C81609010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#3.RX_DAC_REGS.RX_DAC_REGS.RX_DAC_CNTL10_O_PL
        //  Address          8000C81709010C3F (SCOM)
        //  Description      This register contains the tenth set of O controls.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_E_CTLE_COARSE: This is the CTLE coarse peak value.
        // 52:55     RWX     RX_E_CTLE_GAIN: This is the CTLE gain setting.
        scom_and_or_for_chiplet(TGT0, 0x8000C81709010C3F, ~PPC_BITMASK(48, 56), 0x1D00);
        //  Register Name          Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#0.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address                8002280009010C3F (SCOM)
        //  Description            This register contains the second set of EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:55        RO          constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56          RWX         RX_PR_FW_OFF: Removes the flywheel from the phase-rotator (PR) accumulator.
        //                         Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280009010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name          Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#1.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address                8002280109010C3F (SCOM)
        //  Description            This register contains the second set of EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:55        RO          constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56          RWX         RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                         Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280109010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name          Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#2.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address                8002280209010C3F (SCOM)
        //  Description            This register contains the second set of EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:55        RO          constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56          RWX         RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                         Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280209010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name    Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#0.RXPACK.RD.SLICE#3.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address          8002280309010C3F (SCOM)
        //  Description      This register contains the second set of EO mode fields.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:55      RO      constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56        RWX     RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                   Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280309010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name    Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#0.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address          8002280409010C3F (SCOM)
        //  Description      This register contains the second set of EO mode fields.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:55      RO      constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56        RWX     RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                   Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280409010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name          Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#1.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address                8002280509010C3F (SCOM)
        //  Description            This register contains the second set of EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:55        RO          constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56          RWX         RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                         Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280509010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name          Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#2.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address                8002280609010C3F (SCOM)
        //  Description            This register contains the second set of EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:55        RO          constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56          RWX         RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                         Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280609010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name          Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#1.RXPACK.RD.SLICE#3.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address                8002280709010C3F (SCOM)
        //  Description            This register contains the second set of EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:55        RO          constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56          RWX         RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                         Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280709010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name    Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#0.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address          8002280809010C3F (SCOM)
        //  Description      This register contains the second set of EO mode fields.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:55      RO      constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56        RWX     RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                   Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280809010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name    Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#1.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address          8002280909010C3F (SCOM)
        //  Description      This register contains the second set of EO mode fields.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:55      RO      constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56        RWX     RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                   Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280909010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name          Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#2.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address                8002280A09010C3F (SCOM)
        //  Description            This register contains the second set of EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:55        RO          constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56          RWX         RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                         Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280A09010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name          Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#2.RXPACK.RD.SLICE#3.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address                8002280B09010C3F (SCOM)
        //  Description            This register contains the second set of EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:55        RO          constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56          RWX         RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                         Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280B09010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name          Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#0.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address                8002280C09010C3F (SCOM)
        //  Description            This register contains the second set of EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:55        RO          constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56          RWX         RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                         Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280C09010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name    Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#1.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address          8002280D09010C3F (SCOM)
        //  Description      This register contains the second set of EO mode fields.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:55      RO      constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56        RWX     RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                   Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280D09010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name    Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#2.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address          8002280E09010C3F (SCOM)
        //  Description      This register contains the second set of EO mode fields.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:55      RO      constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56        RWX     RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                   Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280E09010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name          Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#3.RXPACK.RD.SLICE#3.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address                8002280F09010C3F (SCOM)
        //  Description            This register contains the second set of EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:55        RO          constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56          RWX         RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                         Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002280F09010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name          Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#0.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address                8002281009010C3F (SCOM)
        //  Description            This register contains the second set of EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:55        RO          constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56          RWX         RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                         Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002281009010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name          Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#1.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address                8002281109010C3F (SCOM)
        //  Description            This register contains the second set of EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:55        RO          constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56          RWX         RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                         Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002281109010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name    Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#2.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address          8002281209010C3F (SCOM)
        //  Description      This register contains the second set of EO mode fields.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:55      RO      constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56        RWX     RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                   Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002281209010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name    Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#4.RXPACK.RD.SLICE#3.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address          8002281309010C3F (SCOM)
        //  Description      This register contains the second set of EO mode fields.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:55      RO      constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56        RWX     RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                   Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002281309010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name          Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#0.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address                8002281409010C3F (SCOM)
        //  Description            This register contains the second set of EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:55        RO          constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56          RWX         RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                         Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002281409010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name          Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#1.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address                8002281509010C3F (SCOM)
        //  Description            This register contains the second set of EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:55        RO          constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56          RWX         RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                         Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002281509010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name          Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#2.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address                8002281609010C3F (SCOM)
        //  Description            This register contains the second set of EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:55        RO          constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56          RWX         RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                         Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(TGT0, 0x8002281609010C3F, ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name    Receive Bit Mode 2 EO Per-Lane Register
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXPACKS#5.RXPACK.RD.SLICE#3.RD.RX_BIT_REGS.RX_BIT_MODE2_EO_PL
        //  Address          8002281709010C3F (SCOM)
        //  Description      This register contains the second set of EO mode fields.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:55      RO      constant = 0b00000000000000000000000000000000000000000000000000000000
        // 56        RWX     RX_PR_FW_OFF: Removes the flywheel from the PR accumulator.
        //                   Note: This is not the same as setting the inertia amount to zero.
        scom_and_or_for_chiplet(
          TGT0, 0x8002281709010C3F,
          ~PPC_BITMASK(57, 63), 0x4C);
        //  Register Name    Receive Spare Mode Per-Group Register
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXCTL.CTL_REGS.RX_CTL_REGS.RX_SPARE_MODE_PG
        //  Address          8008000009010C3F (SCOM)
        //  Description      This register contains per-group spare mode latches.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48        RWX     RX_PG_SPARE_MODE_0: Per-group spare mode latch.
        // 49        RWX     RX_PG_SPARE_MODE_1: Per-group spare mode latch.
        scom_and_for_chiplet(TGT0, 0x8008000009010C3F, ~PPC_BIT(52));
        //  Register Name          Receive ID1 Per-Group Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXCTL.CTL_REGS.RX_CTL_REGS.RX_ID1_PG
        //  Address                8008080009010C3F (SCOM)
        //  Description            This register is used to set the bus number of a clock group.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:53       RWX         RX_BUS_ID: This field is used to programmatically set the bus number that a clock group belongs to.
        // 54:63       RO          constant = 0b0000000000
        scom_and_for_chiplet(TGT0, 0x8008080009010C3F, ~PPC_BITMASK(48, 53));
        //  Register Name    Receive CTL Mode 2 EO Per-Group Register
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXCTL.CTL_REGS.RX_CTL_REGS.RX_CTL_MODE2_EO_PG
        //  Address          8008180009010C3F (SCOM)
        //  Description      This register contains the second set of receive CTL EO mode fields.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:49     RWX     RX_DFE_CA_CFG: Receive DFE clock adjustment settings. This 2-bit field contains an encoded value for K
        //                   as follows:
        if(l_TGT0_ATTR_IO_O_CHANNEL_TYPE == fapi2::ENUM_ATTR_IO_O_CHANNEL_TYPE_CABLE && l_chip_ec != 0x20)
        {
            scom_and_for_chiplet(TGT0, 0x8008180009010C3F, PPC_BIT(54) | PPC_BIT(57));
        }
        else
        {
            scom_and_for_chiplet(TGT0, 0x8008180009010C3F, ~(PPC_BIT(54) | PPC_BIT(57)));
        }
        //  Register Name          Receive CTL Mode 10 EO Per-Group Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXCTL.CTL_REGS.RX_CTL_REGS.RX_CTL_MODE10_EO_PG
        //  Address                8008580009010C3F (SCOM)
        //  Description            This register contains the tenth set of receive CTL EO mode fields.
        scom_and_or_for_chiplet(
          TGT0, 0x8008580009010C3F,
          ~PPC_BITMASK(48, 63), 0x4AD2);
        //  Register Name    Receive CTL Mode 11 EO Per-Group Register
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXCTL.CTL_REGS.RX_CTL_REGS.RX_CTL_MODE11_EO_PG
        //  Address          8008600009010C3F (SCOM)
        //  Description      This register contains the 11th set of receive CTL EO mode fields.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:50     RWX     RX_OFF_INIT_CFG: This register controls the servo filter for offset measurements during initialization.
        // 51:53     RWX     RX_OFF_RECAL_CFG: This register controls the servo filter for offset measurements during recalibration.
        scom_and_or_for_chiplet(
          TGT0, 0x8008600009010C3F,
          ~PPC_BITMASK(48, 56), 0x4900);
        //  Register Name    Receive CTL Mode 12 EO Per-Group Register
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXCTL.CTL_REGS.RX_CTL_REGS.RX_CTL_MODE12_EO_PG
        //  Address          8008680009010C3F (SCOM)
        //  Description      This register contains the 12th set of receive CTL EO mode fields.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:51     RWX     RX_SERVO_CHG_CFG: This register controls the minimum acceptable changes of the accumulator for a
        //                   valid servo operation. It is used to assure that we have reached a stable point.
        scom_and_or_for_chiplet(
          TGT0, 0x8008680009010C3F,
          ~PPC_BITMASK(60, 62), 0x10);
        //  Register Name          Receive CTL Mode 13 EO Per-Group Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXCTL.CTL_REGS.RX_CTL_REGS.RX_CTL_MODE13_EO_PG
        //  Address                8008700009010C3F (SCOM)
        //  Description            This register contains the 13th set of receive CTL EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:48        RO          constant = 0b0000000000000000000000000000000000000000000000000
        // 49:55       RWX         RX_CM_OFFSET_VAL: This field contains the value used to offset the amp DAC when running common
        //                         mode.
        scom_and_or_for_chiplet(
          TGT0, 0x8008700009010C3F,
          ~PPC_BITMASK(49, 55), 0x4600);
        //  Register Name          Receive CTL Mode 14 EO Per-Group Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXCTL.CTL_REGS.RX_CTL_REGS.RX_CTL_MODE14_EO_PG
        //  Address                8008780009010C3F (SCOM)
        //  Description            This register contains the 14th set of receive CTL EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_AMP_INIT_TIMEOUT: This field is used for amplitude measurements during initialization.
        // 52:55       RWX         RX_AMP_RECAL_TIMEOUT: This field is used for amplitude measurements during recalibration.
        scom_and_or_for_chiplet(
          TGT0, 0x8008780009010C3F,
          ~PPC_BITMASK(48, 55), 0x6600);
        //  Register Name          Receive CTL Mode 15 EO Per-Group Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXCTL.CTL_REGS.RX_CTL_REGS.RX_CTL_MODE15_EO_PG
        //  Address                8008800009010C3F (SCOM)
        //  Description            This register contains the 15th set of receive CTL EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:51       RWX         RX_OFF_INIT_TIMEOUT: This field is used for offset measurements during initialization.
        // 52:55       RWX         RX_OFF_RECAL_TIMEOUT: This field is used for offset measurements during recalibration.
        scom_and_or_for_chiplet(
          TGT0, 0x8008800009010C3F,
          ~PPC_BITMASK(48, 55), 0x6600);
        //  Register Name          Receive CTL Mode 29 EO Per-Group Register
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXCTL.CTL_REGS.RX_CTL_REGS.RX_CTL_MODE29_EO_PG
        //  Address                8008D00009010C3F (SCOM)
        //  Description            This register contains the 29th set of receive CTL EO mode fields.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:55       RWX         RX_APX111_HIGH: This field contains the receive Amax high target, in amplitude DAC steps (as measured
        //                         by ap_x111 and an_x000). The default is d102.
        scom_and_or_for_chiplet(
          TGT0, 0x8008D00009010C3F,
          ~PPC_BITMASK(48, 63), 0x785A);
        //  Register Name    Receive CTL Mode 27 EO Per-Group Register
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXCTL.CTL_REGS.RX_CTL_REGS.RX_CTL_MODE27_EO_PG
        //  Address          8009700009010C3F (SCOM)
        //  Description      This register contains the 27th set of receive CTL EO mode fields.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48        RWX     RX_RC_ENABLE_CTLE_1ST_LATCH_OFFSET_CAL: Receive recalibration; first latch offset adjustment
        //                   enable with CTLE-based disable.
        if(l_TGT0_ATTR_IO_O_CHANNEL_TYPE == fapi2::ENUM_ATTR_IO_O_CHANNEL_TYPE_CABLE && l_chip_ec != 0x20)
        {
            scom_or_for_chiplet(TGT0, 0x8009700009010C3F, PPC_BIT(51));
        }
        else
        {
            scom_and_for_chiplet(TGT0, 0x8009700009010C3F, ~PPC_BIT(51));
        }
        //  Register Name    Receive CTL Mode 28 EO Per-Group Register
        //  Mnemonic         IOO0.IOO_CPLT.RX0.RXCTL.CTL_REGS.RX_CTL_REGS.RX_CTL_MODE28_EO_PG
        //  Address          8009780009010C3F (SCOM)
        //  Description      This register contains the 28th set of receive CTL EO mode fields.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48        RWX     RX_DC_ENABLE_CM_COARSE_CAL: This bit enables receive DC calibration eye-optimization common-
        //                   mode coarse calibration.
        scom_and_for_chiplet(TGT0, 0x8009780009010C3F, ~PPC_BIT(48));
        //  Register Name          Receive CTL Mode 2 O Per-Group
        //  Mnemonic               IOO0.IOO_CPLT.RX0.RXCTL.CTL_REGS.RX_CTL_REGS.RX_CTL_MODE2_O_PG
        //  Address                8009880009010C3F (SCOM)
        //  Description            This is a per-group receive CTL mode O register.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:50       RWX         RX_OCTANT_SELECT: This field selects which c16 clock is used on IOO.
        // 51:52       RWX         RX_SPEED_SELECT: This field selects the IOO speed control.
        scom_and_or_for_chiplet(
          TGT0, 0x8009880009010C3F,
          ~PPC_BITMASK(48, 52), 0x5000);
        //  Register Name    Transmit ID1 Per-Group Register
        //  Mnemonic         IOO0.IOO_CPLT.TX0.TXCTL.CTL_REGS.TX_CTL_REGS.TX_ID1_PG
        //  Address          800C0C0009010C3F (SCOM)
        //  Description      This register is used to programmatically set the bus number that a group belongs to.
        //  Bits     SCOM    Field Mnemonic: Description
        // 0:47      RO      constant = 0b000000000000000000000000000000000000000000000000
        // 48:53     RWX     TX_BUS_ID: This field is used to programmatically set the bus number that a group belongs to.
        // 54:63     RO      constant = 0b0000000000
        scom_and_for_chiplet(TGT0, 0x800C0C0009010C3F, ~PPC_BITMASK(48, 53));
        //  Register Name          Transmit Impedance Calibration P 4X Per-Bus Register
        //  Mnemonic               IOO0.IOO_CPLT.BUSCTL.BUS_REG_IF.BUS_CTL_REGS.TX_IMPCAL_P_4X_PB
        //  Address                800F1C0009010C3F (SCOM)
        //  Description            This register is used for impedance calibration.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:52       RWX         TX_ZCAL_P_4X: Calibration circuit PSeg-4X enable value. This field holds the current value of the enabled
        //                         segments. It is a 2x multiple of the actual segment count. It can be read for the current calibration result set
        scom_and_or_for_chiplet(
          TGT0, 0x800F1C0009010C3F,
          ~PPC_BITMASK(48, 52), 0xE000);
        //  Register Name          Transmit Impedance Calibration SWO2 Per-Bus Register
        //  Mnemonic               IOO0.IOO_CPLT.BUSCTL.BUS_REG_IF.BUS_CTL_REGS.TX_IMPCAL_SWO2_PB
        //  Address                0x800F2C0009010C3F (SCOM)
        //  Description            This register is used for impedance calibration.
        //  Bits       SCOM        Field Mnemonic: Description
        // 0:47        RO          constant = 0b000000000000000000000000000000000000000000000000
        // 48:54       RWX         TX_ZCAL_SM_MIN_VAL: Impedance calibration minimum search threshold. This low-side segment count
        //                         limit is used in the calibration process. (binary code - 0x00 is zero slices, and 0x50 is maximum slices).
        // 55:61       RWX         TX_ZCAL_SM_MAX_VAL: Impedance calibration maximum search threshold. This high-side segment count
        //                         limit is used in the calibration process. (binary code - 0x00 is zero slices and 0x50 is maximum slices).
        // 62:63       RO          constant = 0b00
        scom_and_or_for_chiplet(
          TGT0, 0x800F2C0009010C3F,
          ~PPC_BITMASK(48, 61), 0x158C);
    };
}
