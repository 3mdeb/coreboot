/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/vpd.h>
#include <program_loading.h>
#include <device/smbus_host.h>
#include <device/dram/ddr4.h>
#include <lib.h>	// hexdump
#include <spd_bin.h>

void main(void)
{
	int i;
	dimm_attr dimm[CONFIG_DIMM_MAX];

	console_init();
	vpd_pnor_main();

	struct spd_block blk = {
		.addr_map = { DIMM0, DIMM1, DIMM2, DIMM3, DIMM4, DIMM5, DIMM6, DIMM7},
	};

	get_spd_smbus(&blk);
	hexdump(blk.spd_array[2], blk.len);
	dump_spd_info(&blk);

	for (i = 0; i < CONFIG_DIMM_MAX; i++) {
		spd_decode_ddr4(&dimm[i], blk.spd_array[i]);
	}

	run_ramstage();
}
