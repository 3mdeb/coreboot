/* SPDX-License-Identifier: GPL-2.0-only */

#include <program_loading.h>

#if ENV_PAYLOAD_LOADER

#include <device_tree.h>
#include <fit.h>

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

static void *fdt_prepare(void)
{
	struct device_tree *dt = fdt_unflatten(&fdt_buf[0]);

	fit_update_memory(dt);

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
