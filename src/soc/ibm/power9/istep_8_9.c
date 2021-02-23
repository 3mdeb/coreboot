/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/istep_8_9.h>
#include <cpu/power/scom.h>
#include <cpu/power/scom_registers.h>

void istep_8_9(void)
{
	printk(BIOS_EMERG, "starting istep 8.9\n");
	p9_fbc_no_hp_scom();
    p9_fbc_ioe_tl_scom();

    p9_fbc_ioe_dl_scom(XB_CHIPLET_ID);
    write_scom_for_chiplet(XB_CHIPLET_ID, XBUS_LL0_IOEL_FIR_ACTION0_REG, FBC_IOE_DL_FIR_ACTION0);
    write_scom_for_chiplet(XB_CHIPLET_ID, XBUS_LL0_IOEL_FIR_ACTION1_REG, FBC_IOE_DL_FIR_ACTION1);
    write_scom_for_chiplet(XB_CHIPLET_ID, XBUS_LL0_IOEL_FIR_MASK_REG, FBC_IOE_DL_FIR_MASK);
	printk(BIOS_EMERG, "ending istep 8.9\n");
}

void p9_fbc_ioe_dl_scom(uint64_t TGT0)
{   
    uint64_t pb_ioe_ll0_ioel_config = read_scom_for_chiplet(TGT0, PB_ELL_CFG_REG);
    uint64_t pb_ioe_ll0_ioel_replay_threshold = read_scom_for_chiplet(TGT0, PB_ELL_REPLAY_TRESHOLD_REG);
    uint64_t pb_ioe_ll0_ioel_sl_ecc_threshold = read_scom_for_chiplet(TGT0, PB_ELL_SL_ECC_TRESHOLD_REG);
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
    pb_ioe_ll0_ioel_replay_threshold &= 0x0FFFFFFFFFFFFFFF;
    pb_ioe_ll0_ioel_replay_threshold |= 0x6FE0000000000000;
    pb_ioe_ll0_ioel_sl_ecc_threshold &= 0x7FFFFFFFFFFFFFFF;
    pb_ioe_ll0_ioel_sl_ecc_threshold |= 0x7F70000000000000;

    write_scom_for_chiplet(TGT0, PB_ELL_CFG_REG, pb_ioe_ll0_ioel_config);
    write_scom_for_chiplet(TGT0, PB_ELL_REPLAY_TRESHOLD_REG, pb_ioe_ll0_ioel_replay_threshold);
    write_scom_for_chiplet(TGT0, PB_ELL_SL_ECC_TRESHOLD_REG, pb_ioe_ll0_ioel_sl_ecc_threshold);
}

void tl_fir(void)
{
    write_scom_for_chiplet(PROC_CHIPLET, PU_PB_IOE_FIR_ACTION0_REG, FBC_IOE_TL_FIR_ACTION0);
    write_scom_for_chiplet(PROC_CHIPLET, PU_PB_IOE_FIR_ACTION1_REG, FBC_IOE_TL_FIR_ACTION1);
    // not sure about this one, maybe X1 or X2 mask?
    write_scom_for_chiplet(PROC_CHIPLET, PU_PB_IOE_FIR_MASK_REG, FBC_IOE_TL_FIR_MASK | FBC_IOE_TL_FIR_MASK_X0_NF);
}

void p9_fbc_no_hp_scom(void)
{
    uint64_t pb_com_pb_west_mode = read_scom_for_chiplet(PROC_CHIPLET, PB_WEST_MODE_CFG_REG);
    uint64_t pb_com_pb_cent_mode = read_scom_for_chiplet(PROC_CHIPLET, PB_CENT_MODE_CFG_REG);
    uint64_t pb_com_pb_cent_gp_cmd_rate_dp0 = read_scom_for_chiplet(PROC_CHIPLET, PB_CENT_GP_COMMAND_RATE_DP0_REG);
    uint64_t pb_com_pb_cent_gp_cmd_rate_dp1 = read_scom_for_chiplet(PROC_CHIPLET, PB_CENT_GP_COMMAND_RATE_DP1_REG);
    uint64_t pb_com_pb_cent_rgp_cmd_rate_dp0 = read_scom_for_chiplet(PROC_CHIPLET, PB_CENT_RGP_COMMAND_RATE_DP0_REG);
    uint64_t pb_com_pb_cent_rgp_cmd_rate_dp1 = read_scom_for_chiplet(PROC_CHIPLET, PB_CENT_RGP_COMMAND_RATE_DP1_REG);
    uint64_t pb_com_pb_cent_sp_cmd_rate_dp0 = read_scom_for_chiplet(PROC_CHIPLET, PB_CENT_SP_COMMAND_RATE_DP0_REG);
    uint64_t pb_com_pb_cent_sp_cmd_rate_dp1 = read_scom_for_chiplet(PROC_CHIPLET, PB_CENT_SP_COMMAND_RATE_DP1_REG);
    uint64_t pb_com_pb_east_mode = read_scom_for_chiplet(PROC_CHIPLET, PB_EAST_MODE_CFG_REG);

    pb_com_pb_cent_gp_cmd_rate_dp0 = 0;
    pb_com_pb_cent_gp_cmd_rate_dp1 = 0;

    if (ATTR_PROC_FABRIC_X_LINKS_CNFG == 0)
    {
        pb_com_pb_cent_rgp_cmd_rate_dp0 = 0;
        pb_com_pb_cent_rgp_cmd_rate_dp1 = 0;
        pb_com_pb_cent_sp_cmd_rate_dp0 = 0;
        pb_com_pb_cent_sp_cmd_rate_dp1 = 0;
    }
    else if (ATTR_PROC_FABRIC_X_LINKS_CNFG > 0 && ATTR_PROC_FABRIC_X_LINKS_CNFG < 3)
    {
        pb_com_pb_cent_rgp_cmd_rate_dp0 = 0x030406080A0C1218;
        pb_com_pb_cent_rgp_cmd_rate_dp1 = 0x040508080A0C1218;
        pb_com_pb_cent_sp_cmd_rate_dp0 = 0x030406080A0C1218;
        pb_com_pb_cent_sp_cmd_rate_dp1 = 0x030406080A0C1218;
    }
    else if (ATTR_PROC_FABRIC_X_LINKS_CNFG == 3)
    {
        pb_com_pb_cent_rgp_cmd_rate_dp0 = 0x0304060D10141D28;
        pb_com_pb_cent_rgp_cmd_rate_dp1 = 0x0405080D10141D28;
        pb_com_pb_cent_sp_cmd_rate_dp0 = 0x05070A0D10141D28;
        pb_com_pb_cent_sp_cmd_rate_dp1 = 0x05070A0D10141D28;
    }

    if (ATTR_PROC_FABRIC_X_LINKS_CNFG == 0 && ATTR_PROC_FABRIC_A_LINKS_CNFG == 0)
    {
        pb_com_pb_east_mode |= 0x0300000000000000;
    }
    else
    {
        pb_com_pb_east_mode &= 0xF1FFFFFFFFFFFFFF;
    }
    pb_com_pb_west_mode &= 0xFFFF0003FFFFFFFF;
    pb_com_pb_west_mode |= 0x0000FAFC00000000;
    pb_com_pb_cent_mode &= 0xFFFF0003FFFFFFFF;
    pb_com_pb_cent_mode |= 0x00007EFC00000000;
    pb_com_pb_east_mode &= 0xFFFF0003FFFFFFFF;
    pb_com_pb_east_mode |= 0x000007EFC0000000;

    pb_com_pb_west_mode &= 0xFFFFFFFC0FFFFFFF;
    pb_com_pb_west_mode |= 0x00000002a0000000;
    pb_com_pb_cent_mode &= 0xFFFFFFFC0FFFFFFF;
    pb_com_pb_cent_mode |= 0x00000002a0000000;
    pb_com_pb_east_mode &= 0xFFFFFFFC0FFFFFFF;
    pb_com_pb_east_mode |= 0x00000002a0000000;

    write_scom_for_chiplet(PROC_CHIPLET, PB_WEST_MODE_CFG_REG, pb_com_pb_west_mode);
    write_scom_for_chiplet(PROC_CHIPLET, PB_CENT_MODE_CFG_REG, pb_com_pb_cent_mode);
    write_scom_for_chiplet(PROC_CHIPLET, PB_CENT_GP_COMMAND_RATE_DP0_REG, pb_com_pb_cent_gp_cmd_rate_dp0);
    write_scom_for_chiplet(PROC_CHIPLET, PB_CENT_GP_COMMAND_RATE_DP1_REG, pb_com_pb_cent_gp_cmd_rate_dp1);
    write_scom_for_chiplet(PROC_CHIPLET, PB_CENT_RGP_COMMAND_RATE_DP0_REG, pb_com_pb_cent_rgp_cmd_rate_dp0);
    write_scom_for_chiplet(PROC_CHIPLET, PB_CENT_RGP_COMMAND_RATE_DP1_REG, pb_com_pb_cent_rgp_cmd_rate_dp1);
    write_scom_for_chiplet(PROC_CHIPLET, PB_CENT_SP_COMMAND_RATE_DP0_REG, pb_com_pb_cent_sp_cmd_rate_dp0);
    write_scom_for_chiplet(PROC_CHIPLET, PB_CENT_SP_COMMAND_RATE_DP1_REG, pb_com_pb_cent_sp_cmd_rate_dp1);
    write_scom_for_chiplet(PROC_CHIPLET, PB_EAST_MODE_CFG_REG, pb_com_pb_east_mode);
}

void p9_fbc_ioe_tl_scom(void)
{
    uint64_t pb_ioe_scom_pb_elink_data_01_cfg_reg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_DATA_BUFF_01_CFG_REG);
    uint64_t pb_ioe_scom_pb_elink_data_23_cfg_reg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_DATA_BUFF_23_CFG_REG);
    uint64_t pb_ioe_scom_pb_elink_data_45_cfg_reg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_DATA_BUFF_45_CFG_REG);
    uint64_t pb_ioe_scom_pb_fp01_cfg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_FRAMER_PARSER_01_CFG_REG);
    uint64_t pb_ioe_scom_pb_fp23_cfg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_FRAMER_PARSER_23_CFG_REG);
    uint64_t pb_ioe_scom_pb_fp45_cfg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_FRAMER_PARSER_45_CFG_REG);
    uint64_t pb_ioe_scom_pb_misc_cfg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_MISC_CFG_REG);
    uint64_t pb_ioe_scom_pb_trace_cfg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_LINK_TRACE_CFG_REG);

	if(X0_ENABLED)
	{
		pb_ioe_scom_pb_fp01_cfg &= 0xfff004fffff007bf;
        pb_ioe_scom_pb_fp01_cfg |= 0x0002010000020000;
	}
	else
	{
		pb_ioe_scom_pb_fp01_cfg |= 0x84000000840;
	}

    if(X0_ENABLED)
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

    if(X1_ENABLED)
    {
        pb_ioe_scom_pb_fp23_cfg &= 0xfff004bffff007bf;
        pb_ioe_scom_pb_fp23_cfg |= 0x0002010000020000;
        pb_ioe_scom_pb_elink_data_23_cfg_reg &= 0xFE0FFFFFFFFFFFFF;
        pb_ioe_scom_pb_elink_data_23_cfg_reg |= 0x01F0000000000000;
        pb_ioe_scom_pb_elink_data_23_cfg_reg &= 0x808080FF808080FF;
        pb_ioe_scom_pb_elink_data_23_cfg_reg |= 0x403C3C00403C3C00;
    }
    else
    {
        pb_ioe_scom_pb_fp23_cfg |= 0x0000084000000840;
    }
	
	if(X2_ENABLED)
    {
        pb_ioe_scom_pb_fp45_cfg &= 0xfff004bffff007bf;
        pb_ioe_scom_pb_fp45_cfg |= 0x0002010000020000;
    }
    else
    {
        pb_ioe_scom_pb_fp45_cfg |= 0x84000000840;
    }

    if(X2_ENABLED)
    {
        pb_ioe_scom_pb_elink_data_45_cfg_reg &= 0x808080FF808080FF;
        pb_ioe_scom_pb_elink_data_45_cfg_reg |= 0x403C3CF8403C3C00;
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

    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_LINK_TRACE_CFG_REG, pb_ioe_scom_pb_trace_cfg);
    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_MISC_CFG_REG, pb_ioe_scom_pb_misc_cfg);
    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_DATA_BUFF_01_CFG_REG, pb_ioe_scom_pb_elink_data_01_cfg_reg);
    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_DATA_BUFF_23_CFG_REG, pb_ioe_scom_pb_elink_data_23_cfg_reg);
    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_DATA_BUFF_45_CFG_REG, pb_ioe_scom_pb_elink_data_45_cfg_reg);
    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_FRAMER_PARSER_01_CFG_REG, pb_ioe_scom_pb_fp01_cfg);
    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_FRAMER_PARSER_23_CFG_REG, pb_ioe_scom_pb_fp23_cfg);
    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_FRAMER_PARSER_45_CFG_REG, pb_ioe_scom_pb_fp45_cfg);
}
