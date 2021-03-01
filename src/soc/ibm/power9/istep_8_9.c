/* SPDX-License-Identifier: GPL-2.0-only */

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
    scom_and_or_for_chiplet(XB_CHIPLET_ID, PB_ELL_CFG_REG, 0x7FEFFFFFFFFFFFFF, 0x280F000F00000000);
    scom_and_or_for_chiplet(XB_CHIPLET_ID, PB_ELL_REPLAY_TRESHOLD_REG, 0xFFFFFFFFFFFFFFF, 0x6FE0000000000000);
    scom_and_or_for_chiplet(XB_CHIPLET_ID, PB_ELL_SL_ECC_TRESHOLD_REG, 0x7FFFFFFFFFFFFFFF, 0x7F70000000000000);
}

void ioe_tl_fir(void)
{
    write_scom(PU_PB_IOE_FIR_ACTION0_REG, FBC_IOE_TL_FIR_ACTION0);
    write_scom(PU_PB_IOE_FIR_ACTION1_REG, FBC_IOE_TL_FIR_ACTION1);
    write_scom(PU_PB_IOE_FIR_MASK_REG, FBC_IOE_TL_FIR_MASK | FBC_IOE_TL_FIR_MASK_X0_NF);
}

void ioel_fir(void)
{
    write_scom_for_chiplet(XB_CHIPLET_ID, XBUS_LL0_IOEL_FIR_ACTION0_REG, FBC_IOE_DL_FIR_ACTION0);
    write_scom_for_chiplet(XB_CHIPLET_ID, XBUS_LL0_IOEL_FIR_ACTION1_REG, FBC_IOE_DL_FIR_ACTION1);
    write_scom_for_chiplet(XB_CHIPLET_ID, XBUS_LL0_IOEL_FIR_MASK_REG, FBC_IOE_DL_FIR_MASK);
}

void p9_fbc_no_hp_scom(void)
{
    scom_and_or(PB_WEST_MODE_CFG_REG, 0xFFFF00000FFFFFFF, 0xFAFEA0000000);
    scom_and_or(PB_CENT_MODE_CFG_REG, 0xFFFF00000FFFFFFF, 0x7EFEA0000000);
    write_scom(PB_CENT_GP_COMMAND_RATE_DP0_REG, 0);
    write_scom(PB_CENT_GP_COMMAND_RATE_DP1_REG, 0);
    write_scom(PB_CENT_RGP_COMMAND_RATE_DP0_REG, 0x30406080A0C1218);
    write_scom(PB_CENT_RGP_COMMAND_RATE_DP1_REG, 0x40508080A0C1218);
    write_scom(PB_CENT_SP_COMMAND_RATE_DP0_REG, 0x30406080A0C1218);
    write_scom(PB_CENT_SP_COMMAND_RATE_DP1_REG, 0x30406080A0C1218);
    scom_and_or(PB_EAST_MODE_CFG_REG, 0xF1FF00000FFFFFFF, 0x7EFE0000000);
}

void p9_fbc_ioe_tl_scom(void)
{
    scom_and_or(PB_ELE_PB_FRAMER_PARSER_01_CFG_REG, 0xfff004fffff007bf, 0x2010000020000);
    scom_and_or(PB_ELE_PB_DATA_BUFF_01_CFG_REG, 0x808080FF808080FF, 0x403C3C1F403C3C00);
    scom_and_or(PB_ELE_LINK_TRACE_CFG_REG, 0xFFFFFFFFFFFF, 0x4141000000000000);
    scom_or(PB_ELE_PB_FRAMER_PARSER_23_CFG_REG, 0x84000000840);
    scom_or(PB_ELE_PB_FRAMER_PARSER_45_CFG_REG, 0x84000000840);
    scom_and_or(PB_ELE_MISC_CFG_REG, 0x9FFFFFFFFFFFFFFF, 0x8000000000000000);
}
