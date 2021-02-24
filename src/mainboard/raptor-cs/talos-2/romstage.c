/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/vpd.h>
#include <cpu/power/istep_13.h>
#include <program_loading.h>
#include <device/smbus_host.h>
#include <lib.h>	// hexdump
#include <spd_bin.h>

void main(void)
{
	int i;
	dimm_attr dimms[CONFIG_DIMM_MAX] = {0};

	console_init();
	vpd_pnor_main();

	struct spd_block blk = {
		.addr_map = { DIMM0, DIMM1, DIMM2, DIMM3, DIMM4, DIMM5, DIMM6, DIMM7},
	};

	get_spd_smbus(&blk);
	hexdump(blk.spd_array[2], blk.len);
	dump_spd_info(&blk);

	for (i = 0; i < CONFIG_DIMM_MAX; i++) {
		if (blk.spd_array[i] != NULL)
			spd_decode_ddr4(&dimms[i], blk.spd_array[i]);
	}

	report_istep(13,1);	// no-op
	istep_13_2(dimms);
	istep_13_3(&blk);
	istep_13_4(dimms);

	run_ramstage();
}
