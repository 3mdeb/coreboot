/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/scom.h>

#define XBUS_PHY_FIR_ACTION0 (0x0000000000000000ULL)
#define XBUS_PHY_FIR_ACTION1 (0x2068680000000000ULL)
#define XBUS_PHY_FIR_MASK    (0xDF9797FFFFFFC000ULL)

void istep_8_10(void);
void p9_io_xbus_scominit(const uint8_t);
void p9_xbus_g0_scom(void);
void p9_xbus_g1_scom(void);
