/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <program_loading.h>

#include <cpu/power/scom.h>

#define X0_ENABLED (true)
#define X1_ENABLED (true)
#define X2_ENABLED (true)
#define X0_IS_PAIRED (true)
#define X1_IS_PAIRED (true)
#define X2_IS_PAIRED (true)
#define DD2X_PARTS (true)
#define OPTICS_IS_A_BUS (true)
#define ATTR_PROC_FABRIC_A_LINKS_CNFG (0)
#define ATTR_PROC_FABRIC_X_LINKS_CNFG (0)

#define EPS_TYPE_LE (0x01),
#define EPS_TYPE_HE (0x02),
#define EPS_TYPE_HE_F8 (0x03)
#define ATTR_PROC_EPS_TABLE_TYPE (EPS_TYPE_LE) // default value

#define CHIP_IS_NODE (0x01)
#define CHIP_IS_GROUP (0x02)
#define ATTR_PROC_FABRIC_PUMP_MODE (CHIP_IS_NODE) // default value

// Power Bus PB West Mode Configuration Register
#define PB_WEST_MODE_CFG (0x501180A)
// Power Bus PB CENT Mode Register
#define PB_CENT_MODE_CFG (0x5011C0A)
// Power Bus PB CENT GP command RATE DP0 Register
#define PB_CENT_GP_COMMAND_RATE_DP0 (0x5011C26)
// Power Bus PB CENT GP command RATE DP1 Register
#define PB_CENT_GP_COMMAND_RATE_DP1 (0x5011C27)
// Power Bus PB CENT RGP command RATE DP0 Register
#define PB_CENT_RGP_COMMAND_RATE_DP0 (0x5011C28)
// Power Bus PB CENT RGP command RATE DP1 Register
#define PB_CENT_RGP_COMMAND_RATE_DP1 (0x5011C29)
// Power Bus PB CENT SP command RATE DP0 Register
#define PB_CENT_SP_COMMAND_RATE_DP0 (0x5011C2A)
// Power Bus PB CENT SP command RATE DP1 Register
#define PB_CENT_SP_COMMAND_RATE_DP1 (0x5011C2B)
// Power Bus PB East Mode Configuration Register
#define PB_EAST_MODE_CFG (0x501200A)
// Power bus Electrical Framer/Parser 01 configuration register
#define PB_ELE_PB_FRAMER_PARSER_01_CFG (0x501340A)
// Power Bus Electrical Framer/Parser 23 Configuration Register
#define PB_ELE_PB_FRAMER_PARSER_23_CFG (0x501340B)
// Power Bus Electrical Framer/Parser 45 Configuration Register
#define PB_ELE_PB_FRAMER_PARSER_45_CFG (0x501340C)
// Power Bus Electrical Link Data Buffer 01 Configuration Register
#define PB_ELE_PB_DATA_BUFF_01_CFG (0x5013410)
// Power Bus Electrical Link Data Buffer 23 Configuration Register
#define PB_ELE_PB_DATA_BUFF_23_CFG (0x5013411)
// Power Bus Electrical Link Data Buffer 45 Configuration Register
#define PB_ELE_PB_DATA_BUFF_45_CFG (0x5013412)
// Power Bus Electrical Miscellaneous Configuration Register
#define PB_ELE_MISC_CFG (0x5013423)
// Power Bus Electrical Link Trace Configuration Register
#define PB_ELE_LINK_TRACE_CFG (0x5013424)

#define FBC_IOE_TL_FIR_ACTION0 (0x0000000000000000ULL)
#define FBC_IOE_TL_FIR_ACTION1 (0x0049000000000000ULL)
#define PU_PB_IOE_FIR_ACTION0_REG (0x5013406)
#define PU_PB_IOE_FIR_ACTION1_REG (0x5013407)
#define FBC_IOE_TL_FIR_MASK (0xFF24F0303FFFF11FULL)
#define P9_FBC_UTILS_MAX_ELECTRICAL_LINKS (3)
#define FBC_IOE_TL_FIR_MASK_X0_NF (0x00C00C0C00000880ULL)
#define FBC_IOE_TL_FIR_MASK_X1_NF (0x0018030300000440ULL)
#define FBC_IOE_TL_FIR_MASK_X2_NF (0x000300C0C0000220ULL)
#define PU_PB_IOE_FIR_MASK_REG (0x5013403)

uint64_t xbusChiplets[] = {0x06};
#define ARRAY_LENGTH(a) (sizeof(a)/sizeof(*a))

#define FBC_IOE_DL_FIR_ACTION0 (0)
#define FBC_IOE_DL_FIR_ACTION1 (0x303c00000001ffc)
#define FBC_IOE_DL_FIR_MASK (0xfcfc3fffffffe003)
#define XBUS_LL0_IOEL_FIR_ACTION0_REG (0x6011806)
#define XBUS_LL0_IOEL_FIR_ACTION1_REG (0x6011807)
#define XBUS_LL0_LL0_LL0_IOEL_FIR_MASK_REG (0x6011803)

#define PB_ELE_CONFIG_REG (0x601180A)
#define PB_ELE_REPLAY_TRESHOLD_REG (0x6011818)
#define PB_ELE_ECC_TRESHOLD_REG (0x6011819)

#define BOTH (0x0)
#define EVEN_ONLY (0x1)
#define ODD_ONLY (0x2)
#define NONE (0x3)
#define ATTR_LINK_TRAIN BOTH

void p9_fbc_ioe_tl_scom(void);
void istep89(void);
void p9_fbc_no_hp_scom(void);
void tl_fir(void);
void p9_fbc_ioe_dl_scom(uint64_t TGT0, uint64_t &TGT1);

void istep89(void)
{
	printk(BIOS_EMERG, "starting istep 8.9\n");
	p9_fbc_no_hp_scom();
    p9_fbc_ioe_tl_scom();

    // setup IOE (XBUS FBC IO) DL SCOMs
    for(size_t xbusChipletIndex = 0; xbusChipletIndex < ARRAY_LENGTH(xbusChiplets); ++xbusChipletIndex)
    {
        p9_fbc_ioe_dl_scom(xbusChiplets[xbusChipletIndex], i_target);
        write_scom_for_chiplet(xbusChipletIndex, XBUS_LL0_IOEL_FIR_ACTION0_REG, FBC_IOE_DL_FIR_ACTION0);
        write_scom_for_chiplet(xbusChipletIndex, XBUS_LL0_IOEL_FIR_ACTION1_REG, FBC_IOE_DL_FIR_ACTION1);
        write_scom_for_chiplet(xbusChipletIndex, XBUS_LL0_LL0_LL0_IOEL_FIR_MASK_REG, FBC_IOE_DL_FIR_MASK);
    }

    // set FBC optics config mode attribute
    l_obus_chiplets = i_target.getChildren<fapi2::TARGET_TYPE_OBUS>();

    for l_iter in l_obus_chiplets:
    {
        uint8_t l_unit_pos;
        l_unit_pos = fapi2::ATTR_CHIP_UNIT_POS[l_iter];
        l_fbc_optics_cfg_mode[l_unit_pos] = fapi2::ATTR_OPTICS_CONFIG_MODE[l_iter];
    }
    fapi2::ATTR_PROC_FABRIC_OPTICS_CONFIG_MODE[i_target] = l_fbc_optics_cfg_mode;
	printk(BIOS_EMERG, "ending istep 8.9\n");
}

void p9_fbc_ioe_dl_scom(uint64_t TGT0, uint64_t TGT1)
{
    // REGISTERS read
    uint64_t pb_ioe_ll0_ioel_config = read_scom_for_chiplet(TGT0, 0x601180A); // ELL Configuration Register
    uint64_t pb_ioe_ll0_ioel_replay_threshold = read_scom_for_chiplet(TGT0, 0x6011818); // ELL Replay Threshold Register
    uint64_t pb_ioe_ll0_ioel_sl_ecc_threshold = read_scom_for_chiplet(TGT0, 0x6011819); // ELL SL ECC Threshold Register
    if (ATTR_LINK_TRAIN == BOTH)
    {
        pb_ioe_ll0_ioel_config |= 0x8000000000000000;
    }
    else
    {
        pb_ioe_ll0_ioel_config &= 0x7FFFFFFFFFFFFFFF;
    }
    pb_ioe_ll0_ioel_config &= 0xFFEFFFFFFFFFFFFF;
    pb_ioe_ll0_ioel_config |= 0x280F000F00000000;
    pb_ioe_ll0_ioel_sl_ecc_threshold |= 0x0070000000000000;
    pb_ioe_ll0_ioel_replay_threshold &= 0x0FFFFFFFFFFFFFFF;
    pb_ioe_ll0_ioel_replay_threshold |= 0x6FE0000000000000;

    pb_ioe_ll0_ioel_sl_ecc_threshold &= 0x7FFFFFFFFFFFFFFF;
    pb_ioe_ll0_ioel_sl_ecc_threshold |= 0x7F00000000000000;
    // REGISTERS write
    TGT0[0x601180A] = pb_ioe_ll0_ioel_config;
    TGT0[0x6011818] = pb_ioe_ll0_ioel_replay_threshold;
    TGT0[0x6011819] = pb_ioe_ll0_ioel_sl_ecc_threshold;
}

void tl_fir(void)
{
    write_scom(PU_PB_IOE_FIR_ACTION0_REG, FBC_IOE_TL_FIR_ACTION0);
    write_scom(PU_PB_IOE_FIR_ACTION1_REG, FBC_IOE_TL_FIR_ACTION1);
    uint64_t l_fir_mask = FBC_IOE_TL_FIR_MASK;

    if (len(l_xbusChiplets) == 0)
    {
        // no valid links, mask
        // l_fir_mask.flush<1>();
        l_fir_mask = 0xFFFFFFFFFFFFFFFF;
    }
    else
    {
        bool l_x_functional[P9_FBC_UTILS_MAX_ELECTRICAL_LINKS] =
        {
            false,
            false,
            false
        };
        uint64_t l_x_non_functional_mask[P9_FBC_UTILS_MAX_ELECTRICAL_LINKS] =
        {
            FBC_IOE_TL_FIR_MASK_X0_NF,
            FBC_IOE_TL_FIR_MASK_X1_NF,
            FBC_IOE_TL_FIR_MASK_X2_NF
        };
        for l_iter in l_xbusChiplets:
        {
            uint8_t l_unit_pos;
            l_unit_pos = fapi2::ATTR_CHIP_UNIT_POS[l_iter];
            l_x_functional[l_unit_pos] = true;
        }

        for(unsigned int ll = 0; ll < P9_FBC_UTILS_MAX_ELECTRICAL_LINKS; ++ll)
        {
            if (!l_x_functional[ll])
            {
                l_fir_mask |= l_x_non_functional_mask[ll];
            }
        }
        // shoudn't this happen for no xbus chiplets too?
        write_scom(PU_PB_IOE_FIR_MASK_REG, l_fir_mask);
    }
}

void p9_fbc_no_hp_scom(void)
{
    uint64_t is_flat_8 = false;

    // REGISTERS read
    uint64_t pb_com_pb_west_mode = read_scom(PB_WEST_MODE_CFG);
    uint64_t pb_com_pb_cent_mode = read_scom(PB_CENT_MODE_CFG);
    uint64_t pb_com_pb_cent_gp_cmd_rate_dp0 = read_scom(PB_CENT_GP_COMMAND_RATE_DP0);
    uint64_t pb_com_pb_cent_gp_cmd_rate_dp1 = read_scom(PB_CENT_GP_COMMAND_RATE_DP1);
    uint64_t pb_com_pb_cent_rgp_cmd_rate_dp0 = read_scom(PB_CENT_RGP_COMMAND_RATE_DP0);
    uint64_t pb_com_pb_cent_rgp_cmd_rate_dp1 = read_scom(PB_CENT_RGP_COMMAND_RATE_DP1);
    uint64_t pb_com_pb_cent_sp_cmd_rate_dp0 = read_scom(PB_CENT_SP_COMMAND_RATE_DP0);
    uint64_t pb_com_pb_cent_sp_cmd_rate_dp1 = read_scom(PB_CENT_SP_COMMAND_RATE_DP1);
    uint64_t pb_com_pb_east_mode = read_scom(PB_EAST_MODE_CFG);

    if (ATTR_PROC_FABRIC_PUMP_MODE == CHIP_IS_GROUP
    || (ATTR_PROC_FABRIC_PUMP_MODE != CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG == 0))
    {
        pb_com_pb_cent_gp_cmd_rate_dp0 = 0;
        pb_com_pb_cent_gp_cmd_rate_dp1 = 0;
    }
    else if (ATTR_PROC_FABRIC_PUMP_MODE != CHIP_IS_GROUP
          && ATTR_PROC_FABRIC_X_LINKS_CNFG > 0 && ATTR_PROC_FABRIC_X_LINKS_CNFG < 3)
    {
        pb_com_pb_cent_gp_cmd_rate_dp0 = 0x030406171C243448;
        pb_com_pb_cent_gp_cmd_rate_dp1 = 0x040508191F283A50;
    }
    else if (ATTR_PROC_FABRIC_PUMP_MODE != CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG > 2)
    {
        pb_com_pb_cent_gp_cmd_rate_dp0 = 0x0304062832405C80;
        pb_com_pb_cent_gp_cmd_rate_dp1 = 0x0405082F3B4C6D98;
    }

    if (ATTR_PROC_FABRIC_PUMP_MODE == CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG == 0)
    {
        pb_com_pb_cent_rgp_cmd_rate_dp0 = 0;
        pb_com_pb_cent_rgp_cmd_rate_dp1 = 0;
        pb_com_pb_cent_sp_cmd_rate_dp0 = 0;
        pb_com_pb_cent_sp_cmd_rate_dp1 = 0;
    }
    else if (ATTR_PROC_FABRIC_PUMP_MODE == CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG > 0 && ATTR_PROC_FABRIC_X_LINKS_CNFG < 3)
    {
        pb_com_pb_cent_rgp_cmd_rate_dp0 = 0x030406080A0C1218;
        pb_com_pb_cent_rgp_cmd_rate_dp1 = 0x040508080A0C1218;
        pb_com_pb_cent_sp_cmd_rate_dp0 = 0x030406080A0C1218;
        pb_com_pb_cent_sp_cmd_rate_dp1 = 0x030406080A0C1218;
    }
    else if ((ATTR_PROC_FABRIC_PUMP_MODE == CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG == 3)
          || (ATTR_PROC_FABRIC_PUMP_MODE != CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG == 0))
    {
        pb_com_pb_cent_rgp_cmd_rate_dp0 = 0x0304060D10141D28;
        pb_com_pb_cent_rgp_cmd_rate_dp1 = 0x0405080D10141D28;
        pb_com_pb_cent_sp_cmd_rate_dp0 = 0x05070A0D10141D28;
        pb_com_pb_cent_sp_cmd_rate_dp1 = 0x05070A0D10141D28;
    }
    else if ((ATTR_PROC_FABRIC_PUMP_MODE == CHIP_IS_GROUP && is_flat_8)
          || (ATTR_PROC_FABRIC_PUMP_MODE != CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG > 0 && ATTR_PROC_FABRIC_X_LINKS_CNFG < 3))
    {
        pb_com_pb_cent_rgp_cmd_rate_dp0 = 0x030406171C243448;
        pb_com_pb_cent_rgp_cmd_rate_dp1 = 0x040508191F283A50;
        pb_com_pb_cent_sp_cmd_rate_dp0 = 0x080C12171C243448;
        pb_com_pb_cent_sp_cmd_rate_dp1 = 0x0A0D14191F283A50;
    }
    else if (ATTR_PROC_FABRIC_PUMP_MODE != CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG > 2)
    {
        pb_com_pb_cent_rgp_cmd_rate_dp0 = 0x0304062832405C80;
        pb_com_pb_cent_rgp_cmd_rate_dp1 = 0x0405082F3B4C6D98;
        pb_com_pb_cent_sp_cmd_rate_dp0 = 0x08141F2832405C80;
        pb_com_pb_cent_sp_cmd_rate_dp1 = 0x0A18252F3B4C6D98;
    }

    if (ATTR_PROC_FABRIC_X_LINKS_CNFG == 0 && ATTR_PROC_FABRIC_A_LINKS_CNFG == 0)
    {
        pb_com_pb_east_mode |= 0x0300000000000000;
    }
    else
    {
        pb_com_pb_east_mode &= 0xF1FFFFFFFFFFFFFF;
    }
    if (ATTR_PROC_FABRIC_PUMP_MODE != CHIP_IS_GROUP && is_flat_8)
    {
        pb_com_pb_west_mode &= 0xFFFF0003FFFFFFFF;
        pb_com_pb_west_mode |= 0x00003E8000000000;
        pb_com_pb_cent_mode &= 0xFFFF0003FFFFFFFF;
        pb_com_pb_cent_mode |= 0x00003E8000000000;
        pb_com_pb_east_mode &= 0xFFFF0003FFFFFFFF;
        pb_com_pb_east_mode |= 0x00003E8000000000;
    }
    else
    {
        pb_com_pb_west_mode &= 0xFFFF0003FFFFFFFF;
        pb_com_pb_west_mode |= 0x0000FAFC00000000;
        pb_com_pb_cent_mode &= 0xFFFF0003FFFFFFFF;
        pb_com_pb_cent_mode |= 0x00007EFC00000000;
        pb_com_pb_east_mode &= 0xFFFF0003FFFFFFFF;
        pb_com_pb_east_mode |= 0x000007EFC0000000;
    }

    pb_com_pb_west_mode &= 0xFFFFFFFC0FFFFFFF;
    pb_com_pb_west_mode |= 0x00000002a0000000;
    pb_com_pb_cent_mode &= 0xFFFFFFFC0FFFFFFF;
    pb_com_pb_cent_mode |= 0x00000002a0000000;
    pb_com_pb_east_mode &= 0xFFFFFFFC0FFFFFFF;
    pb_com_pb_east_mode |= 0x00000002a0000000;

    // REGISTERS write
    write_scom(PB_WEST_MODE_CFG, pb_com_pb_west_mode);
    write_scom(PB_CENT_MODE_CFG, pb_com_pb_cent_mode);
    write_scom(PB_CENT_GP_COMMAND_RATE_DP0, pb_com_pb_cent_gp_cmd_rate_dp0);
    write_scom(PB_CENT_GP_COMMAND_RATE_DP1, pb_com_pb_cent_gp_cmd_rate_dp1);
    write_scom(PB_CENT_RGP_COMMAND_RATE_DP0, pb_com_pb_cent_rgp_cmd_rate_dp0);
    write_scom(PB_CENT_RGP_COMMAND_RATE_DP1, pb_com_pb_cent_rgp_cmd_rate_dp1);
    write_scom(PB_CENT_SP_COMMAND_RATE_DP0, pb_com_pb_cent_sp_cmd_rate_dp0);
    write_scom(PB_CENT_SP_COMMAND_RATE_DP1, pb_com_pb_cent_sp_cmd_rate_dp1);
    write_scom(PB_EAST_MODE_CFG, pb_com_pb_east_mode);
}

void p9_fbc_ioe_tl_scom(void)
{
	uint64_t pb_ioe_scom_pb_fp01_cfg = read_scom(PB_ELE_PB_FRAMER_PARSER_01_CFG);
    uint64_t pb_ioe_scom_pb_fp23_cfg = read_scom(PB_ELE_PB_FRAMER_PARSER_23_CFG);
    uint64_t pb_ioe_scom_pb_fp45_cfg = read_scom(PB_ELE_PB_FRAMER_PARSER_45_CFG);
    uint64_t pb_ioe_scom_pb_elink_data_01_cfg_reg = read_scom(PB_ELE_PB_DATA_BUFF_01_CFG);
    uint64_t pb_ioe_scom_pb_elink_data_23_cfg_reg = read_scom(PB_ELE_PB_DATA_BUFF_23_CFG);
    uint64_t pb_ioe_scom_pb_elink_data_45_cfg_reg = read_scom(PB_ELE_PB_DATA_BUFF_45_CFG);
    uint64_t pb_ioe_scom_pb_misc_cfg = read_scom(PB_ELE_MISC_CFG);
    uint64_t pb_ioe_scom_pb_trace_cfg = read_scom(PB_ELE_LINK_TRACE_CFG);

	if(X0_ENABLED)
	{
		pb_ioe_scom_pb_fp01_cfg &= 0xfff004fffff007bf;
        pb_ioe_scom_pb_fp01_cfg |= 0x0002010000020000;
	}
	else
	{
		pb_ioe_scom_pb_fp01_cfg |= 0x84000000840;
	}

	// if(X0_ENABLED && DD2X_PARTS)
	// {
	// 	PB_IOE_SCOM_PB_FP01_CFG.insert<4, 8, 56, uint64_t>(0x15 - (l_def_DD2_LO_LIMIT_N / l_def_DD2_LO_LIMIT_D))
    //     PB_IOE_SCOM_PB_FP01_CFG.insert<36, 8, 56, uint64_t>(0x15 - (l_def_DD2_LO_LIMIT_N / l_def_DD2_LO_LIMIT_D))
	// }
	// else if ((X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R < DD1_LO_LIMIT_H)
    //       || (X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R == DD1_LO_LIMIT_H && DD1_LO_LIMIT_P))
	// {
	// 	PB_IOE_SCOM_PB_FP01_CFG.insert<4, 8, 56, uint64_t>(0x1A - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
    //     PB_IOE_SCOM_PB_FP01_CFG.insert<36, 8, 56, uint64_t>(0x1A - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
	// }
	// else if ((X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R == DD1_LO_LIMIT_H && !DD1_LO_LIMIT_P)
    //       || (X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R > DD1_LO_LIMIT_H))
    // {
    //     PB_IOE_SCOM_PB_FP01_CFG.insert<4, 8, 56, uint64_t>(0x19 - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
    //     PB_IOE_SCOM_PB_FP01_CFG.insert<36, 8, 56, uint64_t>(0x19 - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
    // }
	if(X2_ENABLED)
    {
        pb_ioe_scom_pb_fp45_cfg &= 0xfff004bffff007bf;
        pb_ioe_scom_pb_fp45_cfg |= 0x0002010000020000;
    }
    else
    {
        pb_ioe_scom_pb_fp45_cfg |= 0x84000000840;
    }
	// if (X2_ENABLED && DD2X_PARTS)
    // {
    //     pb_ioe_scom_pb_fp45_cfg.insert<4, 8, 56, uint64_t>(0x15 - (l_def_dd2_lo_limit_n / l_def_dd2_lo_limit_d))
    //     pb_ioe_scom_pb_fp45_cfg.insert<36, 8, 56, uint64_t>(0x15 - (l_def_dd2_lo_limit_n / l_def_dd2_lo_limit_d))
    // }
    // else if ((X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R < DD1_LO_LIMIT_H)
    //       || (X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R == DD1_LO_LIMIT_H && DD1_LO_LIMIT_P))
    // {
    //     pb_ioe_scom_pb_fp45_cfg.insert<4, 8, 56, uint64_t>(0x1A - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D))
    //     pb_ioe_scom_pb_fp45_cfg.insert<36, 8, 56, uint64_t>(0x1A - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D))
    // }
    // else if ((X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R == DD1_LO_LIMIT_H && !DD1_LO_LIMIT_P)
    //       || (X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R > DD1_LO_LIMIT_H))
    // {
    //     pb_ioe_scom_pb_fp45_cfg.insert<4, 8, 56, uint64_t>(0x19 - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D))
    //     pb_ioe_scom_pb_fp45_cfg.insert<36, 8, 56, uint64_t>(0x19 - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D))
    // }

    if (X0_ENABLED && OPTICS_IS_A_BUS)
    {
        pb_ioe_scom_pb_elink_data_01_cfg_reg &= 0xFFFFFF07FFFFFFFF;
        pb_ioe_scom_pb_elink_data_01_cfg_reg |= 0x0000001000000000;
    }
    else if(X0_ENABLED)
    {
        pb_ioe_scom_pb_elink_data_01_cfg_reg |= 0x0000001F00000000;
    }

    if (X0_ENABLED)
    {
        pb_ioe_scom_pb_elink_data_01_cfg_reg &= 0x80FFFFFF80FFFFFF;
        pb_ioe_scom_pb_elink_data_01_cfg_reg |= 0x4000000040000000;

        pb_ioe_scom_pb_elink_data_01_cfg_reg &= 0xFF8080FFFF8080FF;
        pb_ioe_scom_pb_elink_data_01_cfg_reg |= 0x003C3C00003C3C00;
        
        pb_ioe_scom_pb_trace_cfg &= 0x0000FFFFFFFFFFFF;
        pb_ioe_scom_pb_trace_cfg |= 0x4141000000000000;
    }
    else if (X1_ENABLED)
    {
        pb_ioe_scom_pb_trace_cfg &= 0xFFFF0000FFFFFFFF;
        pb_ioe_scom_pb_trace_cfg |= 0x0000414100000000;
    }
    else if (X2_ENABLED)
    {
        pb_ioe_scom_pb_trace_cfg &= 0xFFFFFFFF0000FFFF;
        pb_ioe_scom_pb_trace_cfg |= 0x0000000041410000;
    }

    // if (X1_ENABLED && DD2X_PARTS)
    // {
    //     PB_IOE_SCOM_PB_FP23_CFG.insert<4, 8, 56, uint64_t>(0x15 - (DD2_LO_LIMIT_N / DD2_LO_LIMIT_D));
    //     PB_IOE_SCOM_PB_FP23_CFG.insert<36, 8, 56, uint64_t>(0x15 - (DD2_LO_LIMIT_N / DD2_LO_LIMIT_D));
    // }
    // else if ((X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R < DD1_LO_LIMIT_H)
    //       || (X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R == DD1_LO_LIMIT_H && DD1_LO_LIMIT_P))
    // {
    //     PB_IOE_SCOM_PB_FP23_CFG.insert<4, 8, 56, uint64_t>(0x1A - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D));
    //     PB_IOE_SCOM_PB_FP23_CFG.insert<36, 8, 56, uint64_t>(0x1A - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D));
    // }
    // else if ((X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R == DD1_LO_LIMIT_H && !DD1_LO_LIMIT_P)
    //       || (X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R > DD1_LO_LIMIT_H))
    // {
    //     PB_IOE_SCOM_PB_FP23_CFG.insert<4, 8, 56, uint64_t>(0x19 - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D));
    //     PB_IOE_SCOM_PB_FP23_CFG.insert<36, 8, 56, uint64_t>(0x19 - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D));
    // }


    if(X1_ENABLED)
    {
        pb_ioe_scom_pb_fp23_cfg &= 0xfff004bffff007bf;
        pb_ioe_scom_pb_fp23_cfg |= 0x0002010000020000;
        if (OPTICS_IS_A_BUS)
        {
            pb_ioe_scom_pb_elink_data_23_cfg_reg &= 0xFE0FFFFFFFFFFFFF;
            pb_ioe_scom_pb_elink_data_23_cfg_reg |= 0x0100000000000000;
        }
        else
        {
            pb_ioe_scom_pb_elink_data_23_cfg_reg &= 0xFE0FFFFFFFFFFFFF;
            pb_ioe_scom_pb_elink_data_23_cfg_reg |= 0x01F0000000000000;
        }
        pb_ioe_scom_pb_elink_data_23_cfg_reg &= 0x808080FF808080FF;
        pb_ioe_scom_pb_elink_data_23_cfg_reg |= 0x403C3C00403C3C00;
    }
    else
    {
        pb_ioe_scom_pb_fp23_cfg |= 0x0000084000000840;
    }

    if(X2_ENABLED)
    {
        if (OPTICS_IS_A_BUS)
        {
            pb_ioe_scom_pb_elink_data_45_cfg_reg &= 0xFFFFFF07FFFFFFFF;
            pb_ioe_scom_pb_elink_data_45_cfg_reg |= 0x0000008000000000;
        }
        else
        {
            pb_ioe_scom_pb_elink_data_45_cfg_reg |= 0x000000F800000000;
        }
        pb_ioe_scom_pb_elink_data_45_cfg_reg &= 0x808080FF808080FF;
        pb_ioe_scom_pb_elink_data_45_cfg_reg |= 0x403C3C00403C3C00;
    }

    if(X0_IS_PAIRED)
    {
        pb_ioe_scom_pb_misc_cfg |= 0x8000000000000000;
    }
    else
    {
        pb_ioe_scom_pb_misc_cfg &= 0x7FFFFFFFFFFFFFFF;
    }
    if(X1_IS_PAIRED)
    {
        pb_ioe_scom_pb_misc_cfg |= 0x4000000000000000;
    }
    else
    {
        pb_ioe_scom_pb_misc_cfg &= 0xBFFFFFFFFFFFFFFF;
    }
    if(X2_IS_PAIRED)
    {
        pb_ioe_scom_pb_misc_cfg |= 0x2000000000000000;
    }
    else
    {
        pb_ioe_scom_pb_misc_cfg &= 0xDFFFFFFFFFFFFFFF;
    }

    // REGISTERS write
    write_scom(PB_ELE_PB_FRAMER_PARSER_01_CFG, pb_ioe_scom_pb_fp01_cfg);
    write_scom(PB_ELE_PB_FRAMER_PARSER_23_CFG, pb_ioe_scom_pb_fp23_cfg);
    write_scom(PB_ELE_PB_FRAMER_PARSER_45_CFG, pb_ioe_scom_pb_fp45_cfg);
    write_scom(PB_ELE_PB_DATA_BUFF_01_CFG, pb_ioe_scom_pb_elink_data_01_cfg_reg);
    write_scom(PB_ELE_PB_DATA_BUFF_23_CFG, pb_ioe_scom_pb_elink_data_23_cfg_reg);
    write_scom(PB_ELE_PB_DATA_BUFF_45_CFG, pb_ioe_scom_pb_elink_data_45_cfg_reg);
    write_scom(PB_ELE_MISC_CFG, pb_ioe_scom_pb_misc_cfg);
    write_scom(PB_ELE_LINK_TRACE_CFG, pb_ioe_scom_pb_trace_cfg);
}

void main(void)
{
	console_init();
	istep89();
	run_ramstage();
}
