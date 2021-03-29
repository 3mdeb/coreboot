/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/scom.h>

void memDiag(void);
void istep_14_1(void);

memdiagTargets[] = {MC01_CHIPLET_ID, MC23_CHIPLET_ID};
unsigned int memdiagTargetsSize = sizeof memdiagTargets / sizeof memdiagTargets[0];

#define RRQ0Q_REG 0x000000000701090E
#define WRQ0Q_REG 0x000000000701090D

#define MBA_RRQ0Q_CFG_RRQ_FIFO_MODE 6
#define MBA_WRQ0Q_CFG_WRQ_FIFO_MODE 5

#define PORT_OFFSEET 6
