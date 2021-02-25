/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/dram/ddr4.h>
#include <spd_bin.h>

void istep_13_2(dimm_attr dimms[CONFIG_DIMM_MAX]);
void istep_13_3(struct spd_block *blk);
void istep_13_4(dimm_attr dimms[CONFIG_DIMM_MAX]);
void istep_13_6(dimm_attr dimms[CONFIG_DIMM_MAX]);
