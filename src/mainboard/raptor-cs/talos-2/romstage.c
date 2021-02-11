/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <program_loading.h>

#include <cpu/power/scom.h>

#define X0_ENABLED true
#define X2_ENABLED true
#define DD2X_PARTS true

void istep89()
{
	printk(BIOS_EMERG, "starting istep 8.9\n");
	p9_fbc_no_hp_scom();
	printk(BIOS_EMERG, "ending istep 8.9\n");
}

void p9_fbc_no_hp_scom()
{
	uint64_t pb_ioe_scom_pb_fp01_cfg = read_scom(0x501340A); // Processor bus Electrical Framer/Parser 01 configuration register
    uint64_t pb_ioe_scom_pb_fp23_cfg = read_scom(0x501340B); // Power Bus Electrical Framer/Parser 23 Configuration Register
    uint64_t pb_ioe_scom_pb_fp45_cfg = read_scom(0x501340C); // Power Bus Electrical Framer/Parser 45 Configuration Register
    uint64_t pb_ioe_scom_pb_elink_data_01_cfg_reg = read_scom(0x5013410); // Power Bus Electrical Link Data Buffer 01 Configuration Register
    uint64_t pb_ioe_scom_pb_elink_data_23_cfg_reg = read_scom(0x5013411); // Power Bus Electrical Link Data Buffer 23 Configuration Register
    uint64_t pb_ioe_scom_pb_elink_data_45_cfg_reg = read_scom(0x5013412); // Power Bus Electrical Link Data Buffer 45 Configuration Register
    uint64_t pb_ioe_scom_pb_misc_cfg = read_scom(0x5013423); // Power Bus Electrical Miscellaneous Configuration Register
    uint64_t pb_ioe_scom_pb_trace_cfg = read_scom(0x5013424); // Power Bus Electrical Link Trace Configuration Register

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
	if (x2_enabled && l_def_dd2x_parts)
    {
        pb_ioe_scom_pb_fp45_cfg.insert<4, 8, 56, uint64_t>(0x15 - (l_def_dd2_lo_limit_n / l_def_dd2_lo_limit_d))
        pb_ioe_scom_pb_fp45_cfg.insert<36, 8, 56, uint64_t>(0x15 - (l_def_dd2_lo_limit_n / l_def_dd2_lo_limit_d))
    }
    else if ((l_def_X0_ENABLED && l_def_DD1_PARTS && l_def_DD1_LO_LIMIT_R < l_def_DD1_LO_LIMIT_H)
          || (l_def_X0_ENABLED && l_def_DD1_PARTS && l_def_DD1_LO_LIMIT_R == l_def_DD1_LO_LIMIT_H && l_def_DD1_LO_LIMIT_P))
    {
        pb_ioe_scom_pb_fp45_cfg.insert<4, 8, 56, uint64_t>(0x1A - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
        pb_ioe_scom_pb_fp45_cfg.insert<36, 8, 56, uint64_t>(0x1A - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
    }
    else if ((l_def_X0_ENABLED && l_def_DD1_PARTS && l_def_DD1_LO_LIMIT_R == l_def_DD1_LO_LIMIT_H && !l_def_DD1_LO_LIMIT_P)
          || (l_def_X0_ENABLED && l_def_DD1_PARTS && l_def_DD1_LO_LIMIT_R > l_def_DD1_LO_LIMIT_H))
    {
        pb_ioe_scom_pb_fp45_cfg.insert<4, 8, 56, uint64_t>(0x19 - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
        pb_ioe_scom_pb_fp45_cfg.insert<36, 8, 56, uint64_t>(0x19 - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
    }

    if (l_def_X0_ENABLED)IOEL_REPLAY_THRESHOLD // FIXME: there can be some missing code
        {
            pb_ioe_scom_pb_elink_data_01_cfg_reg &= 0xFFFFFF07FFFFFFFF
            pb_ioe_scom_pb_elink_data_01_cfg_reg |= 0x0000008000000000
        }
        else
        {
            pb_ioe_scom_pb_elink_data_01_cfg_reg |= 0x000000F800000000
        }
        if (l_TGT0_ATTR_CHIP_EC_FEATURE_HW384245 != 0)
        {
            pb_ioe_scom_pb_elink_data_01_cfg_reg &= 0x80FFFFFF80FFFFFF
            pb_ioe_scom_pb_elink_data_01_cfg_reg |= 0x3F0000003F000000
        }
        else
        {
            pb_ioe_scom_pb_elink_data_01_cfg_reg &= 0x80FFFFFF80FFFFFF
            pb_ioe_scom_pb_elink_data_01_cfg_reg |= 0x4000000040000000
        }
        pb_ioe_scom_pb_elink_data_01_cfg_reg &= 0xFF8080FFFF8080FF
        pb_ioe_scom_pb_elink_data_01_cfg_reg |= 0x003C3C00003C3C00

        pb_ioe_scom_pb_trace_cfg &= 0x0000FFFFFFFFFFFF
        pb_ioe_scom_pb_trace_cfg |= 0x4141000000000000
    }
    else if (l_def_X1_ENABLED)
    {
        pb_ioe_scom_pb_trace_cfg &= 0xFFFF0000FFFFFFFF
        pb_ioe_scom_pb_trace_cfg |= 0x0000414100000000
    }
    else if (l_def_X2_ENABLED)
    {
        pb_ioe_scom_pb_trace_cfg &= 0xFFFFFFFF0000FFFF
        pb_ioe_scom_pb_trace_cfg |= 0x0000000041410000
    }

    if (l_def_X1_ENABLED && l_def_DD2X_PARTS)
    {
        PB_IOE_SCOM_PB_FP23_CFG.insert<4, 8, 56, uint64_t>(0x15 - (l_def_DD2_LO_LIMIT_N / l_def_DD2_LO_LIMIT_D))
        PB_IOE_SCOM_PB_FP23_CFG.insert<36, 8, 56, uint64_t>(0x15 - (l_def_DD2_LO_LIMIT_N / l_def_DD2_LO_LIMIT_D))
    }
    else if ((l_def_X0_ENABLED && l_def_DD1_PARTS && l_def_DD1_LO_LIMIT_R < l_def_DD1_LO_LIMIT_H)
          || (l_def_X0_ENABLED && l_def_DD1_PARTS && l_def_DD1_LO_LIMIT_R == l_def_DD1_LO_LIMIT_H && l_def_DD1_LO_LIMIT_P))
    {
        PB_IOE_SCOM_PB_FP23_CFG.insert<4, 8, 56, uint64_t>(0x1A - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
        PB_IOE_SCOM_PB_FP23_CFG.insert<36, 8, 56, uint64_t>(0x1A - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
    }
    else if ((l_def_X0_ENABLED && l_def_DD1_PARTS && l_def_DD1_LO_LIMIT_R == l_def_DD1_LO_LIMIT_H && !l_def_DD1_LO_LIMIT_P)
          || (l_def_X0_ENABLED && l_def_DD1_PARTS && l_def_DD1_LO_LIMIT_R > l_def_DD1_LO_LIMIT_H))
    {
        PB_IOE_SCOM_PB_FP23_CFG.insert<4, 8, 56, uint64_t>(0x19 - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
        PB_IOE_SCOM_PB_FP23_CFG.insert<36, 8, 56, uint64_t>(0x19 - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
    }


    if(l_def_X1_ENABLED)
    {
        PB_IOE_SCOM_PB_FP23_CFG &= 0xfff004bffff007bf
        PB_IOE_SCOM_PB_FP23_CFG |= 0x0002010000020000
        if (l_def_OPTICS_IS_A_BUS)
        {
            PB_IOE_SCOM_PB_ELINK_DATA_23_CFG_REG &= 0xFE0FFFFFFFFFFFFF
            PB_IOE_SCOM_PB_ELINK_DATA_23_CFG_REG |= 0x0100000000000000
        }
        else
        {
            PB_IOE_SCOM_PB_ELINK_DATA_23_CFG_REG &= 0xFE0FFFFFFFFFFFFF
            PB_IOE_SCOM_PB_ELINK_DATA_23_CFG_REG |= 0x01F0000000000000
        }
        if (l_TGT0_ATTR_CHIP_EC_FEATURE_HW384245 != 0)
        {
            PB_IOE_SCOM_PB_ELINK_DATA_23_CFG_REG &= 0x80FFFFFF80FFFFFF
            PB_IOE_SCOM_PB_ELINK_DATA_23_CFG_REG |= 0x3F0000003F000000
        }
        else
        {
            PB_IOE_SCOM_PB_ELINK_DATA_23_CFG_REG &= 0x80FFFFFF80FFFFFF
            PB_IOE_SCOM_PB_ELINK_DATA_23_CFG_REG |= 0x4000000040000000
        }
        PB_IOE_SCOM_PB_ELINK_DATA_23_CFG_REG &= 0xFF8080FFFF8080FF
        PB_IOE_SCOM_PB_ELINK_DATA_23_CFG_REG |= 0x003C3C00003C3C00
    }
    else
    {
        PB_IOE_SCOM_PB_FP23_CFG |= 0x0000084000000840
    }

    if(l_def_X2_ENABLED)
    {
        if (l_def_OPTICS_IS_A_BUS)
        {
            PB_IOE_SCOM_PB_ELINK_DATA_45_CFG_REG &= 0xFFFFFF07FFFFFFFF
            PB_IOE_SCOM_PB_ELINK_DATA_45_CFG_REG |= 0x0000008000000000
        }
        else
        {
            PB_IOE_SCOM_PB_ELINK_DATA_45_CFG_REG |= 0x000000F800000000
        }
        if (l_TGT0_ATTR_CHIP_EC_FEATURE_HW384245 != 0)
        {
            PB_IOE_SCOM_PB_ELINK_DATA_45_CFG_REG &= 0x80FFFFFF80FFFFFF
            PB_IOE_SCOM_PB_ELINK_DATA_45_CFG_REG |= 0x3F0000003F000000
        }
        else
        {
            PB_IOE_SCOM_PB_ELINK_DATA_45_CFG_REG &= 0x80FFFFFF80FFFFFF
            PB_IOE_SCOM_PB_ELINK_DATA_45_CFG_REG |= 0x4000000040000000
        }
        PB_IOE_SCOM_PB_ELINK_DATA_45_CFG_REG &= 0xFF8080FFFF8080FF
        PB_IOE_SCOM_PB_ELINK_DATA_45_CFG_REG |= 0x003C3C00003C3C00
    }

    if (l_def_X0_IS_PAIRED)
    {
        PB_IOE_SCOM_PB_MISC_CFG |= 0x8000000000000000
    }
    else
    {
        PB_IOE_SCOM_PB_MISC_CFG &= 0x7FFFFFFFFFFFFFFF
    }
    if (l_def_X1_IS_PAIRED)
    {
        PB_IOE_SCOM_PB_MISC_CFG |= 0x4000000000000000
    }
    else
    {
        PB_IOE_SCOM_PB_MISC_CFG &= 0xBFFFFFFFFFFFFFFF
    }
    if (l_def_X2_IS_PAIRED)
    {
        PB_IOE_SCOM_PB_MISC_CFG |= 0x2000000000000000
    }
    else
    {
        PB_IOE_SCOM_PB_MISC_CFG |= 0xDFFFFFFFFFFFFFFF
    }

    // REGISTERS write
    TGT0[0x501340A] = PB_IOE_SCOM_PB_FP01_CFG;
    TGT0[0x501340B] = PB_IOE_SCOM_PB_FP23_CFG;
    TGT0[0x501340C] = pb_ioe_scom_pb_fp45_cfg;
    TGT0[0x5013410] = pb_ioe_scom_pb_elink_data_01_cfg_reg;
    TGT0[0x5013411] = PB_IOE_SCOM_PB_ELINK_DATA_23_CFG_REG;
    TGT0[0x5013412] = PB_IOE_SCOM_PB_ELINK_DATA_45_CFG_REG;
    TGT0[0x5013423] = PB_IOE_SCOM_PB_MISC_CFG;
    TGT0[0x5013424] = pb_ioe_scom_pb_trace_cfg;
}

void main(void)
{
	console_init();
	istep89();
	run_ramstage();
}
