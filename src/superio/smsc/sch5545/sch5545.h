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

#ifndef SUPERIO_SCH_5545_H
#define SUPERIO_SCH_5545_H

/* LPC i/o space */
#define SCH5545_RUNTIME_REG_BASE	0x0a00
#define SCH5545_EMI_BASE 		0x0a40

/* runtime register */
#define SCH5545_RR_LED		0x25
/* bit 0-1 blink */
#define  SCH5545_LED_BLINK_OFF	0x0
#define  SCH5545_LED_BLINK_1HZ	0x1
#define  SCH5545_LED_BLINK_ON	0x3
#define  SCH5545_LED_BLINK_MASK	0x3
/* bit 2 color */
#define  SCH5545_LED_COLOR_YELLOW 0x0
#define  SCH5545_LED_COLOR_GREEN 0x4
#define  SCH5545_LED_CODE_FETCH  0x8

/* logical devices */
#define SCH5545_LDN_EMI		0x00
#define SCH5545_LDN_KBC		0x01
#define SCH5545_LDN_UART1	0x07
#define SCH5545_LDN_UART2	0x08
#define SCH5545_LDN_RR		0x0a	/* Runtime Registers */
#define SCH5545_LDN_FDC		0x0b
#define SCH5545_LDN_LPC_IF	0x0c	/* LPC Interface */
#define SCH5545_LDN_PP		0x11

/* UART config registers */
#define SCH5545_CONFIG_SELECT		0xf0

/* UART config select */
#define SCH5545_UART_POWER_VCC		0x02
#define SCH5545_UART_INVERT_POLARITY	0x04

/* LPC IF config registers */
#define SCH5545_IRQ_BASE		0x40
#define SCH5545_DRQ_BASE		0x50
/*
 * BAR registers are 4 byte
 * byte 0 0-6 mask, 7 reserved
 * byte 1 0-5 frame, 6 device, 7 valid
 * byte 2 LPC address least sig.
 * byte 3 LPC address most sig.
 */
#define SCH5545_BAR_LPC_IF	0x60
#define SCH5545_BAR_EM_IF	0x64
#define SCH5545_BAR_UART1	0x68
#define SCH5545_BAR_UART2	0x6c
#define SCH5545_BAR_RUNTIME_REG	0x70
/* missing ? */
#define SCH5545_BAR_KBC		0x78
#define SCH5545_BAR_FLOPPY	0x7c
#define SCH5545_BAR_PARPORT	0x80

/* IRQ <> device mappings */
#define SCH5545_IRQ_KBD			0x01
#define SCH5545_IRQ_MOUSE		0x81
#define SCH5545_IRQ_UART1		0x07
#define SCH5545_IRQ_UART2		0x08
#define SCH5545_IRQ_EMI_MAILBOX		0x00
#define SCH5545_IRQ_EMI_IRQ_SOURCE	0x80
#define SCH5545_IRQ_RUNTIME_REG		0x0a
#define SCH5545_IRQ_RUNTIME_REG_SMI	0x8a
#define SCH5545_IRQ_FLOPPY		0x0b
#define SCH5545_IRQ_PARPORT		0x11
#define SCH5545_IRQ_DISABLED		0xff

void sch5545_early_init(unsigned port);
void sch5545_set_led(unsigned runtime_reg_base, unsigned color, uint16_t blink);

#endif /* SUPERIO_SCH_5545_H */
