/* SPDX-License-Identifier: GPL-2.0-only */

#include <boot_device.h>
#include <cbmem.h>
#include <console/console.h>
#include <fmap.h>
#include <stddef.h>
#include <string.h>
#include <symbols.h>
#include <endian.h>

#include "fmap_config.h"

/*
 * See http://code.google.com/p/flashmap/ for more information on FMAP.
 */

static int fmap_print_once;
static struct mem_region_device fmap_cache;

#define print_once(...) do { \
		if (!fmap_print_once) \
			printk(__VA_ARGS__); \
	} while (0)

uint64_t get_fmap_flash_offset(void)
{
	return FMAP_OFFSET;
}

static int check_signature(const struct fmap *fmap)
{
	return memcmp(fmap->signature, FMAP_SIGNATURE, sizeof(fmap->signature));
}

static void report(const struct fmap *fmap)
{
	print_once(BIOS_DEBUG, "FMAP: Found \"%s\" version %d.%d at %#x.\n",
	       fmap->name, fmap->ver_major, fmap->ver_minor, FMAP_OFFSET);
	print_once(BIOS_DEBUG, "FMAP: base = %#llx size = %#x #areas = %d\n",
	       (long long)le64toh(fmap->base), le32toh(fmap->size),
	       le16toh(fmap->nareas));
	fmap_print_once = 1;
}

static void setup_preram_cache(struct mem_region_device *cache_mrdev)
{
	if (CONFIG(NO_FMAP_CACHE))
		return;

	/* No need to use FMAP cache in SMM */
	if (ENV_SMM)
		return;

	if (!ENV_ROMSTAGE_OR_BEFORE) {
		/* We get here if ramstage makes an FMAP access before calling
		   cbmem_initialize(). We should avoid letting it come to that,
		   so print a warning. */
		print_once(BIOS_WARNING,
			"WARNING: Post-RAM FMAP access too early for cache!\n");
		return;
	}

	struct fmap *fmap = (struct fmap *)_fmap_cache;
	if (!(ENV_INITIAL_STAGE)) {
		/* NOTE: This assumes that the first stage will make
		   at least one FMAP access (usually from finding CBFS). */
		if (!check_signature(fmap))
			goto register_cache;

		printk(BIOS_ERR, "ERROR: FMAP cache corrupted?!\n");
	}

	/* In case we fail below, make sure the cache is invalid. */
	memset(fmap->signature, 0, sizeof(fmap->signature));

	boot_device_init();
	const struct region_device *boot_rdev = boot_device_ro();
	if (!boot_rdev)
		return;

	/* memlayout statically guarantees that the FMAP_CACHE is big enough. */
	if (rdev_readat(boot_rdev, fmap, FMAP_OFFSET, FMAP_SIZE) != FMAP_SIZE)
		return;
	if (check_signature(fmap))
		return;
	report(fmap);

register_cache:
	mem_region_device_ro_init(cache_mrdev, fmap, FMAP_SIZE);
}

static int find_fmap_directory(struct region_device *fmrd)
{
	const struct region_device *boot;
	struct fmap *fmap;
	size_t offset = FMAP_OFFSET;

	/* Try FMAP cache first */
	if (!region_device_sz(&fmap_cache.rdev))
		setup_preram_cache(&fmap_cache);
	if (region_device_sz(&fmap_cache.rdev))
		return rdev_chain_full(fmrd, &fmap_cache.rdev);

	boot_device_init();
	boot = boot_device_ro();

	if (boot == NULL)
		return -1;

	fmap = rdev_mmap(boot, offset, sizeof(struct fmap));

	if (fmap == NULL)
		return -1;

	if (check_signature(fmap)) {
		printk(BIOS_DEBUG, "No FMAP found at %zx offset.\n", offset);
		rdev_munmap(boot, fmap);
		return -1;
	}

	report(fmap);

	rdev_munmap(boot, fmap);

	return rdev_chain(fmrd, boot, offset, FMAP_SIZE);
}

int fmap_locate_area_as_rdev(const char *name, struct region_device *area)
{
	struct region ar;

	if (fmap_locate_area(name, &ar))
		return -1;

	return boot_device_ro_subregion(&ar, area);
}

int fmap_locate_area_as_rdev_rw(const char *name, struct region_device *area)
{
	struct region ar;

	if (fmap_locate_area(name, &ar))
		return -1;

	return boot_device_rw_subregion(&ar, area);
}

int fmap_locate_area(const char *name, struct region *ar)
{
	struct region_device fmrd;
	size_t offset;

	if (find_fmap_directory(&fmrd))
		return -1;

	/* Start reading the areas just after fmap header. */
	offset = sizeof(struct fmap);

	while (1) {
		struct fmap_area *area;

		area = rdev_mmap(&fmrd, offset, sizeof(*area));

		if (area == NULL)
			return -1;

		if (strcmp((const char *)area->name, name)) {
			rdev_munmap(&fmrd, area);
			offset += sizeof(struct fmap_area);
			continue;
		}

		printk(BIOS_DEBUG, "FMAP: area %s found @ %x (%d bytes)\n",
		       name, le32toh(area->offset), le32toh(area->size));

		ar->offset = le32toh(area->offset);
		ar->size = le32toh(area->size);

		rdev_munmap(&fmrd, area);

		return 0;
	}

	printk(BIOS_DEBUG, "FMAP: area %s not found\n", name);

	return -1;
}

int fmap_find_region_name(const struct region * const ar,
	char name[FMAP_STRLEN])
{
	struct region_device fmrd;
	size_t offset;

	if (find_fmap_directory(&fmrd))
		return -1;

	/* Start reading the areas just after fmap header. */
	offset = sizeof(struct fmap);

	while (1) {
		struct fmap_area *area;

		area = rdev_mmap(&fmrd, offset, sizeof(*area));

		if (area == NULL)
			return -1;

		if ((ar->offset != le32toh(area->offset)) ||
		    (ar->size != le32toh(area->size))) {
			rdev_munmap(&fmrd, area);
			offset += sizeof(struct fmap_area);
			continue;
		}

		printk(BIOS_DEBUG, "FMAP: area (%zx, %zx) found, named %s\n",
			ar->offset, ar->size, area->name);

		memcpy(name, area->name, FMAP_STRLEN);

		rdev_munmap(&fmrd, area);

		return 0;
	}

	printk(BIOS_DEBUG, "FMAP: area (%zx, %zx) not found\n",
		ar->offset, ar->size);

	return -1;
}

ssize_t fmap_read_area(const char *name, void *buffer, size_t size)
{
	struct region_device rdev;
	if (fmap_locate_area_as_rdev(name, &rdev))
		return -1;
	return rdev_readat(&rdev, buffer, 0,
			MIN(size, region_device_sz(&rdev)));
}

ssize_t fmap_overwrite_area(const char *name, const void *buffer, size_t size)
{
	struct region_device rdev;

	if (fmap_locate_area_as_rdev_rw(name, &rdev))
		return -1;
	if (size > region_device_sz(&rdev))
		return -1;
	if (rdev_eraseat(&rdev, 0, region_device_sz(&rdev)) < 0)
		return -1;
	return rdev_writeat(&rdev, buffer, 0, size);
}

static void fmap_register_cbmem_cache(int unused)
{
	const struct cbmem_entry *e;

	/* Find the FMAP cache installed by previous stage */
	e = cbmem_entry_find(CBMEM_ID_FMAP);
	/* Don't set fmap_cache so that find_fmap_directory will use regular path */
	if (!e)
		return;

	mem_region_device_ro_init(&fmap_cache, cbmem_entry_start(e), cbmem_entry_size(e));
}

/*
 * The main reason to copy the FMAP into CBMEM is to make it available to the
 * OS on every architecture. As side effect use the CBMEM copy as cache.
 */
static void fmap_setup_cbmem_cache(int unused)
{
	struct region_device fmrd;

	if (find_fmap_directory(&fmrd))
		return;

	/* Reloads the FMAP even on ACPI S3 resume */
	const size_t s = region_device_sz(&fmrd);
	struct fmap *fmap = cbmem_add(CBMEM_ID_FMAP, s);
	if (!fmap) {
		printk(BIOS_ERR, "ERROR: Failed to allocate CBMEM\n");
		return;
	}

	const ssize_t ret = rdev_readat(&fmrd, fmap, 0, s);
	if (ret != s) {
		printk(BIOS_ERR, "ERROR: Failed to read FMAP into CBMEM\n");
		cbmem_entry_remove(cbmem_entry_find(CBMEM_ID_FMAP));
		return;
	}

	/* Finally advertise the cache for the current stage */
	fmap_register_cbmem_cache(unused);
}

ROMSTAGE_CBMEM_INIT_HOOK(fmap_setup_cbmem_cache)
RAMSTAGE_CBMEM_INIT_HOOK(fmap_register_cbmem_cache)
POSTCAR_CBMEM_INIT_HOOK(fmap_register_cbmem_cache)
