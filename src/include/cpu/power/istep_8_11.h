/* SPDX-License-Identifier: GPL-2.0-only */

void call_proc_xbus_enable_ridi(void);
void p9_chiplet_scominit(void);

#define FBC_IOO_TL_FIR_ACTION0 (0x0000000000000000ULL)
#define FBC_IOO_TL_FIR_ACTION1 (0x0049200000000000ULL)
#define FBC_IOO_TL_FIR_MASK    (0xFF2490000FFFF00FULL)
