/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>

// not sure about them
#define X0_ENABLED (true)
#define X1_ENABLED (true)
#define X2_ENABLED (true)

// not sure about them
#define X0_IS_PAIRED (true)
#define X1_IS_PAIRED (true)
#define X2_IS_PAIRED (true)

// set in c++ code please verify
#define ATTR_PROC_FABRIC_A_LINKS_CNFG (0)
// set in c++ code please verify
#define ATTR_PROC_FABRIC_X_LINKS_CNFG (0)

#define FBC_IOE_TL_FIR_ACTION0 (0x0000000000000000ULL)
#define FBC_IOE_TL_FIR_ACTION1 (0x0049000000000000ULL)

#define FBC_IOE_TL_FIR_MASK (0xFF24F0303FFFF11FULL)
#define P9_FBC_UTILS_MAX_ELECTRICAL_LINKS (3)
#define FBC_IOE_TL_FIR_MASK_X0_NF (0x00C00C0C00000880ULL)
#define FBC_IOE_TL_FIR_MASK_X1_NF (0x0018030300000440ULL)
#define FBC_IOE_TL_FIR_MASK_X2_NF (0x000300C0C0000220ULL)
#define PU_PB_IOE_FIR_MASK_REG (0x5013403)

#define FBC_IOE_DL_FIR_ACTION0 (0)
#define FBC_IOE_DL_FIR_ACTION1 (0x303c00000001ffc)
#define FBC_IOE_DL_FIR_MASK (0xfcfc3fffffffe003)

// set in c++ code please verify
#define BOTH        (0x0)
#define EVEN_ONLY   (0x1)
#define ODD_ONLY    (0x2)
#define NONE        (0x3)
// for sure can be NONE, but maybe should be smth else?
#define ATTR_LINK_TRAIN NONE

// this is probably wrong
#define PROC_CHIPLET (0x10)

void istep_8_9(void);
void p9_fbc_ioe_dl_scom(uint64_t TGT0);
void p9_fbc_ioe_tl_scom(void);
void p9_fbc_no_hp_scom(void);
void tl_fir(void);
