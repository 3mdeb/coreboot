/* SPDX-License-Identifier: GPL-2.0-only */

void call_proc_xbus_enable_ridi(void);
void p9_chiplet_scominit(void);
void p9_cxa_scom(void);
void p9_nx_scom(void);
void p9_vas_scom(void);
void p9_fbc_ioo_dl_scom(void);

#define FBC_IOO_TL_FIR_ACTION0 (0x0000000000000000ULL)
#define FBC_IOO_TL_FIR_ACTION1 (0x0049200000000000ULL)
#define FBC_IOO_TL_FIR_MASK    (0xFF2490000FFFF00FULL)

#define CHIP_EC (0x20)
#define SECURE_MEMORY (CHIP_EC >= 0x22)
