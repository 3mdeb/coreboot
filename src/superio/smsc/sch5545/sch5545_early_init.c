/*
 * This file is part of the coreboot project.
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
#include <device/pnp_def.h>
#include <device/pnp_ops.h>
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
static void set_iobase(pnp_devfn_t dev, uint16_t device_addr, uint16_t bar_addr)
{
	uint16_t bar;

	/* set BAR
	 * we have to flip the BAR due to different register layout:
	 * - LPC addr LSB on device_addr + 2
	 * - LPC addr MSB on device_addr + 3
	 */
	bar = ((bar_addr >> 8) & 0xff) | ((bar_addr & 0xff) << 8);
	pnp_set_iobase(dev, device_addr + 2, bar);
}

/*
 * set the IRQ for the specific device
 * pnp_devfn_t dev must be in conf state
 * LDN LPC IF must be active.
 */
static void set_irq(pnp_devfn_t dev, uint8_t irq_device, unsigned irq)
{
	if (irq > 15)
		return;

	pnp_write_config(dev, SCH5545_IRQ_BASE + irq, irq_device);
}

/* sch5545 has 2 LEDs which are accessed via color (1 bit),
 * 2 bits for a pattern blink
 * 1 bit for "code fetch" which means the cpu/mainboard is working (always set)
 */
void sch5545_set_led(unsigned runtime_reg_base, unsigned color, uint16_t blink)
{
	uint8_t val = blink & SCH5545_LED_BLINK_MASK;
	val |= SCH5545_LED_CODE_FETCH;
	if (color)
		val |= SCH5545_LED_COLOR_GREEN;
	outb(val, runtime_reg_base + SCH5545_RR_LED);
}

void sch5545_early_init(unsigned port)
{
	pnp_devfn_t dev;

	/* Enable SERIRQ */
	dev = PNP_DEV(port, SCH5545_LDN_GCONF);
	pnp_enter_conf_state(dev);
	pnp_set_logical_device(dev);
	pnp_write_config(dev, 0x24, pnp_read_config(dev, 0x24) | 0x04);


	/* enable lpc if */
	dev = PNP_DEV(port, SCH5545_LDN_LPC_IF);
	pnp_set_logical_device(dev);
	pnp_set_enable(dev, 1);
	/* Set LPC BAR mask */
	pnp_write_config(dev, SCH5545_BAR_LPC_IF, 0x01);
	/* BAR valid, Frame/LDN = 0xc */
	pnp_write_config(dev, SCH5545_BAR_LPC_IF + 1,
			 SCH5545_LDN_LPC_IF | 0x80);
	set_iobase(dev, SCH5545_BAR_LPC_IF, port);

	/* Enable runtime registers */

	/* The Runtime Registers BAR is 0x40 long */
	pnp_write_config(dev, SCH5545_BAR_RUNTIME_REG, 0x3f);
	/* BAR valid, Frame/LDN = 0xa */
	pnp_write_config(dev, SCH5545_BAR_RUNTIME_REG + 1,
			 SCH5545_LDN_RR | 0x80);

	/* map runtime registers */
	set_iobase(dev, SCH5545_BAR_RUNTIME_REG, SCH5545_RUNTIME_REG_BASE);
	dev = PNP_DEV(port, SCH5545_LDN_RR);
	pnp_set_logical_device(dev);
	pnp_set_enable(dev, 1);

	/* Clear pending adn enable PMEs */
	outb(0x01, SCH5545_RUNTIME_REG_BASE);
	outb(0x01, SCH5545_RUNTIME_REG_BASE + 1);

	/* set LED color and indicate BIOS has reached code fetch phase */
	sch5545_set_led(SCH5545_RUNTIME_REG_BASE, SCH5545_LED_COLOR_GREEN,
			SCH5545_LED_BLINK_ON);

	/* Configure EMI */
	dev = PNP_DEV(port, SCH5545_LDN_LPC_IF);
	pnp_set_logical_device(dev);
	/* EMI BAR has 11 registers, but vendor sets the mask to 0xf */
	pnp_write_config(dev, SCH5545_BAR_EM_IF, 0x0f);
	/* BAR valid, Frame/LDN = 0x00 */
	pnp_write_config(dev, SCH5545_BAR_EM_IF + 1, SCH5545_LDN_EMI | 0x80);
	set_iobase(dev, SCH5545_BAR_EM_IF, SCH5545_EMI_BASE);

	pnp_exit_conf_state(dev);
}

void sch5545_enable_uart(unsigned port, unsigned uart_no)
{
	pnp_devfn_t dev;

	if (uart_no > 1)
		return;

	/* configure serial 1 / UART 1 */
	dev = PNP_DEV(port, SCH5545_LDN_LPC_IF);
	pnp_set_logical_device(dev);
	/* Set UART BAR mask to 0x07 (8 registers) */
	pnp_write_config(dev, SCH5545_BAR_UART1 + (4 * uart_no), 0x07);
	/* Set BAR valid, Frame/LDN = UART1/2 LDN 0x07/0x08 */
	pnp_write_config(dev, SCH5545_BAR_UART1 + (4 * uart_no) + 1,
			 (SCH5545_LDN_UART1 + uart_no) | 0x80);
	set_iobase(dev, SCH5545_BAR_UART1 + (4 * uart_no),
		   (uart_no == 1) ? 0x2f8 : 0x3f8);
	/* IRQ 3 for UART2, IRQ4 for UART1 */
	set_irq(dev, SCH5545_LDN_UART1 + uart_no, 4 - uart_no);

	dev = PNP_DEV(port, SCH5545_LDN_UART1 + uart_no);
	pnp_set_logical_device(dev);
	pnp_set_enable(dev, 1);
	pnp_write_config(dev, SCH5545_UART_CONFIG_SELECT,
			 SCH5545_UART_POWER_VCC);

	pnp_exit_conf_state(dev);
}
