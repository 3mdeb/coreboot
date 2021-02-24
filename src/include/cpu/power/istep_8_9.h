/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>

#define ATTR_PROC_FABRIC_X_LINKS_CNFG (1)

#define FBC_IOE_TL_FIR_ACTION0 (0x0000000000000000ULL)
#define FBC_IOE_TL_FIR_ACTION1 (0x0049000000000000ULL)

#define FBC_IOE_TL_FIR_MASK       (0xFF24F0303FFFF11FULL)
#define FBC_IOE_TL_FIR_MASK_X0_NF (0x00C00C0C00000880ULL)

#define FBC_IOE_DL_FIR_ACTION0 (0)
#define FBC_IOE_DL_FIR_ACTION1 (0x303c00000001ffc)
#define FBC_IOE_DL_FIR_MASK    (0xfcfc3fffffffe003)

void istep_8_9(void);
void p9_fbc_ioe_dl_scom(void);
void p9_fbc_ioe_tl_scom(void);
void p9_fbc_no_hp_scom(void);
void ioe_tl_fir(void);
void ioel_fir(void);
