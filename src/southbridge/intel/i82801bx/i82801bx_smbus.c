/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2005 Yinghai Lu <yinghailu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <stdint.h>
#include <device/smbus.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <arch/io.h>
#include "i82801bx.h"
#include "i82801bx_smbus.h"

static int lsmbus_read_byte(device_t dev, u8 address)
{
	u16 device;
	struct resource *res;
	struct bus *pbus;

	device = dev->path.i2c.device;
	pbus = get_pbus_smbus(dev);
	res = find_resource(pbus->dev, 0x20);

	return do_smbus_read_byte(res->base, device, address);
}

static struct smbus_bus_operations lops_smbus_bus = {
	.read_byte	= lsmbus_read_byte,
};

static const struct device_operations smbus_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.init			= 0,
	.scan_bus		= scan_static_bus,
	.enable			= i82801bx_enable,
	.ops_smbus_bus		= &lops_smbus_bus,
};

/* 82801AA (ICH) */
static const struct pci_driver i82801aa_smb __pci_driver = {
	.ops	= &smbus_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= PCI_DEVICE_ID_INTEL_82801AA_SMB,
};

/* 82801AB (ICH0) */
static const struct pci_driver i82801ab_smb __pci_driver = {
	.ops	= &smbus_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= PCI_DEVICE_ID_INTEL_82801AB_SMB,
};

/* 82801BA/BAM (ICH2/ICH2-M) */
static const struct pci_driver i82801ba_smb __pci_driver = {
	.ops	= &smbus_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= PCI_DEVICE_ID_INTEL_82801BA_SMB,
};

/* 82801CA/CAM (ICH3-S/ICH3-M) */
static const struct pci_driver i82801ca_smb __pci_driver = {
	.ops	= &smbus_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= PCI_DEVICE_ID_INTEL_82801CA_SMB,
};

/* 82801DB/DBL/DBM (ICH4/ICH4-L/ICH4-M) */
static const struct pci_driver i82801db_smb __pci_driver = {
	.ops	= &smbus_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= PCI_DEVICE_ID_INTEL_82801DB_SMB,
};

/* 82801EB/ER (ICH5/ICH5R) */
static const struct pci_driver i82801eb_smb __pci_driver = {
	.ops	= &smbus_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= PCI_DEVICE_ID_INTEL_82801EB_SMB,
};

/* 82801FB/FR/FW/FRW/FBM (ICH6/ICH6R/ICH6W/ICH6RW/ICH6-M) */
static const struct pci_driver i82801fb_smb __pci_driver = {
	.ops	= &smbus_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= PCI_DEVICE_ID_INTEL_82801FB_SMB,
};

/* 82801E (C-ICH) */
static const struct pci_driver i82801e_smb __pci_driver = {
	.ops	= &smbus_ops,
	.vendor	= PCI_VENDOR_ID_INTEL,
	.device	= PCI_DEVICE_ID_INTEL_82801E_SMB,
};
