/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <device/device.h>
#include <cbmem.h>

#include <fit.h>
#include <console/console.h>

static int dt_platform_fixup(struct device_tree_fixup *fixup,
			      struct device_tree *tree)
{
	struct device_tree_node *node;
	//~ uint32_t i = 0x44;

	printk(BIOS_ERR, "dt_platform_fixup called **************************\n");

	/* Memory devices are always direct children of root */
	list_for_each(node, tree->root->children, list_node) {
		const char *devtype = dt_find_string_prop(node, "device_type");
		if (devtype && !strcmp(devtype, "memory")) {
			dt_add_u32_prop(node, "ibm,chip-id", 0 /* FIXME for second CPU */);
		}
	}

	dt_print_node(tree->root);

	printk(BIOS_ERR, "dt_platform_fixup exiting **************************\n");
	return 0;
}

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
	ram_resource(dev, 0, 0, 4 * 1024 * 1024 - 256 * 1024);
	reserved_ram_resource(dev, 1, 4 * 1024 * 1024 - 256 * 1024, 256 * 1024);

	if (CONFIG(PAYLOAD_FIT_SUPPORT)) {
		struct device_tree_fixup *dt_fixup;

		dt_fixup = malloc(sizeof(*dt_fixup));
		if (dt_fixup) {
			dt_fixup->fixup = dt_platform_fixup;
			list_insert_after(&dt_fixup->list_node,
					  &device_tree_fixups);
		}
	}
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
};
