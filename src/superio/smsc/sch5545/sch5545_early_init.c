/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Alexander Couzens <lynxis@fe80.eu>
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


#include <arch/io.h>
#include <device/pnp.h>
#include <stdint.h>

#include "sch5545.h"

static void pnp_enter_conf_state(pnp_devfn_t dev)
{
	unsigned port = dev >> 8;
	outb(0x55, port);
}

static void pnp_exit_conf_state(pnp_devfn_t dev)
{
	unsigned port = dev >> 8;
	outb(0xaa, port);
}

/*
 * Set the BAR / iobase for a specific device.
 * pnp_devfn_t dev must be in conf state.
 * LDN LPC IF must be active.
 */
static void sch5545_set_iobase(pnp_devfn_t dev, uint16_t device_addr, uint32_t bar_addr)
{
	uint8_t val;
	unsigned port = dev >> 8;

	pnp_devfn_t lpcif = PNP_DEV(port, SCH5545_LDN_LPC_IF);
	pnp_set_logical_device(lpcif);

	/* set BAR */
	pnp_set_iobase(dev, device_addr + 2, bar_addr);

	/* set valid */
	val = pnp_read_config(dev, device_addr + 1);
	val |= 0x80;
	pnp_write_config(dev, device_addr + 1, val);

	pnp_set_logical_device(dev);
}

/*
 * set the IRQ for the specific device
 * pnp_devfn_t dev must be in conf state
 * LDN LPC IF must be active.
 */
static void sch5545_set_irq(pnp_devfn_t dev, uint8_t irq_device, unsigned irq)
{
	unsigned port = dev >> 8;
	if (irq > 15)
		return;

	pnp_devfn_t lpcif = PNP_DEV(port, SCH5545_LDN_LPC_IF);
	pnp_set_logical_device(lpcif);
	pnp_write_config(dev, SCH5545_IRQ_BASE + irq, irq_device);
	pnp_set_logical_device(dev);
}

/* sch5545 has 2 LEDs which are accessed via color (1 bit),
 * 2 bits for a pattern blink
 * 1 bit for "code fetch" which means the cpu/mainboard is working (always set)
 */
void sch5545_set_led(unsigned runtime_reg_base, unsigned color, uint16_t blink)
{
	uint8_t val = blink && SCH5545_LED_BLINK_MASK;
	val |= SCH5545_LED_CODE_FETCH;
	if (color)
		val |= SCH5545_LED_COLOR_GREEN;
	outb(val, runtime_reg_base + SCH5545_RR_LED);
}

void sch5545_early_init(unsigned port)
{
	pnp_devfn_t dev;


	/* enable lpc if */
	dev = PNP_DEV(port, SCH5545_LDN_LPC_IF);
	pnp_enter_conf_state(dev);

	pnp_set_logical_device(dev);
	pnp_set_enable(dev, 1);

	/* map runtime register */
	sch5545_set_iobase(dev,
			SCH5545_BAR_RUNTIME_REG,
			SCH5545_RUNTIME_REG_BASE);

	/* configure serial 1 / UART 1 */
	dev = PNP_DEV(port, SCH5545_LDN_UART1);
	pnp_set_logical_device(dev);
	pnp_set_enable(dev, 1);
	pnp_write_config(dev, SCH5545_CONFIG_SELECT, SCH5545_UART_POWER_VCC);
	sch5545_set_iobase(dev, SCH5545_BAR_UART1, CONFIG_TTYS0_BASE);
	sch5545_set_irq(dev, SCH5545_IRQ_UART1, 4);

	/* set SCH5545_LED_CODE_FETCH + blink orange = 1hz */
	sch5545_set_led(SCH5545_RUNTIME_REG_BASE, SCH5545_LED_COLOR_YELLOW, SCH5545_LED_BLINK_1HZ);

	pnp_exit_conf_state(dev);
}
