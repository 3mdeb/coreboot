/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Based on SMSC SCH5627 datahseet:
 * http://pdf.datasheetcatalog.com/datasheets/microchip/00001996A.pdf
 */

#include <arch/io.h>
#include <device/device.h>
#include <device/pnp.h>
#include <superio/conf_mode.h>
#include <console/console.h>
#include <pc80/keyboard.h>
#include <stdlib.h>

#include "sch5545.h"

static void sch5545_set_led_on(u16 runtime_reg_base)
{
	u8 val = SCH5545_LED_BLINK_ON | SCH5545_LED_COLOR_GREEN |
		 SCH5545_LED_CODE_FETCH;
	outb(val, runtime_reg_base + SCH5545_RR_LED);
}

static void sch5545_init(struct device *dev)
{
	if (!dev->enabled) {
		return;
	}

	switch (dev->path.pnp.device) {
		case SCH5545_LDN_KBC:
			pc_keyboard_init(NO_AUX_DEVICE);
			break;
		case SCH5545_LDN_RR:
			sch5545_set_led_on(SCH5545_RUNTIME_REG_BASE);
			break;
	}
}

static void sch5545_set_iobase(struct device *dev, u8 index, u16 iobase)
{
	u8 val;
	struct device *lpc_if;

	/* All IO base addresses are configured via LPC LDN */
	lpc_if = (struct device *) malloc(sizeof(struct device *));
	lpc_if->path.type = DEVICE_PATH_PNP;
	lpc_if->path.pnp.device = SCH5545_LDN_LPC_IF;
	lpc_if->path.pnp.port = dev->path.pnp.port;	
	pnp_set_logical_device(dev);

	/* Flip the bytes in IO base, LSB goes first */
	pnp_write_config(lpc_if, index + 0, iobase & 0xff);
	pnp_write_config(lpc_if, index + 1, (iobase >> 8) & 0xff);

	/* set valid */
	val = pnp_read_config(lpc_if, index - 1);
	val |= 0x80;
	pnp_write_config(lpc_if, index - 1 , val);
}

static void sch5545_set_irq(struct device *dev, u8 index, u8 irq)
{
	struct device *lpc_if;
	u8 select_bit = 0;

	/* All IRQs are programmed via LPC LDN */
	lpc_if = (struct device *) malloc(sizeof(struct device *));
	lpc_if->path.type = DEVICE_PATH_PNP;
	lpc_if->path.pnp.device = SCH5545_LDN_LPC_IF;
	lpc_if->path.pnp.port = dev->path.pnp.port;
	pnp_set_logical_device(lpc_if);

	/* 
	 * Some LDNs can generate IRQs from two sources, i.e.
	 * - KBC may generate separate IRQ for maouse and keyboard,
	 * - RR LDN may generate IRQ for PME and SMI etc.
	 * SELECT bit allows to distinguish IRQ source for single LDN.
	 * Use the standard IRQs for devices.
	 */
	switch (dev->path.pnp.device) {
		case SCH5545_LDN_EMI:
			/* EC-to-Host may generate SMI, use select bit */
			if (irq == 2)
				select_bit = 0x80;	
			break;
		case SCH5545_LDN_KBC:
			/* set SELECT bit for mouse */
			if (irq == 12)
				select_bit = 0x80;
			break;
		case SCH5545_LDN_RR:
			/* Runtime registers can generate SMI from intrusion
			 * switch or GPIOs etc, thus should use select bit.
			 * We use IRQ9 here to distinguish RR from EMI.
			 */
			if (irq == 9)
				select_bit = 0x80;
			break;
		default:
			break;
	}

	/* 
	 * IRQs are set in a little different manner. Each IRQ number has its
	 * own register which is programmed with LDN number which should use
	 * the IRQ. Ignore the index offset and choose register based on IRQ
	 * number counting from IRQ base.
	 */
	pnp_write_config(dev, SCH5545_IRQ_BASE + irq, dev->path.pnp.device |
						      select_bit);

	pnp_set_logical_device(dev);
}

static void sch5545_set_drq(struct device *dev, u8 index, u8 drq)
{
	struct device *lpc_if;

	if (drq == 4) {
		printk(BIOS_ERR, "ERROR: %s %02x: Trying to set reserved"
				 "DRQ 4!\n", dev_path(dev), index);
	}

	/* DMA is programmed via LPC LDN */
	lpc_if = (struct device *) malloc(sizeof(struct device *));
	lpc_if->path.type = DEVICE_PATH_PNP;
	lpc_if->path.pnp.device = SCH5545_LDN_LPC_IF;
	lpc_if->path.pnp.port = dev->path.pnp.port;
	pnp_set_logical_device(lpc_if);

	/* 
	 * There are 8 configurable DMA channels. DRQs are set in a little
	 * different manner. Each DMA channel has its own 16-bit register which
	 * is programmed with LDN number (in higher byte) which should use the
	 * IRQ. Ignore the index offset and choose register based on IRQ number
	 * counting from IRQ base. Set valid bit (bit 7) additionally.
	 */
	pnp_write_config(dev, SCH5545_DRQ_BASE + (drq * 2) + 1,
			 dev->path.pnp.device | 0x80);

	pnp_set_logical_device(dev);
}

static void sch5545_set_resource(struct device *dev, struct resource *resource)
{
	if (!(resource->flags & IORESOURCE_ASSIGNED)) {
		/* The PNP_MSC super IO registers have the IRQ flag set. If no
		   value is assigned in the devicetree, the corresponding
		   PNP_MSC register doesn't get written, which should be printed
		   as warning and not as error. */
		if (resource->flags & IORESOURCE_IRQ &&
		    (resource->index != PNP_IDX_IRQ0) &&
		    (resource->index != PNP_IDX_IRQ1))
			printk(BIOS_WARNING, "WARNING: %s %02lx %s size: "
			       "0x%010llx not assigned\n", dev_path(dev),
			       resource->index, resource_type(resource),
			       resource->size);
		else
			printk(BIOS_ERR, "ERROR: %s %02lx %s size: 0x%010llx "
			       "not assigned\n", dev_path(dev), resource->index,
			       resource_type(resource), resource->size);
		return;
	}

	/* Now store the resource. */
	if (resource->flags & IORESOURCE_IO) {
		sch5545_set_iobase(dev, resource->index, resource->base);
	} else if (resource->flags & IORESOURCE_DRQ) {
		sch5545_set_drq(dev, resource->index, resource->base);
	} else if (resource->flags & IORESOURCE_IRQ) {
		sch5545_set_irq(dev, resource->index, resource->base);
	} else {
		printk(BIOS_ERR, "ERROR: %s %02lx unknown resource type\n",
		       dev_path(dev), resource->index);
		return;
	}
	resource->flags |= IORESOURCE_STORED;

	report_resource_stored(dev, resource, "");
}

static void sch5545_set_resources(struct device *dev)
{
	struct resource *res;

	pnp_enter_conf_mode(dev);

	/* Select the logical device (LDN). */
	pnp_set_logical_device(dev);

	/* Paranoia says I should disable the device here... */
	for (res = dev->resource_list; res; res = res->next)
		sch5545_set_resource(dev, res);

	pnp_exit_conf_mode(dev);
}

static struct device_operations ops = {
	.read_resources   = pnp_read_resources,
	.set_resources    = sch5545_set_resources,
	.enable_resources = pnp_enable_resources,
	.enable           = pnp_alt_enable,
	.init             = sch5545_init,
	.ops_pnp_mode     = &pnp_conf_mode_55_aa,
};

static struct pnp_info pnp_dev_info[] = {
	{ &ops, SCH5545_LDN_EMI, PNP_IO0 | PNP_IRQ0 | PNP_IRQ1 , 0x0ff0, },
	{ &ops, SCH5545_LDN_KBC, PNP_IO0 | PNP_IRQ0 | PNP_IRQ1 | PNP_MSC0 |
				 PNP_MSC1,
	  0x0ffc },
	{ &ops, SCH5545_LDN_UART1, PNP_IO0 | PNP_IRQ0 | PNP_MSC0, 0x0ff8, },
	{ &ops, SCH5545_LDN_UART2, PNP_IO0 | PNP_IRQ0 | PNP_MSC0, 0x0ff8, },
	{ &ops, SCH5545_LDN_RR, PNP_IO0 | PNP_IRQ0 | PNP_IRQ1 | PNP_MSC0,
	  0x0fc0 },
	{ &ops, SCH5545_LDN_FDC, PNP_IO0 | PNP_IRQ0 | PNP_DRQ0 | PNP_MSC0 |
				 PNP_MSC1 | PNP_MSC2 | PNP_MSC3 | PNP_MSC4 |
				 PNP_MSC5,
	  0x0ff8, },
	{ &ops, SCH5545_LDN_LPC_IF, PNP_IO0, 0x0ffe },
	{ &ops, SCH5545_LDN_PP, PNP_IO0 | PNP_IRQ0 | PNP_DRQ0 | PNP_MSC0 |
				PNP_MSC1, 0x0ff8 },
};

static void enable_dev(struct device *dev)
{
	pnp_enable_devices(dev, &ops, ARRAY_SIZE(pnp_dev_info),
			       pnp_dev_info);
}

struct chip_operations superio_smsc_sch5545_ops = {
	CHIP_NAME("SMSC SCH5545 Super I/O")
	.enable_dev = enable_dev,
};

