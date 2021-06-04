/* SPDX-License-Identifier: GPL-2.0-only */

#include <program_loading.h>

#if ENV_PAYLOAD_LOADER

#include <device_tree.h>
#include <fit.h>
#include <commonlib/stdlib.h>

/* Empty FDT */
static uint32_t fdt_buf[100*1024] =
{
	FDT_HEADER_MAGIC,		/* uint32_t magic; */
	sizeof(struct fdt_header),	/* uint32_t totalsize; */
	sizeof(struct fdt_header),	/* uint32_t structure_offset; */
	sizeof(struct fdt_header),	/* uint32_t strings_offset; */
	sizeof(struct fdt_header),	/* uint32_t reserve_map_offset; */

	FDT_SUPPORTED_VERSION,		/* uint32_t version; */
	16,				/* uint32_t last_comp_version; */

	4,				/* uint32_t boot_cpuid_phys; */

	0,				/* uint32_t strings_size; */
	0,				/* uint32_t structure_size; */
};

static uint8_t xscom_compat[] = "ibm,xscom\0ibm,power9-xscom\0";
static uint64_t xscom_addrs[] = { 0x000603fc00000000 };
static uint64_t xscom_sizes[] = { 0x0000000800000000 };
static uint8_t chiptod_compat[] = "ibm,power9-chiptod\0ibm,power-chiptod\0";
static uint64_t chiptod_addrs[] = { 0x40000 };
static uint64_t chiptod_sizes[] = { 0x34 };
static uint64_t cpu_addrs[] = { 0x4, 0xc, 0x10, 0x1c };
static uint8_t lpcm_compat[] = "ibm,power9-lpcm-opb\0simple-bus\0";
static uint64_t lpcm_addrs[] = { 0x0006030000000000 };
static uint64_t lpcm_sizes[] = { 0x0000000100000000 };
static uint32_t lpcm_ranges[] = { 0x00, 0x60300, 0x00, 0x80000000, 0x80000000,
	                          0x60300, 0x80000000, 0x80000000 };
static uint8_t lpc_compat[] = "ibm,power9-lpc\0ibm,power8-lpc\0";
static uint64_t serial_addrs[] = { 0x00000001000003f8 };
static uint64_t serial_sizes[] = { 1 };

static void *fdt_prepare(void)
{
	struct device_tree *dt = fdt_unflatten(&fdt_buf[0]);
	struct device_tree_node *node, *subnode, *subsubnode;

	fit_update_memory(dt);

	dt_add_u32_prop(dt->root, "#address-cells", 2);
	dt_add_u32_prop(dt->root, "#size-cells", 2);


	node = xzalloc(sizeof(*node));
	node->name = "xscom"; /* FIXME: shall include the unit address */
	dt_add_bin_prop(node, "primary", NULL, 0);
	dt_add_reg_prop(node, xscom_addrs, xscom_sizes, 1, 2, 2);
	dt_add_u32_prop(node, "#address-cells", 1);
	dt_add_u32_prop(node, "#size-cells", 1);
	//~ dt_add_u32_prop(node, "primary", 0);
	dt_add_u32_prop(node, "ibm,chip-id", 0 /* FIXME for second CPU */);
	dt_add_bin_prop(node, "compatible", xscom_compat, sizeof(xscom_compat));
	list_insert_after(&node->list_node, &dt->root->children);

	subnode = xzalloc(sizeof(*subnode));
	subnode->name = "chiptod";
	dt_add_bin_prop(subnode, "primary", NULL, 0);
	dt_add_reg_prop(subnode, chiptod_addrs, chiptod_sizes, 1, 1, 1);
	dt_add_bin_prop(subnode, "compatible", chiptod_compat, sizeof(chiptod_compat));
	list_insert_after(&subnode->list_node, &node->children);


	node = xzalloc(sizeof(*node));
	node->name = "cpus";
	dt_add_u32_prop(node, "#address-cells", 1);
	dt_add_u32_prop(node, "#size-cells", 0);
	list_insert_after(&node->list_node, &dt->root->children);

	subnode = xzalloc(sizeof(*subnode));
	subnode->name = "PowerPC,POWER9@4";
	dt_add_reg_prop(subnode, &cpu_addrs[0], NULL, 1, 1, 0);
	dt_add_string_prop(subnode, "device_type", (char *)"cpu");
	dt_add_string_prop(subnode, "status", (char *)"okay");
	dt_add_u32_prop(subnode, "ibm,chip-id", 0 /* FIXME for second CPU */);
	dt_add_u32_prop(subnode, "clock-frequency", 0xa0eebb00); /* 512MHz */
	dt_add_u32_prop(subnode, "timebase-frequency", 0x1e848000); /* 2.7GHz */
	list_insert_after(&subnode->list_node, &node->children);

	subnode = xzalloc(sizeof(*subnode));
	subnode->name = "PowerPC,POWER9@c";
	dt_add_reg_prop(subnode, &cpu_addrs[1], NULL, 1, 1, 0);
	dt_add_string_prop(subnode, "device_type", (char *)"cpu");
	dt_add_string_prop(subnode, "status", (char *)"disabled");
	dt_add_u32_prop(subnode, "ibm,chip-id", 0 /* FIXME for second CPU */);
	dt_add_u32_prop(subnode, "clock-frequency", 0xa0eebb00); /* 512MHz */
	dt_add_u32_prop(subnode, "timebase-frequency", 0x1e848000); /* 2.7GHz */
	list_insert_after(&subnode->list_node, &node->children);

	subnode = xzalloc(sizeof(*subnode));
	subnode->name = "PowerPC,POWER9@10";
	dt_add_reg_prop(subnode, &cpu_addrs[2], NULL, 1, 1, 0);
	dt_add_string_prop(subnode, "device_type", (char *)"cpu");
	dt_add_string_prop(subnode, "status", (char *)"disabled");
	dt_add_u32_prop(subnode, "ibm,chip-id", 0 /* FIXME for second CPU */);
	dt_add_u32_prop(subnode, "clock-frequency", 0xa0eebb00); /* 512MHz */
	dt_add_u32_prop(subnode, "timebase-frequency", 0x1e848000); /* 2.7GHz */
	list_insert_after(&subnode->list_node, &node->children);

	subnode = xzalloc(sizeof(*subnode));
	subnode->name = "PowerPC,POWER9@1c";
	dt_add_reg_prop(subnode, &cpu_addrs[3], NULL, 1, 1, 0);
	dt_add_string_prop(subnode, "device_type", (char *)"cpu");
	dt_add_string_prop(subnode, "status", (char *)"disabled");
	dt_add_u32_prop(subnode, "ibm,chip-id", 0 /* FIXME for second CPU */);
	dt_add_u32_prop(subnode, "clock-frequency", 0xa0eebb00); /* 512MHz */
	dt_add_u32_prop(subnode, "timebase-frequency", 0x1e848000); /* 2.7GHz */
	list_insert_after(&subnode->list_node, &node->children);


	node = xzalloc(sizeof(*node));
	node->name = "lpcm-opb";
	dt_add_reg_prop(node, lpcm_addrs, lpcm_sizes, 1, 2, 2);
	dt_add_u32_prop(node, "#address-cells", 1);
	dt_add_u32_prop(node, "#size-cells", 1);
	dt_add_bin_prop(node, "compatible", lpcm_compat, sizeof(lpcm_compat));
	dt_add_bin_prop(node, "ranges", lpcm_ranges, sizeof(lpcm_ranges));
	dt_add_u32_prop(node, "ibm,chip-id", 0 /* FIXME for second CPU */);
	list_insert_after(&node->list_node, &dt->root->children);

	subnode = xzalloc(sizeof(*subnode));
	subnode->name = "lpc";
	dt_add_u32_prop(subnode, "#address-cells", 2);
	dt_add_u32_prop(subnode, "#size-cells", 1);
	dt_add_bin_prop(subnode, "compatible", lpc_compat, sizeof(lpc_compat));
	list_insert_after(&subnode->list_node, &node->children);

	subsubnode = xzalloc(sizeof(*subnode));
	subsubnode->name = "serial";
	dt_add_reg_prop(subsubnode, serial_addrs, serial_sizes, 1, 2, 1);
	dt_add_string_prop(subsubnode, "device_type", (char *)"serial");
	dt_add_string_prop(subsubnode, "compatible", (char *)"ns16550");
	dt_add_u32_prop(subsubnode, "clock-frequency", 0x1c2000); /* 512MHz */
	dt_add_u32_prop(subsubnode, "current-speed", 0x1c200); /* 2.7GHz */
	list_insert_after(&subsubnode->list_node, &subnode->children);


	/* Adding 'bmc' node significantly changes skiboot's flow */
	node = xzalloc(sizeof(*node));
	node->name = "bmc";
	dt_add_string_prop(subsubnode, "compatible", (char *)"ibm,ast2500,openbmc");
	list_insert_after(&node->list_node, &dt->root->children);


	dt_apply_fixups(dt);

	/* Repack FDT for handoff to kernel */
	dt_flatten(dt, &fdt_buf[0]);

	return &fdt_buf[0];
}

/*
 * Payload's entry point is an offset to the real entry point, not to OPD
 * (Official Procedure Descriptor) for entry point.
 */
void arch_prog_run(struct prog *prog)
{
	asm volatile(
	    "mtctr %1\n"
	    "mr 3, %0\n"
	    "bctr\n"
	    :: "r"(fdt_prepare()), "r"(prog_entry(prog)) : "memory");
}

#else

void arch_prog_run(struct prog *prog)
{
	void (*doit)(void *) = prog_entry(prog);

	doit(prog_entry_arg(prog));
}

#endif
