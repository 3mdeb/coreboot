/* SPDX-License-Identifier: GPL-2.0-only */

#include <boot_device.h>
#include <arch/io.h>
#include <console/console.h>
#include "../../../../3rdparty/ffs/ffs/ffs.h"

/* Base is not NULL on real hardware, it will be patched later */
static struct mem_region_device boot_dev =
	MEM_REGION_DEV_RO_INIT(NULL, CONFIG_ROM_SIZE);

#define LPC_FLASH_MIN (MMIO_GROUP0_CHIP0_LPC_BASE_ADDR + LPCHC_FW_SPACE)
#define LPC_FLASH_TOP (LPC_FLASH_MIN + FW_SPACE_SIZE)

static char *find_cbfs_in_pnor(void)
{
	char *cbfs = NULL;
	int i;
	struct ffs_hdr *hdr = (struct ffs_hdr *)LPC_FLASH_TOP;

	while (hdr > (struct ffs_hdr *)LPC_FLASH_MIN) {
		uint32_t csum = 0;
		/* Assume block_size = 4K */
		hdr = (struct ffs_hdr *)(((char *)hdr) - 0x1000);

		if (hdr->magic != FFS_MAGIC)
			continue;
		if (hdr->version != FFS_VERSION_1)
			continue;

		csum = hdr->magic ^ hdr->version ^ hdr->size ^ hdr->entry_size ^
		       hdr->entry_count ^ hdr->block_size ^ hdr->block_count ^
		       hdr->resvd[0] ^ hdr->resvd[1] ^ hdr->resvd[2] ^ hdr->resvd[3] ^
		       hdr->checksum;
		if (csum == 0) break;
	}

	if (hdr <= (struct ffs_hdr *)LPC_FLASH_MIN)
		return NULL;

	printk(BIOS_DEBUG, "Found FFS header at %p\n", hdr);
	printk(BIOS_SPEW, "    size %x\n", hdr->size);
	printk(BIOS_SPEW, "    entry_size %x\n", hdr->entry_size);
	printk(BIOS_SPEW, "    entry_count %x\n", hdr->entry_count);
	printk(BIOS_SPEW, "    block_size %x\n", hdr->block_size);
	printk(BIOS_SPEW, "    block_count %x\n", hdr->block_count);
	printk(BIOS_DEBUG, "PNOR base at %lx\n",
	       LPC_FLASH_TOP - hdr->block_size * hdr->block_count);

	for (i = 0; i < hdr->entry_count; i++) {
		struct ffs_entry *e = &hdr->entries[i];
		printk(BIOS_SPEW, "%s: base %x, size %x  (%x)\n\t type %x, flags %x\n",
		       e->name, e->base, e->size, e->actual, e->type, e->flags);
		// TODO: find CBFS partition, calculate address, remember about partition header
	}

	return cbfs;
}

const struct region_device *boot_device_ro(void)
{
	if (boot_dev.base == NULL)
		boot_dev.base = find_cbfs_in_pnor();

	return &boot_dev.rdev;
}
