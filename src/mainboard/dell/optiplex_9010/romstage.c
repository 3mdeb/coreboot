/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008-2009 coresystems GmbH
 * Copyright (C) 2014 Vladimir Serbinenko
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdint.h>
#include <string.h>
#include <lib.h>
#include <timestamp.h>
#include <arch/byteorder.h>
#include <arch/io.h>
#include <device/pci_def.h>
#include <device/pci_ops.h>
#include <device/pnp_def.h>
#include <cpu/x86/lapic.h>
#include <arch/acpi.h>
#include <console/console.h>
#include <northbridge/intel/sandybridge/sandybridge.h>
#include <northbridge/intel/sandybridge/raminit_native.h>
#include <southbridge/intel/bd82x6x/pch.h>
#include <southbridge/intel/common/gpio.h>
#include <arch/cpu.h>
#include <cpu/x86/msr.h>
#include <superio/smsc/sch5545/sch5545.h>

void pch_enable_lpc(void)
{
	pci_write_config16(PCI_DEV(0, 0x1f, 0), 0x82, 0x3c01);
	pci_write_config32(PCI_DEV(0, 0x1f, 0), 0x84, 0x007c0a01);
	pci_write_config32(PCI_DEV(0, 0x1f, 0), 0x88, 0x007c0901);
	pci_write_config32(PCI_DEV(0, 0x1f, 0), 0x8c, 0x003c07e1);
	pci_write_config32(PCI_DEV(0, 0x1f, 0), 0x90, 0x001c0901);
	pci_write_config16(PCI_DEV(0, 0x1f, 0), 0x80, 0x0000);
}

void mainboard_rcba_config(void)
{
}

const struct southbridge_usb_port mainboard_usb_ports[] = {
	{ 1, 0, 0 },
	{ 1, 0, 1 },
	{ 1, 1, 2 },
	{ 1, 1, 3 },
	{ 1, 1, -1 },
	{ 1, 1, -1 },
	{ 1, 0, -1 },
	{ 1, 0, -1 },
	{ 1, 0, 4 },
	{ 1, 0, 4 },
	{ 1, 0, 5 },
	{ 1, 1, 5 },
	{ 1, 1, 6 },
	{ 1, 0, 6 },
};

void mainboard_early_init(int s3resume)
{
}

void mainboard_config_superio(void)
{
	// sch5545_early_init(0x2e);
}

/* FIXME: Put proper SPD map here. */
void mainboard_get_spd(spd_raw_data *spd, bool id_only)
{
	read_spd(&spd[0], 0x50, id_only);
	read_spd(&spd[1], 0x51, id_only);
	read_spd(&spd[2], 0x52, id_only);
	read_spd(&spd[3], 0x53, id_only);
}
