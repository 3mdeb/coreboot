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

#ifndef SUPERIO_SCH_5545_H
#define SUPERIO_SCH_5545_H

/* LPC I/O space */
#define SCH5545_RUNTIME_REG_BASE		0x0a00
#define SCH5545_EMI_BASE			0x0a40

/* logical devices */
#define SCH5545_LDN_EMI				0x00
#define SCH5545_LDN_KBC				0x01
#define SCH5545_LDN_UART1			0x07
#define SCH5545_LDN_UART2			0x08
#define SCH5545_LDN_RR				0x0a	/* Runtime Registers */
#define SCH5545_LDN_FDC				0x0b
#define SCH5545_LDN_LPC_IF			0x0c	/* LPC Interface */
#define SCH5545_LDN_PP				0x11
#define SCH5545_LDN_GCONF			0x3f	/* Global Config */

/* KBC config registers */
#define SCH5545_KRST_GA20			0xf0
#define   SCH5545_PORT92_EN			(1 << 2)
#define   SCH5545_KBD_INT_LATCH			(1 << 3)
#define   SCH5545_MOUSE_INT_LATCH		(1 << 4)
#define   SCH5545_KBD_ISOLATION			(1 << 5)
#define   SCH5545_MOUSE_ISOLATION		(1 << 6)
#define SCH5545_KEYBOARD_SELECT			0xf1
#define   SCH5545_KBD_MOUSE_SWAP		(1 << 0)
#define SCH5545_8042_RESET			0xf2
#define   SCH5545_8042_IN_RESET			(1 << 0)
#define   SCH5545_8042_OUT_RESET		(0 << 0)

/* UART config registers */
#define SCH5545_UART_CONFIG_SELECT		0xf0
#define   SCH5545_UART_CLK_64MHZ_RO		(0 << 0)
#define   SCH5545_UART_CLK_96MHZ_PLL		(1 << 0)
#define   SCH5545_UART_POWER_VTR		(0 << 1)
#define   SCH5545_UART_POWER_VCC		(1 << 1)
#define   SCH5545_UART_NO_POLARITY_INVERT	(0 << 2)
#define   SCH5545_UART_INVERT_POLARITY		(1 << 2)

/* RR config registers */
#define SCH5545_SPEKEY				0xf0
#define SCH5545_SPEKEY_WAKE_EN			(0 << 1)
#define SCH5545_SPEKEY_WAKE_DIS			(1 << 1)

/* Floppy config registers */
#define SCH5545_FDD_MODE			0xf0
#define   SCH5545_FDD_MODE_NORMAL		(0 << 0)
#define   SCH5545_FDD_MODE_ENHANCED		(1 << 0)
#define   SCH5545_FDC_DMA_MODE_BURST		(0 << 1)
#define   SCH5545_FDC_DMA_MODE_NON_BURST	(1 << 1)
#define   SCH5545_FDD_IF_MODE_AT		(3 << 2)
#define   SCH5545_FDD_IF_MODE_PS2		(1 << 2)
#define   SCH5545_FDD_IF_MODE_MODEL30		(0 << 2)
#define   SCH5545_FDC_OUTPUT_TYPE_OPEN_DRAIN	(0 << 6)
#define   SCH5545_FDC_OUTPUT_TYPE_PUSH_PULL	(1 << 6)
#define   SCH5545_FDC_OUTPUT_CTRL_ACTIVE	(0 << 7)
#define   SCH5545_FDC_OUTPUT_CTRL_TRISTATE	(1 << 7) 
#define SCH5545_FDD_OPTION			0xf1
#define   SCH5545_FDD_FORCED_WP_INACTIVE	(0 << 0)
#define   SCH5545_FDD_FORCED_WP_ACTIVE		(1 << 0)
#define   SCH5545_FDD_DENSITY_NORMAL		(0 << 2)
#define   SCH5545_FDD_DENSITY_NORMAL_USER	(1 << 2)
#define   SCH5545_FDD_DENSITY_LOGIC_ONE		(2 << 2)
#define   SCH5545_FDD_DENSITY_LOGIC_ZERO	(3 << 2)
#define SCH5545_FDD_TYPE			0xf2
#define SCH5545_FDD0				0xf4
#define   SCH5545_FDD0_TYPE_SEL_DT0		(1 << 0)
#define   SCH5545_FDD0_TYPE_SEL_DT1		(1 << 1)
#define   SCH5545_FDD0_DRT_SEL_DRT0		(1 << 3)
#define   SCH5545_FDD0_USE_PRECOMPENSATION	(0 << 6)
#define   SCH5545_FDD0_NO_PRECOMPENSATION	(1 << 6)

/* Parallel Port config registers */
#define SCH5545_PP_INT_SELECT			0x70
#define   SCH5545_PP_SERIRQ_CHANNEL_MASK	0xf
#define SCH5545_PP_DMA_SELECT			0x74
#define   SCH5545_PP_DMA_CHANNEL_MASK		0x7
#define SCH5545_PP_MODE				0xf0
#define   SCH5545_PP_MODE_PRINTER		(4 << 0)
#define   SCH5545_PP_MODE_SPP			(0 << 0)
#define   SCH5545_PP_MODE_EPP19_SPP		(1 << 0)
#define   SCH5545_PP_MODE_EPP17_SPP		(5 << 0)
#define   SCH5545_PP_MODE_ECP			(2 << 0)
#define   SCH5545_PP_MODE_EPP17_ECP		(3 << 0)
#define   SCH5545_PP_MODE_EPP19_ECP		(7 << 0)
#define   SCH5545_PP_ECP_FIFO_TRESH_MASK	(0xf << 3)
#define   SCH5545_PP_INT_PULSED_LOW		(1 << 7)
#define   SCH5545_PP_INT_FOLLOWS_ACK		(0 << 7)
#define SCH5545_PP_MODE2			0xf1
#define   SCH5545_PP_TMOUT_CLEARED_ON_WRITE	(0 << 4)
#define   SCH5545_PP_TMOUT_CLEARED_ON_READ	(1 << 4)

/* LPC IF config registers */
#define SCH5545_IRQ_BASE			0x40
#define SCH5545_DRQ_BASE			0x50
/*
 * BAR registers are 4 byte
 * byte 0 0-6 mask, 7 reserved
 * byte 1 0-5 frame, 6 device, 7 valid
 * byte 2 LPC address least sig.
 * byte 3 LPC address most sig.
 */
#define SCH5545_BAR_LPC_IF			0x60
#define SCH5545_BAR_EM_IF			0x64
#define SCH5545_BAR_UART1			0x68
#define SCH5545_BAR_UART2			0x6c
#define SCH5545_BAR_RUNTIME_REG			0x70
/* Certain SMSC parts have SPI controller LDN 	0xf with BAR rgeister at 0x74 */
#define SCH5545_BAR_KBC				0x78
#define SCH5545_BAR_FLOPPY			0x7c
#define SCH5545_BAR_PARPORT			0x80

/* IRQ <> device mappings */
#define SCH5545_IRQ_KBD				0x01
#define SCH5545_IRQ_MOUSE			0x81
#define SCH5545_IRQ_UART1			0x07
#define SCH5545_IRQ_UART2			0x08
#define SCH5545_IRQ_EMI_MAILBOX			0x00
#define SCH5545_IRQ_EMI_IRQ_SOURCE		0x80
#define SCH5545_IRQ_RUNTIME_REG			0x0a
#define SCH5545_IRQ_RUNTIME_REG_SMI		0x8a
#define SCH5545_IRQ_FLOPPY			0x0b
#define SCH5545_IRQ_PARPORT			0x11
#define SCH5545_IRQ_DISABLED			0xff


/* runtime registers */
#define SCH5545_RR_PME_STS			0x00
#define   SCH5545_GLOBAL_PME_STS		0x01
#define SCH5545_RR_PME_EN			0x01
#define   SCH5545_GLOBAL_PME_EN			0x01
#define SCH5545_RR_PME_STS1			0x02
#define   SCH5545_UART2_RI_PME_STS		0x2
#define   SCH5545_UART1_RI_PME_STS		0x4
#define   SCH5545_KBD_PME_STS			0x8
#define   SCH5545_MOUSE_PME_STS			0x10
#define   SCH5545_SPECIFIC_KEY_PME_STS		0x20
#define SCH5545_RR_PME_STS2			0x03
#define   SCH5545_IO_SMI_EVT_STS		0x1
#define   SCH5545_WDT_TIMEOUT_EVT_STS		0x2
#define   SCH5545_EM_EVT1_STS			0x4
#define   SCH5545_EM_EVT2_STS			0x8
#define   SCH5545_FW_EVT_STS			0x10
#define   SCH5545_BAT_LOW_STS			0x20
#define   SCH5545_INTRUDER_STS			0x40
#define SCH5545_RR_PME_STS3			0x04
#define   SCH5545_GPIO62_PME_STS		0x1
#define   SCH5545_GPIO54_PME_STS		0x2
#define   SCH5545_GPIO53_PME_STS		0x4
#define   SCH5545_GPIO35_PME_STS		0x8
#define   SCH5545_GPIO31_PME_STS		0x10
#define   SCH5545_GPIO25_PME_STS		0x20
#define   SCH5545_GPIO24_PME_STS		0x40
#define   SCH5545_GPIO21_PME_STS		0x80
#define SCH5545_RR_PME_EN1			0x05
#define   SCH5545_UART2_RI_PME_EN		0x2
#define   SCH5545_UART1_RI_PME_EN		0x4
#define   SCH5545_KBD_PME_EN			0x8
#define   SCH5545_MOUSE_PME_EN			0x10
#define   SCH5545_SPECIFIC_KEY_PME_EN		0x20
#define SCH5545_RR_PME_EN2			0x06
#define   SCH5545_IO_SMI_EVT_PME_EN		0x1
#define   SCH5545_WDT_EVT_PME_EN		0x2
#define   SCH5545_EM_EVT1_PME_EN		0x4
#define   SCH5545_EM_EVT2_PME_EN		0x8
#define   SCH5545_FW_EVT_PME_EN			0x10
#define   SCH5545_BAT_LOW_PME_EN		0x20
#define   SCH5545_INTRUDER_PME_EN		0x40
#define SCH5545_RR_PME_EN3			0x07
#define   SCH5545_GPIO62_PME_EN			0x1
#define   SCH5545_GPIO54_PME_EN			0x2
#define   SCH5545_GPIO53_PME_EN			0x4
#define   SCH5545_GPIO35_PME_EN			0x8
#define   SCH5545_GPIO31_PME_EN			0x10
#define   SCH5545_GPIO25_PME_EN			0x20
#define   SCH5545_GPIO24_PME_EN			0x40
#define   SCH5545_GPIO21_PME_EN			0x80
#define SCH5545_RR_SMI_STS			0x10
#define   SCH5545_SMI_GLOBAL_STS		0x1
#define SCH5545_RR_SMI_EN			0x11
#define   SCH5545_SMI_GLOBAL_EN			0x1
#define SCH5545_RR_SMI_STS1			0x12
#define   SCH5545_LOW_BAT_SMI_STS		0x1
#define   SCH5545_PAR_PORT_SMI_STS		0x2
#define   SCH5545_UART2_SMI_STS			0x4
#define   SCH5545_UART1_SMI_STS			0x8
#define   SCH5545_FLOPPY_SMI_STS		0x10
#define   SCH5545_EM_EVT1_SMI_STS		0x20
#define   SCH5545_EM_EVT2_SMI_STS		0x40
#define   SCH5545_FW_EVT_SMI_STS		0x80
#define SCH5545_RR_SMI_STS2			0x13
#define   SCH5545_MOUSE_SMI_STS			0x1
#define   SCH5545_KBD_SMI_STS			0x2
#define   SCH5545_WATCHDOG_EVT_SMI_STS		0x8
#define   SCH5545_INTRUSION_SMI_STS		0x10
#define SCH5545_RR_SMI_STS3			0x14
#define   SCH5545_GPIO62_SMI_STS		0x1
#define   SCH5545_GPIO54_SMI_STS		0x2
#define   SCH5545_GPIO53_SMI_STS		0x4
#define   SCH5545_GPIO35_SMI_STS		0x8
#define   SCH5545_GPIO31_SMI_STS		0x10
#define   SCH5545_GPIO25_SMI_STS		0x20
#define   SCH5545_GPIO24_SMI_STS		0x40
#define   SCH5545_GPIO21_SMI_STS		0x80
#define SCH5545_RR_SMI_EN1			0x15
#define   SCH5545_LOW_BAT_SMI_EN		0x1
#define   SCH5545_PAR_PORT_SMI_EN		0x2
#define   SCH5545_UART2_SMI_EN			0x4
#define   SCH5545_UART1_SMI_EN			0x8
#define   SCH5545_FLOPPY_SMI_EN			0x10
#define   SCH5545_EM_EVT1_SMI_EN		0x20
#define   SCH5545_EM_EVT2_SMI_EN		0x40
#define   SCH5545_FW_EVT_SMI_EN			0x1
#define SCH5545_RR_SMI_EN2			0x16
#define   SCH5545_MOUSE_SMI_EN			0x1
#define   SCH5545_KBD_SMI_EN			0x2
#define   SCH5545_WATCHDOG_EVT_SMI_EN		0x8
#define   SCH5545_INTRUSION_SMI_EN		0x10
#define SCH5545_RR_SMI_EN3			0x17
#define   SCH5545_GPIO62_SMI_EN			0x1
#define   SCH5545_GPIO54_SMI_EN			0x2
#define   SCH5545_GPIO53_SMI_EN			0x4
#define   SCH5545_GPIO35_SMI_EN			0x8
#define   SCH5545_GPIO31_SMI_EN			0x10
#define   SCH5545_GPIO25_SMI_EN			0x20
#define   SCH5545_GPIO24_SMI_EN			0x40
#define   SCH5545_GPIO21_SMI_EN			0x80
#define SCH5545_RR_FORCE_DISK_CH		0x20
#define   SCH5545_FLOPPY_DISK_CHANGE		0x1
#define SCH5545_RR_FLOPPY_DR_SEL		0x21
#define   SCH5545_DR_SELECT0			0x1
#define   SCH5545_DR_SELECT1			0x2
#define   SCH5545_FLOPPY_PRECOMP0		0x4
#define   SCH5545_FLOPPY_PRECOMP1		0x8
#define   SCH5545_FLOPPY_PRECOMP2		0x10
#define   SCH5545_FLOPPY_PWR_DOWN		0x40
#define   SCH5545_FLOPPY_SOFT_RESET		0x80
#define SCH5545_RR_UART1_FIFO_CTRL		0x22
#define   SCH5545_UART1_FIFO_FE			0x1
#define   SCH5545_UART1_FIFO_RFR		0x2
#define   SCH5545_UART1_FIFO_XFR		0x4
#define   SCH5545_UART1_FIFO_DMS		0x8
#define   SCH5545_UART1_FIFO_RTL		0x40
#define   SCH5545_UART1_FIFO_RTM		0x80
#define SCH5545_RR_UART2_FIFO_CTRL		0x23
#define   SCH5545_UART2_FIFO_FE			0x1
#define   SCH5545_UART2_FIFO_RFR		0x2
#define   SCH5545_UART2_FIFO_XFR		0x4
#define   SCH5545_UART2_FIFO_DMS		0x8
#define   SCH5545_UART2_FIFO_RTL		0x40
#define   SCH5545_UART2_FIFO_RTM		0x80
#define SCH5545_RR_DEV_DISABLE			0x24
#define   SCH5545_FLOPPY_WP			0x1
#define   SCH5545_FLOPPY_DIS			0x8
#define   SCH5545_UART2_DIS			0x20
#define   SCH5545_UART1_DIS			0x40
#define   SCH5545_PAR_PORT_DIS			0x80
#define SCH5545_RR_LED				0x25
#define  SCH5545_LED_BLINK_OFF			0x0
#define  SCH5545_LED_BLINK_1HZ			0x1
#define  SCH5545_LED_BLINK_ON			0x3
#define  SCH5545_LED_BLINK_MASK			0x3
#define  SCH5545_LED_COLOR_YELLOW		0x0
#define  SCH5545_LED_COLOR_GREEN		0x4
#define  SCH5545_LED_CODE_FETCH 		0x8
#define SCH5545_RR_KB_SCAN			0x26
#define SCH5545_RR_PWRGOOD			0x27
#define   SCH5545_PWRGOOD_DELAY			0x1
#define   SCH5545_PWRGOOD_LOCK			0x2
#define   SCH5545_PCIRST_OUT4_EN		0x10
#define   SCH5545_PCIRST_OUT3_EN		0x20
#define   SCH5545_PCIRST_OUT1_EN		0x40
#define   SCH5545_PCIRST_OUT2_EN		0x80	
#define SCH5545_RR_GPIO_SEL			0x28
#define SCH5545_RR_GPIO_READ			0x29
#define SCH5545_RR_GPIO_WRITE			0x2A
#define SCH5545_RR_FW_EVT_STS			0x30
#define SCH5545_RR_FW_EVT_EN			0x31
#define SCH5545_RR_PWR_REC_MODES		0x32
#define   SCH5545_PWR_SUPPLY_OFF		0x00
#define   SCH5545_PWR_SUPPLY_ON			0x80
#define SCH5545_RR_INTRUDER			0x34
#define   SCH5545_INTRUSION_EDGE_STS		0x1
#define   SCH5545_INTRUDER_PIN_STS		0x2

void sch5545_early_init(unsigned port);
void sch5545_enable_uart(unsigned port, unsigned uart_no);
void sch5545_set_led(unsigned runtime_reg_base, unsigned color, uint16_t blink);

#endif /* SUPERIO_SCH_5545_H */
