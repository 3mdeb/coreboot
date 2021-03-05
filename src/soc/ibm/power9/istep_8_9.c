/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/byteorder.h>
#include <console/console.h>
#include <cpu/power/istep_8_9.h>
#include <cpu/power/scom.h>
#include <cpu/power/scom_registers.h>

void istep_8_9(void)
{
    printk(BIOS_EMERG, "starting istep 8.9\n");
    report_istep(8, 9);
    p9_fbc_no_hp_scom();
    p9_fbc_ioe_tl_scom();
    ioe_tl_fir();
    p9_fbc_ioe_dl_scom();
    ioel_fir();
    printk(BIOS_EMERG, "ending istep 8.9\n");
}

void p9_fbc_ioe_dl_scom(void)
{
    // PB_ELL_CFG_REG
    // [0] CONFIG_LINK_PAIR: When 1, the two links operate as a pair.
    //     When 0, only one link is used or they are independent.
    // [2] CONFIG_CRC_LANE_ID: When 1, use lane IDs from CRC for lane sparing.
    // [4] CONFIG_SL_UE_CRC_ERR: When 1, UEs on the SL ECC cause the packet
    //     to be treated as a CRC error.
    // [8:11] CONFIG_UNUSED1: Spare bits.
    // [12:15] CONFIG_PACKET_DELAY_LIMIT: This field indicates the number
    //         of cycles of delay of frame data that are required for the
    //         packet to be considered delayed or replayed. (0 is 16 cycles.)
    // [27:27] CONFIG_AUTO_TDM_BW_DIFF: Bandwidth difference
    //         for auto-entry TMD mode.
    scom_and_or_for_chiplet(XB_CHIPLET_ID, PB_ELL_CFG_REG, ~(PPC_BIT(0) | PPC_BIT(11)), PPC_BIT(2) | PPC_BIT(4) | PPC_BITMASK(12, 15) | PPC_BITMASK(24, 27));
    // PB_ELL_REPLAY_TRESHOLD_REG
    // [0:3] THRESH_REPLAY_TB_SEL: Replay threshold timebase select.
    // [4:7] THRESH_REPLAY_TAP_SEL: Replay threshold tap select.
    // [8:10] THRESH_REPLAY_ENABLE: Replay threshold error enable.
    scom_and_or_for_chiplet(XB_CHIPLET_ID, PB_ELL_REPLAY_TRESHOLD_REG, ~PPC_BITMASK(0, 3), PPC_BITMASK(1, 2) | PPC_BITMASK(4, 10));
    // PB_ELL_SL_ECC_TRESHOLD_REG
    // [0:3] THRESH_SL_ECC_TB_SEL: SL ECC threshold timebase select.
    // [4:7] THRESH_SL_ECC_TAP_SEL: SL ECC threshold tap select.
    // [8:9] THRESH_SL_ECC_ENABLE: SL ECC threshold error enable.
    // [10:25] THRESH_SL_ECC_UNUSED1: Spare bits.
    scom_and_or_for_chiplet(XB_CHIPLET_ID, PB_ELL_SL_ECC_TRESHOLD_REG, ~PPC_BIT(0), PPC_BITMASK(1, 7) | PPC_BITMASK(9, 11));
}

void ioe_tl_fir(void)
{
    // PB_IOE_FIR_ACTION0: Processor bus IOE domain action select for corresponding bit in FIR.
    // (Action0,Action1) = Action Select.
    // (0,0) Checkstop.
    // (0,1) Recoverable Error to Service Processor.
    // (1,0) Special Attention to Service Processor.
    // (1,1) Invalid.
    write_scom(PU_PB_IOE_FIR_ACTION0_REG, FBC_IOE_TL_FIR_ACTION0);
    // PB_IOE_FIR_ACTION1: Processor bus FIR LSB of action select for corresponding bit in FIR.
    // (Action0,Action1) = Action Select.
    // (0,0) Checkstop.
    // (0,1) Recoverable Error to Service Processor.
    // (1,0) Special Attention to Service Processor.
    // (1,1) Invalid.
    write_scom(PU_PB_IOE_FIR_ACTION1_REG, FBC_IOE_TL_FIR_ACTION1);
    // PU_PB_IOE_FIR_MASK_REG: Power Bus PBEN IOX Domain FIR0 Mask Register
    // [0:5] FMR[0:5]_TRAINED_MASK fmr[0:5] trained.
    // [6:7] RSV[6:7]_MASK rsv[6:7] mask.
    // [8] DOB01_UE_MASK: dob01 ue mask.
    // [9] DOB01_CE_MASK: dob01 ce mask.
    // [10] DOB01_SUE_MASK: dob01 sue mask.
    // [11] DOB23_UE_MASK: dob23 ue mask.
    // [12] DOB23_CE_MASK: dob23 ce mask.
    // [13] DOB23_SUE_MASK: dob23 sue mask.
    // [14] DOB45_UE_MASK: dob45 ue mask.
    // [15] DOB45_CE_MASK: dob45 ce mask.
    // [16] DOB45_SUE_MASK: dob45 sue mask.
    // [17:19] RSV17_MASK: rsv[17:19] mask.
    // [20:25] FRAMER[0:5]_ATTN_MASK: framer[0:5] attn mask.
    // [26:27] RSV[26:27]_MASK: rsv[26:27] mask.
    // [28:33] PARSER[0:5]_ATTN_MASK: parser[0:5] attn mask.
    // [34:35] RSV[34:35]_MASK: rsv[34:35] mask.
    // [36:37] MB[0:1]_SPATTN_MASK: mb[0:1] spattn mask.
    // [38:39] MB[10:11]_SPATTN_MASK: mb[10:11] spattn mask.
    // [40:41] MB[20:21]_SPATTN_MASK: mb[20:21] spattn mask.
    // [42:43] MB[30:31]_SPATTN_MASK: mb[30:31] spattn mask.
    // [44:45] MB[40:41]_SPATTN_MASK: mb[40:41] spattn mask.
    // [46:47] MB[50:51]_SPATTN_MASK: mb[50:51] spattn mask.
    // [48:51] Reserved.
    // [52] DOB01_ERR_MASK: data outbound switch internal error mask - links 01.
    // [53] DOB23_ERR_MASK: data outbound switch internal error mask - links 23.
    // [54] DOB45_ERR_MASK: data outbound switch internal error mask - links 45.
    // [55] Reserved.
    // [56] DIB01_ERR_MASK: data inbound switch internal error mask - links 01.
    // [57] DIB23_ERR_MASK: data inbound switch internal error mask - links 23.
    // [58] DIB45_ERR_MASK: data inbound switch internal error mask - links 45.
    // [59:61] Reserved.
    // [62] FIR_SCOM_ERR_MASK_DUP: FIR SCOM err mask dup.
    // [63] FIR_SCOM_ERR_MASK: FIR SCOM err mask.
    write_scom(PU_PB_IOE_FIR_MASK_REG, FBC_IOE_TL_FIR_MASK | FBC_IOE_TL_FIR_MASK_X0_NF);
}

void ioel_fir(void)
{
    // IOEL_FIR_ACTION0: processor bus FIR MSB of action select for corresponding bit in FIR.
    // (Action0,Action1) = Action Select.
    // (0,0) = Checkstop.
    // (0,1) = Recoverable error to service processor.
    // (1,0) = Special attention to service processor.
    // (1,1) = Invalid.
    write_scom_for_chiplet(XB_CHIPLET_ID, XBUS_LL0_IOEL_FIR_ACTION0_REG, FBC_IOE_DL_FIR_ACTION0);
    // IOEL_FIR_ACTION1: processor bus FIR LSB of action select for corresponding bit in FIR.
    // (Action0,Action1) = Action Select.
    // (0,0) = Checkstop.
    // (0,1) = Recoverable error to service processor.
    // (1,0) = Special attention to service processor.
    // (1,1) = Invalid.
    write_scom_for_chiplet(XB_CHIPLET_ID, XBUS_LL0_IOEL_FIR_ACTION1_REG, FBC_IOE_DL_FIR_ACTION1);
    // XBUS_LL0_IOEL_FIR_MASK_REG: This register provides a bit mask for the ELL FIR Register.
    // [0:1] FIR_LINK[0:1]_TRAINED_MASK: Link [0:1] trained mask.
    // [2:3] RESERVED.
    // [4:5] FIR_LINK[0:1]_REPLAY_THRESHOLD_MASK: Link [0:1] replay threshold mask.
    // [6:7] FIR_LINK[0:1]_CRC_ERROR_MASK: Link [0:1] CRC error mask.
    // [8:9] FIR_LINK[0:1]_NAK_RECEIVED_MASK: Link [0:1] NAK received mask.
    // [10:11] FIR_LINK[0:1]_REPLAY_BUFFER_FULL_MASK: Link [0:1] replay buffer full mask.
    // [12:13] FIR_LINK[0:1]_SL_ECC_THRESHOLD_MASK: Link [0:1] SL ECC threshold mask.
    // [14:15] FIR_LINK[0:1]_SL_ECC_CORRECTABLE_MASK: Link [0:1] SL ECC correctable mask.
    // [16:17] FIR_LINK[0:1]_SL_ECC_UE_MASK: Link [0:1] SL ECC UE mask.
    // [18:39] RESERVED.
    // [40:41] FIR_LINK[0:1]_TCOMPLETE_BAD_MASK: Link [0:1] complete bad mask.
    // [42:43] RESERVED.
    // [44:45] FIR_LINK[0:1]_SPARE_DONE_MASK: Link [0:1] spare done mask.
    // [46:47] FIR_LINK[0:1]_TOO_MANY_CRC_ERRORS_MASK: Link [0:1] too many CRC errors mask.
    // [48:50] RESERVED.
    // [51] FIR_PSAVE_INVALID_STATE_MASK: Psave invalid state mask.
    // [52:53] FIR_LINK[0:1]_CORRECTABLE_ARRAY_ERROR_MASK: Link [0:1] correctable array error mask.
    // [54:55] FIR_LINK[0:1]_UNCORRECTABLE_ARRAY_ERROR_MASK: Link [0:1] uncorrectable array error mask.
    // [56:57] FIR_LINK[0:1]_TRAINING_FAILED_MASK: Link [0:1] training failed mask.
    // [58:59] FIR_LINK[0:1]_UNRECOVERABLE_ERROR_MASK: Link [0:1] unrecoverable error mask.
    // [60:61] FIR_LINK[0:1]_INTERNAL_ERROR_MASK: Link [0:1] internal error mask.
    // [62] FIR_SCOM_ERR_DUP_MASK: FIR SCOM error dup mask.
    // [63] FIR_SCOM_ERR_MASK: FIR SCOM error mask.
    write_scom_for_chiplet(XB_CHIPLET_ID, XBUS_LL0_IOEL_FIR_MASK_REG, FBC_IOE_DL_FIR_MASK);
}

void p9_fbc_no_hp_scom(void)
{
    // PB_WEST_MODE_CFG_REG: Power Bus west mode configuration register
    // [16:22] PB_CFG_SP_HW_MARK: configures the maximum system pumps an IO unit may issue.
    // [23:29] PB_CFG_GP_HW_MARK: configures the maximum group pumps this chip may issue.
    // [30:35] PB_CFG_LCL_HW_MARK: configures the maximum local pumps this chip may issue.
    scom_and_or(PB_WEST_MODE_CFG_REG, ~PPC_BITMASK(16, 35), 0xFAFEA0000000);
    // PB_CENT_MODE_CFG_REG: Power Bus PB CENT Mode Register
    // [16:22] PB_CFG_SP_HW_MARK: Configures the maximum system pumps an IO unit may issue.
    // [23:29] PB_CFG_GP_HW_MARK: Configures the maximum group pumps this chip may issue.
    // [30:35] PB_CFG_LCL_HW_MARK: Configures the maximum local pumps this chip may issue.
    scom_and_or(PB_CENT_MODE_CFG_REG, ~PPC_BITMASK(16, 35), 0x7EFEA0000000);
    // PB_CENT_GP_COMMAND_RATE_DP0_REG: Power Bus PB CENT GP command RATE DP0 Register
    // PB_CFG_GP_CMD_RATE_DP0_LVL[0:7]: Configures the command rate for group pump drop priority 0 at level [0:7].
    write_scom(PB_CENT_GP_COMMAND_RATE_DP0_REG, 0);
    // PB_CENT_GP_COMMAND_RATE_DP1_REG: Power Bus PB CENT GP command RATE DP1 Register
    // PB_CFG_GP_CMD_RATE_DP1_LVL[0:7]: Configures the command rate for group pump drop priority 1 at level [0:7].
    write_scom(PB_CENT_GP_COMMAND_RATE_DP1_REG, 0);
    // PB_CENT_RGP_COMMAND_RATE_DP0_REG: Power Bus PB CENT RGP command RATE DP0 Register
    // RWX PB_CFG_RNS_CMD_RATE_DP0_LVL[0:7]: Configures the command rate for remote group pump drop priority 0 at level [0:7].
    write_scom(PB_CENT_RGP_COMMAND_RATE_DP0_REG, 0x30406080A0C1218);
    // PB_CENT_RGP_COMMAND_RATE_DP1_REG: Power Bus PB CENT RGP command RATE DP1 Register
    // PB_CFG_RNS_CMD_RATE_DP1_LVL[0:7]: Configures the command rate for remote group pump drop priority 1 at level [0:7].
    write_scom(PB_CENT_RGP_COMMAND_RATE_DP1_REG, 0x40508080A0C1218);
    // PB_CENT_SP_COMMAND_RATE_DP0_REG: Power Bus PB CENT SP command RATE DP0 Register
    // PB_CFG_VG_CMD_RATE_DP0_LVL[0:7]: Configures the command rate for system pump drop priority 0 at level [0:7].
    write_scom(PB_CENT_SP_COMMAND_RATE_DP0_REG, 0x30406080A0C1218);
    // PB_CENT_SP_COMMAND_RATE_DP1_REG: Power Bus PB CENT SP command RATE DP1 Register
    // PB_CFG_VG_CMD_RATE_DP1_LVL[0:7]: Configures the command rate for system pump drop priority 1 at level [0:7].
    write_scom(PB_CENT_SP_COMMAND_RATE_DP1_REG, 0x30406080A0C1218);
    // PB_EAST_MODE_CFG_REG: Power Bus PB East Mode Configuration Register
    // [0] pb_east_pbixxx_init
    // [1:3] spare
    // [4] PB_CFG_CHIP_IS_SYSTEM: configures whether there are other POWER9s in the system.
    // [5:7] spare
    // [8] PB_CFG_HNG_CHK_DISABLE: Hang Check Disable.
    // [9] PB_DBG_CLR_MAX_HANG_STAGE: Resets the maximum hang state level (pb_hang_level).
    // [10:11] spare 12:15 pb_cfg_east_sw_ab_wait(0:3)
    // [12:15] PB_CFG_SW_AB_WAIT: Adds delay to tc_pb_switch_ab input from TPC during hot plug sequence.
    scom_and_or(PB_EAST_MODE_CFG_REG, 0xF1FF00000FFFFFFF, 0x7EFE0000000);
}

void p9_fbc_ioe_tl_scom(void)
{
    // PB_ELE_PB_FRAMER_PARSER_01_CFG_REG Power Bus Electrical Framer/Parser 01 Configuration Register
    // [0] FP0_CREDIT_PRIORITY_4_NOT_8: fp0 credit priority 4 not 8.
    // [1] FP0_DISABLE_GATHERING: fp0 disable data gathering.
    // [2] FP0_DISABLE_CMD_COMPRESSION: fp0 disable command compression.
    // [3] FP0_DISABLE_PRSP_COMPRESSION: fp0 disable prsp compression.
    // [4:11] FP0_LL_CREDIT_LO_LIMIT: fp0 ll credit low limit - normal frames in flight limit. Set to ROUNDUP(25.5 - (nest freq/elec freq)*10.75)).
    // [12:19] FP0_LL_CREDIT_HI_LIMIT: fp0 ll credit high limit - frames in flight limit during stop_cmds/replay.
    // [20] FP0_FMR_DISABLE: fp0 framer disable - turn the framer clocks OFF.
    // [21] FP0_FMR_SPARE: fp0 framer spare.
    // [22:23] FP01_CMD_EXP_TIME: obs/fmr/prs command expiration time = (value * 2) + 9.
    // [24] FP0_RUN_AFTER_FRAME_ERROR: fp0 run after frame error.
    // [25] FP0_PRS_DISABLE: fp0 parser disable - turn the parser clocks OFF.
    // [26:31] FP0_PRS_SPARE: fp0 parser spare.
    // [32] FP1_CREDIT_PRIORITY_4_NOT_8: fp1 credit priority 4 not 8.
    // [33] FP1_DISABLE_GATHERING: fp1 disable data gathering.
    // [34] FP1_DISABLE_CMD_COMPRESSION: fp1 disable command compression.
    // [35] FP1_DISABLE_PRSP_COMPRESSION: fp1 disable prsp compression.
    // [36:43] FP1_LL_CREDIT_LO_LIMIT: fp1 ll credit low limit - normal frames in flight limit. Set to ROUNDUP(25.5 - (nest freq/elec freq)*10.75)).
    // [44:51] FP1_LL_CREDIT_HI_LIMIT: fp1 ll credit high limit - frames in flight limit during stop_cmds/replay.
    // [52] FP1_FMR_DISABLE: fp1 framer disable - turn the framer clocks OFF.
    // [53:55] FP1_FMR_SPARE: fp1 framer spare.
    // [56] FP1_RUN_AFTER_FRAME_ERROR: fp1 run after frame error.
    // [57] FP1_PRS_DISABLE: fp1 parser disable - turn the parser clocks OFF.
    // [58:63] FP1_PRS_SPARE: fp1 parser spare.
    scom_and_or(PB_ELE_PB_FRAMER_PARSER_01_CFG_REG, 0xfff004fffff007bf, 0x2010000020000);
    // PB_ELE_PB_DATA_BUFF_01_CFG_REG: Power Bus Electrical Link Data Buffer 01 Configuration Register
    // [0] Reserved.
    // [1:7] PB_CFG_LINK0_DOB_LIMIT: pb configuration link0 dob limit - total link credits avail.
    // [8] Reserved.
    // [9:15] PB_CFG_LINK0_DOB_VC0_LIMIT: pb configuration link0 dob vc0 limit - vc0 link credits max.
    // [16] Reserved.
    // [17:23] PB_CFG_LINK0_DOB_VC1_LIMIT: pb configuration link0 dob vc1 limit - vc1 link credits max.
    // [24:28] PB_CFG_LINK01_DIB_VC_LIMIT: pb configuration link01 dib vc limit - limit per VC for data inbound to
    // pbien/s (set to 31/16/8 for 1/2/4 channels in use) (both links use same credit pool).
    // [29:32] Reserved.
    // [33:39] PB_CFG_LINK1_DOB_LIMIT: pb configuration link1 dob limit - total link credits avail.
    // [40] Reserved.
    // [41:47] PB_CFG_LINK1_DOB_VC0_LIMIT: pb configuration link1 dob vc0 limit - vc0 link credits max.
    // [48] Reserved.
    // [49:55] PB_CFG_LINK1_DOB_VC1_LIMIT: pb configuration link1 dob vc1 limit - vc1 link credits max.
    scom_and_or(PB_ELE_PB_DATA_BUFF_01_CFG_REG, 0x808080FF808080FF, 0x403C3C1F403C3C00);
    // PB_ELE_LINK_TRACE_CFG_REG:Power Bus Electrical Link Trace Configuration Register
    // [0:3] LINK00_HI_TRACE_CFG: link00 high trace configuration.
    // [4:7] LINK00_LO_TRACE_CFG: link00 low trace configuration.
    // [8:11] LINK01_HI_TRACE_CFG: link01 high trace configuration.
    // [12:15] LINK01_LO_TRACE_CFG: link01 low trace configuration.
    scom_and_or(PB_ELE_LINK_TRACE_CFG_REG, 0x00FFFFFFFFFFFF, 0x4141000000000000);
    // PB_ELE_PB_FRAMER_PARSER_23_CFG_REG: Power Bus Electrical Framer/Parser 23 Configuration Register
    // [20] FP2_FMR_DISABLE: fp2 framer disable - turn the framer clocks OFF.
    // [25] FP2_PRS_DISABLE: fp2 parser disable - turn the parser clocks OFF.
    // [52] FP3_FMR_DISABLE: fp3 framer disable - turn the framer clocks OFF.
    // [57] FP3_PRS_DISABLE: fp3 parser disable - turn the parser clocks OFF.
    scom_or(PB_ELE_PB_FRAMER_PARSER_23_CFG_REG, PPC_BIT(20) | PPC_BIT(25) | PPC_BIT(52) | PPC_BIT(57));
    // PB_ELE_PB_FRAMER_PARSER_45_CFG_REG Power Bus Electrical Framer/Parser 45 Configuration Register
    // [20] FP4_FMR_DISABLE: fp4 framer disable - turn the framer clocks OFF.
    // [25] FP4_PRS_DISABLE: fp4 parser disable - turn the parser clocks OFF.
    // [52] FP5_FMR_DISABLE: fp5 framer disable - turn the framer clocks OFF.
    // [57] FP5_PRS_DISABLE: fp5 parser disable - turn the parser clocks OFF.
    scom_or(PB_ELE_PB_FRAMER_PARSER_45_CFG_REG, PPC_BIT(20) | PPC_BIT(25) | PPC_BIT(52) | PPC_BIT(57));
    // PB_ELE_MISC_CFG_REG: Power Bus Electrical Miscellaneous Configuration Register
    // [0] PB_CFG_IOE01_IS_LOGICAL_PAIR: pb configuration ioe01 is logical pair.
    // [1] PB_CFG_IOE23_IS_LOGICAL_PAIR: pb configuration ioe23 is logical pair.
    // [2] PB_CFG_IOE45_IS_LOGICAL_PAIR: pb configuration ioe45 is logical pair.
    scom_and_or(PB_ELE_MISC_CFG_REG, ~PPC_BITMASK(1, 2), PPC_BIT(0));
}
