/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <device/device.h>
#include <cbmem.h>

static void mainboard_enable(struct device *dev)
{

	if (!dev) {
		die("No dev0; die\n");
	}

	/*
	 * Smallest reported to be working (but not officially supported) DIMM is
	 * 4GB. This means that we always have at least as much available. Last
	 * 128MB of first 4GB are reserved for hostboot/coreboot - this is mostly
	 * dictated by HRMOR value.
	 *
	 * TODO: implement this properly for all RAM
	 */
	ram_resource(dev, 0, 0, 4 * 1024 * 1024 - 128 * 1024);
	reserved_ram_resource(dev, 1, 4 * 1024 * 1024 - 128 * 1024, 128 * 1024);
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
};
