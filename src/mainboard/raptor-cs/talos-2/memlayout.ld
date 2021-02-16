/* SPDX-License-Identifier: GPL-2.0-only */

#include <memlayout.h>

#include <arch/header.ld>

// TODO: fill in these blanks for Power9.
SECTIONS
{
	DRAM_START(0x0)
	// TODO: add a config option for building for SEEPROM
	BOOTBLOCK(0x3000, 20K)
	STACK(0x8000, 0x8000)
	ROMSTAGE(0x10000, 128K)
	PRERAM_CBMEM_CONSOLE(0x30000, 8K)
	FMAP_CACHE(0x32000, 2K)
	CBFS_MCACHE(0x33000, 8K)
	RAMSTAGE(0x35000, 1M)
}
