/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <program_loading.h>

#include <cpu/power/scom.h>

#define X0_ENABLED true
#define X1_ENABLED true
#define X2_ENABLED true
#define X0_IS_PAIRED true
#define X1_IS_PAIRED true
#define X2_IS_PAIRED true
#define DD2X_PARTS true
#define OPTICS_IS_A_BUS true

void istep89()
{
	printk(BIOS_EMERG, "starting istep 8.9\n");
	p9_fbc_no_hp_scom();
    p9_fbc_ioe_tl_scom();
	printk(BIOS_EMERG, "ending istep 8.9\n");
}

void p9_fbc_ioe_tl_scom()
{
    return;
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
    write_scom(0x501340A, pb_ioe_scom_pb_fp01_cfg);
    write_scom(0x501340B, pb_ioe_scom_pb_fp23_cfg);
    write_scom(0x501340C, pb_ioe_scom_pb_fp45_cfg);
    write_scom(0x5013410, pb_ioe_scom_pb_elink_data_01_cfg_reg);
    write_scom(0x5013411, pb_ioe_scom_pb_elink_data_23_cfg_reg);
    write_scom(0x5013412, pb_ioe_scom_pb_elink_data_45_cfg_reg);
    write_scom(0x5013423, pb_ioe_scom_pb_misc_cfg);
    write_scom(0x5013424, pb_ioe_scom_pb_trace_cfg);
}

void main(void)
{
	console_init();
	istep89();
	run_ramstage();
}
