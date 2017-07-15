/*
 * This file is part of the coreboot project.
 *
 * Copyright(C) 2013 Google Inc.
 * Copyright (C) 2015 Intel Corp.
 * Copyright (C) 2017 Andreas Galauner <andreas@galauner.de>
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

#include "irqroute.h"
#include <soc/gpio.h>
#include <stdlib.h>
#include <boardid.h>
#include "onboard.h"
#include "gpio.h"

//Clocks:
//	1.		Unused
//	2.		Unused
//	3.		Unused
//	4.		Unused
//	5.		Unused

//PWM (Hardware not present, could be used as GPIO):
//	0		Unconnected
//	1		Unconnected

//SDMMC1:	eMMC?
//SDMMC2:	Not present
//SDMMC3:	SD-Card on headers
//FIXME: Check if pullups are necessary

//LPC:		All unconnected
//	AD3 (GPIO):		MCU_RESET
//	AD1 (GPIO):		MCU_BOOT0

//GPIO_DFX	Memory config
//	DFX6
//	DFX7
//	DFX8

/* South East Community */
static const struct soc_gpio_map gpse_gpio_map[] = {
	// /*
	//   FED9C400: 80 03 11 00 00 00 C0 05-00 03 11 00 00 00 C0 05  *................*
	//   FED9C410: 80 03 11 00 00 00 C0 05-00 03 13 00 00 00 C0 05  *................*
	//   FED9C420: 80 03 11 00 00 00 C0 05-00 03 11 00 00 00 C0 05  *................*
	//   FED9C430: 00 03 13 00 00 00 C0 05-80 03 11 00 00 00 C0 05  *................*
	// */
	// GPIO_NC,																			/* 00 MF_PLT_CLK0 */	/*  */
	// GPIO_NC,																			/* 01 PWM1 */			/*  */
	// GPIO_NC,																			/* 02 MF_PLT_CLK1 */	/*  */
	// GPIO_NC,																			/* 03 MF_PLT_CLK4 */	/*  */
	// GPIO_NC,																			/* 04 MF_PLT_CLK3 */	/*  */
	// GPIO_NC,																			/* 05 PWM0*/			/*  */
	// GPIO_NC,																			/* 06 MF_PLT_CLK5 */	/*  */
	// GPIO_NC,																			/* 07 MF_PLT_CLK2 */	/*  */

	/*SE00 */ { .pad_conf0 = 0x00110380, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE01 */ GPIO_SKIP,
	/*SE02 */ { .pad_conf0 = 0x00110380, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE03 */ { .pad_conf0 = 0x00130300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE04 */ { .pad_conf0 = 0x00110380, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE05 */ GPIO_SKIP,
	/*SE06 */ { .pad_conf0 = 0x00130300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE07 */ { .pad_conf0 = 0x00110380, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },


	// /*
	//   FED9C800: 81 03 91 00 00 00 C0 05-81 03 11 00 00 00 C0 05  *................*
	//   FED9C810: 81 03 91 00 00 00 C0 05-81 03 91 00 00 00 C0 05  *................*
	//   FED9C820: 80 03 11 00 00 00 C0 05-81 03 91 00 00 00 C0 05  *................*
	//   FED9C830: 81 03 91 00 00 00 C0 05-81 03 91 00 00 00 C0 05  *................*
	//   FED9C840: 81 03 91 00 00 00 C0 05-81 03 91 00 00 00 C0 05  *................*
	//   FED9C850: 81 03 91 00 00 00 C0 05-81 03 91 00 00 00 C0 05  *................*
	// */
	// GPIO_NC,																			/* 15 SDMMC2_D3_CD_B */	/*  */
	// Native_M1,																			/* 16 SDMMC1_CLK */		/* SDMMC1_CLK */
	// NATIVE_PU20K(1),																	/* 17 SDMMC1_D0 */		/* SDMMC1_D_0 */
	// GPIO_NC,																			/* 18 SDMMC2_D1 */		/*  */
	// GPIO_NC,																			/* 19 SDMMC2_CLK */		/*  */
	// NATIVE_PU20K(1),																	/* 20 SDMMC1_D2 */		/* SDMMC1_D_2 */
	// GPIO_NC,																			/* 21 SDMMC2_D2 */		/*  */
	// GPIO_NC,																			/* 22 SDMMC2_CMD  */	/*  */
	// NATIVE_PU20K(1),																	/* 23 SDMMC1_CMD */		/* SDMMC1_CMD */
	// NATIVE_PU20K(1),																	/* 24 SDMMC1_D1 */		/* SDMMC1_D_1 */
	// GPIO_NC,																			/* 25 SDMMC2_D0 */		/*  */
	// NATIVE_PU20K(1),																	/* 26 SDMMC1_D3_CD_B */	/* SDMMC1_D_3_CD_B */

	/*SE15 */ GPIO_SKIP,
	/*SE16 */ { .pad_conf0 = 0x00110380, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE17 */ { .pad_conf0 = 0x00910381, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE18 */ GPIO_SKIP,
	/*SE19 */ GPIO_SKIP,
	/*SE20 */ { .pad_conf0 = 0x00910381, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE21 */ GPIO_SKIP,
	/*SE22 */ GPIO_SKIP,
	/*SE23 */ { .pad_conf0 = 0x00910381, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE24 */ { .pad_conf0 = 0x00910381, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE25 */ GPIO_SKIP,
	/*SE26 */ { .pad_conf0 = 0x00910381, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },


	///*
	//  FED9CC00: 81 03 91 00 00 00 C0 05-80 03 11 00 00 00 C0 05  *................*
	//  FED9CC10: 81 03 91 00 00 00 C0 05-81 03 91 00 00 00 C0 05  *................*
	//  FED9CC20: 81 03 91 00 00 00 C0 05-81 03 91 00 00 00 C0 05  *................*
	//*/
	//NATIVE_PU20K(1),																	/* 30 SDMMC3_D1 */		/* SDMMC3_D1 */
	//Native_M1,																			/* 31 SDMMC3_CLK */		/* SDMMC3_CLK */
	//NATIVE_PU20K(1),																	/* 32 SDMMC3_D3 */		/* SDMMC3_D3 */
	//NATIVE_PU20K(1),																	/* 33 SDMMC3_D2 */		/* SDMMC3_D2 */
	//NATIVE_PU20K(1),																	/* 34 SDMMC3_CMD */		/* SDMMC3_CMD */
	//NATIVE_PU20K(1),																	/* 35 SDMMC3_D0 */		/* SDMMC3_D0 */
	
	/*SE30 */ { .pad_conf0 = 0x00910381, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE31 */ { .pad_conf0 = 0x00110380, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE32 */ { .pad_conf0 = 0x00910381, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE33 */ { .pad_conf0 = 0x00910381, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE34 */ { .pad_conf0 = 0x00910381, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE35 */ { .pad_conf0 = 0x00910381, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },



	// /*
	//   FED9D000: 01 03 91 00 00 00 C0 05-01 03 91 00 00 00 C0 05  *................*
	//   FED9D010: 01 03 91 00 00 00 C0 05-01 03 91 00 00 00 C0 05  *................*
	//   FED9D020: 00 03 11 00 00 00 C0 05-02 81 91 00 00 00 C0 05  *................*
	//   FED9D030: 00 03 11 00 00 00 C0 05-02 81 91 00 00 00 C0 05  *................*
	// */
	// GPIO_NC,																			/* 45 MF_LPC_AD2 */		/*  */
	// GPIO_NC,																			/* 46 LPC_CLKRUNB */	/*  */
	// GPIO_NC,																			/* 47 MF_LPC_AD0 */		/*  */
	// GPIO_NC,																			/* 48 LPC_FRAMEB */		/*  */
	// GPIO_NC,																			/* 49 MF_LPC_CLKOUT1 */	/*  */
	// GPIO_OUT_HIGH,																		/* 50 MF_LPC_AD3 */		/* MCU_RESET */
	// GPIO_NC,																			/* 51 MF_LPC_CLKOUT0 */	/*  */
	// GPIO_OUT_HIGH,																		/* 52 MF_LPC_AD1 */		/* MCU_BOOT0 */

	/*SE45 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE46 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE47 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE48 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE49 */ GPIO_SKIP,
	/*SE50 */ { .pad_conf0 = 0x00918102, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE51 */ { .pad_conf0 = 0x00110300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE52 */ { .pad_conf0 = 0x00918102, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },


	// /*
	//   FED9D400: 02 82 91 58 19 00 C0 05-02 81 91 00 00 00 C0 05  *...X............*
	//   FED9D410: 01 82 91 4C 01 00 C0 05-81 03 91 00 00 00 C0 05  *...L............*
	//   FED9D420: 02 81 91 00 00 00 C0 05-81 03 91 00 00 00 C0 05  *................*
	//   FED9D430: 02 81 91 00 00 00 C0 05-81 03 91 00 00 00 C0 05  *................*
	//   FED9D440: 81 03 91 00 00 00 C0 05-80 03 11 00 00 00 C0 05  *................*
	// */
	// GPI(trig_edge_low, L5, P_20K_H, non_maskable, en_rx_data, UNMASK_WAKE , NA),		/* 60 SPI1_MISO */		/* PMIC_IRQ_1P8 */ 				/* Open drain? */	/* FIXME: InvRX_Enable - is this connected to the PMIC? */
	// GPIO_NC,																			/* 61 SPI1_CS0_B */		/*  */
	// GPIO_NC,																			/* 62 SPI1_CLK */		/*  */
	// NATIVE_PU20K(1),																	/* 63 MMC1_D6 */		/* MMC1_D6 */
	// GPIO_NC,																			/* 64 SPI1_MOSI */		/*  */
	// NATIVE_PU20K(1),																	/* 65 MMC1_D5 */		/* MMC1_D5 */
	// GPIO_OUT_HIGH,																		/* 66 SPI1_CS1_B */		/* LAN P0 ISOLATE */			/* FIXME: check if this pin is actually used as P0 Isolate */	//FIXME: Find P1 isolate
	// NATIVE_PU20K(1),																	/* 67 MMC1_D4_SD_WE */	/* MMC1_D4 */
	// NATIVE_PU20K(1),																	/* 68 MMC1_D7 */		/* MMC1_D7 */
	// GPIO_NC,																			/* 69 MMC1_RCLK */		/*  */

	/*SE60 */ { .pad_conf0 = 0x58918200, .pad_conf1 = 0x05C00019, .wake_mask = 1, .int_mask = 1, .gpe = 0 },
	/*SE61 */ { .pad_conf0 = 0x00918102, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE62 */ { .pad_conf0 = 0x4C918201, .pad_conf1 = 0x05C00001, .wake_mask = 0, .int_mask = 1, .gpe = 0 },
	/*SE63 */ { .pad_conf0 = 0x00910381, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE64 */ { .pad_conf0 = 0x00918102, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE65 */ { .pad_conf0 = 0x00910381, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE66 */ { .pad_conf0 = 0x00918102, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE67 */ { .pad_conf0 = 0x00910381, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE68 */ { .pad_conf0 = 0x00910381, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE69 */ GPIO_SKIP,


	// /*
	//   FED9D800: 01 03 91 00 00 00 C0 05-01 03 91 00 00 00 C0 05  *................*
	//   FED9D810: 01 82 91 14 01 00 C0 05-00 03 11 00 00 00 C0 05  *................*
	//   FED9D820: 01 03 91 00 00 00 C0 05-01 03 91 00 00 00 C0 05  *................*
	//   FED9D830: 00 02 91 10 04 00 C0 05-00 03 91 00 00 00 C0 05  *................*
	//   FED9D840: 00 03 11 00 00 00 C0 05-00 03 11 00 00 00 C0 05  *................*
	//   FED9D850: 00 03 11 00 00 00 C0 05-FF FF FF FF FF FF FF FF  *................*
	// */
	// Native_M1,																			/* 75 USB_OC1_B */			/* USB_OC1_B */				/* FIXME: pulls? */
	// Native_M1,																			/* 76 PMU_RESETBUTTON_B */	/* PMU_RESETBUTTON_B */		/* FIXME: pulls? */
	// GPIO_NC,																			/* 77 GPIO_ALERT   */		/*  */
	// Native_M1,																			/* 78 SDMMC3_PWR_EN_B */	/* SD_CARD_PWRDN_N */		/* FIXME: pulls? */	//FIXME: Connected?
	// GPIO_NC,																			/* 79 ILB_SERIRQ */			/*  */
	// Native_M1,																			/* 80 USB_OC0_B */			/* USB_OC0_B */				/* FIXME: pulls? */
	// NATIVE_INT_PU20K(1, L1),															/* 81 SDMMC3_CD_B */		/* SD_CARD_DET_N */			/* FIXME: Native M1, no pull, trig_level, invert */
	// SPEAKER,																			/* 82 SPKR */				/* SPKR */
	// GPIO_NC,																			/* 83 SUSPWRDNACK */		/*  */
	// Native_M1,																			/* 84 SDMMC1_RCLK */		/* SDMMC1_RCLK */			/* FIXME: no SPARE_PIN!? check if NC */
	// Native_M1,																			/* 85 SDMMC3_1P8_EN */		/* SD_CARD_PWR_EN */		/* FIXME: check if NC */

	/*SE75 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE76 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE77 */ { .pad_conf0 = 0x14918201, .pad_conf1 = 0x05C00001, .wake_mask = 1, .int_mask = 1, .gpe = 0 },
	/*SE78 */ { .pad_conf0 = 0x00110301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE79 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE80 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE81 */ { .pad_conf0 = 0x10910300, .pad_conf1 = 0x05C00004, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE82 */ GPIO_SKIP,
	/*SE83 */ { .pad_conf0 = 0x00110300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE84 */ { .pad_conf0 = 0x00110380, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SE85 */ { .pad_conf0 = 0x00110300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },


	GPIO_END
};

/* South West Community */
static const struct soc_gpio_map  gpsw_gpio_map[] = {
	// /*
	//   FED84400: 01 03 91 00 00 00 C0 05-00 03 91 00 00 00 C0 05  *................*
	//   FED84410: 00 03 91 00 00 00 C0 05-01 03 91 00 00 00 C0 05  *................*
	//   FED84420: 02 81 91 00 00 00 C0 05-01 03 91 00 00 00 C0 05  *................*
	//   FED84430: 01 03 91 00 00 00 C0 05-00 81 21 00 00 00 C0 05  *..........!.....*
	// */
	// GPIO_NC,																			/* 00 FST_SPI_D2 */			/*  */
	// Native_M1,																			/* 01 FST_SPI_D0 */			/* SPI_D0 */
	// Native_M1,																			/* 02 FST_SPI_CLK */		/* SPI_CLK */
	// GPIO_NC,																			/* 03 FST_SPI_D3 */			/*  */
	// GPIO_NC,																			/* 04 FST_SPI_CS1_B */		/*  */
	// Native_M1,																			/* 05 FST_SPI_D1 */			/* SPI_D1 */
	// Native_M1,																			/* 06 FST_SPI_CS0_B */		/* SPI_CS (SPI NOR on SOM) */
	// GPIO_NC,																			/* 07 FST_SPI_CS2_B */		/*  */

	/*SW00 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW01 */ { .pad_conf0 = 0x00910300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW02 */ { .pad_conf0 = 0x00910300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW03 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW04 */ { .pad_conf0 = 0x00918102, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW05 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW06 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW07 */ { .pad_conf0 = 0x00218100, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },


	// /*
	//   FED84800: 00 00 C0 04 00 00 C0 05-01 00 02 00 00 00 C0 04  *................*
	//   FED84810: 01 03 91 00 00 00 C0 05-00 00 01 00 00 00 C0 04  *................*
	//   FED84820: 01 03 91 00 00 00 C0 05-01 00 02 00 00 00 C0 04  *................*
	//   FED84830: 01 03 91 00 00 00 C0 05-01 03 91 00 00 00 C0 05  *................*
  	// */
	// Native_M1,																			/* 15 UART1_RTS_B */		/* UART1_RTS_B */
	// Native_M2,																			/* 16 UART1_RXD */			/* UART1_RXD */		//NOTE: set to UART0 for debugging
	// Native_M1,																			/* 17 UART2_RXD */			/* UART2_RXD */
	// Native_M1,																			/* 18 UART1_CTS_B */		/* UART1_CTS_B */
	// Native_M1,																			/* 19 UART2_RTS_B */		/* UART2_RTS_B */
	// Native_M2,																			/* 20 UART1_TXD */			/* UART1_TXD */		//NOTE: set to UART0 for debugging
	// Native_M1,																			/* 21 UART2_TXD */			/* UART2_TXD */
	// Native_M1,																			/* 22 UART2_CTS_B */		/* UART2_CTS_B */

	/*SW15 */ GPIO_SKIP, //{ .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW16 */ GPIO_SKIP, //{ .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW17 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW18 */ GPIO_SKIP,
	/*SW19 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW20 */ GPIO_SKIP,
	/*SW21 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW22 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },


	// /*
	//   FED84C00: 01 03 12 00 00 00 C0 05-01 03 12 00 00 00 C0 05  *................*
	//   FED84C10: 00 03 12 00 00 00 C0 05-00 03 12 00 00 00 C0 05  *................*
	//   FED84C20: 00 03 11 00 20 00 C0 05-00 03 12 00 00 00 C0 05  *.... ...........*
	//   FED84C30: 00 03 11 00 00 00 C0 05-00 03 11 00 00 00 C0 05  *................*
  	// */
	// Native_M2,																			/* 30 MF_HDA_CLK */					/* MF_HDA_CLK II GP_SSP_0_I2S_TXD */
	// Native_M2,																			/* 31 GPIO_SW31/MF_HDA_RSTB */		/* AUD_LINK_RST_N || I2S_0_CLK_R_AICO (AIC) */
	// Native_M2,																			/* 32 GPIO_SW32 /MF_HDA_SDI0 */		/* AUD_LINK_SDI0 (ALC282) || I2S_2_CLK_R_AICO (AIC) */
	// Native_M2,																			/* 33 MF_HDA_SDO */					/* AUD_LINK_SDO_R||I2S_0_RXD_R_AICO (AIC) */
	// GPIO_NC,																			/* 34 MF_HDA_DOCKRSTB */			/* I2S_2_TXD_R_AICO (AIC) */
	// Native_M2,																			/* 35 MF_HDA_SYNC */				/* AUD_LINKSYNC_R|| I2S_0_FS_R_AICO (AIC) */
	// GPIO_NC,																			/* 36 GPIO_SW36 MF_HDA_SDI1 */		/* I2S_2_FS_R_AICO */
	// GPIO_NC,																			/* 37 MF_HDA_DOCKENB */				/* I2S_2_RXD_R_AICO(AIC) */

	/*SW30 */ { .pad_conf0 = 0x00120300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW31 */ { .pad_conf0 = 0x00120300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW32 */ { .pad_conf0 = 0x00120300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW33 */ { .pad_conf0 = 0x00120300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW34 */ { .pad_conf0 = 0x00110300, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW35 */ { .pad_conf0 = 0x00120300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW36 */ { .pad_conf0 = 0x00110300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW37 */ { .pad_conf0 = 0x00110300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },

	// /*
	//   FED85000: 01 03 C1 00 20 00 C0 05-01 03 C1 00 20 00 C0 05  *.... ....... ...*
	//   FED85010: 01 03 C1 00 20 00 C0 05-01 03 C1 00 20 00 C0 05  *.... ....... ...*
	//   FED85020: 01 03 91 00 20 00 C0 05-01 03 C1 00 20 00 C0 05  *.... ....... ...*
	//   FED85030: 01 03 C1 00 20 00 C0 05-01 03 91 00 20 00 C0 05  *.... ....... ...*
	// */
	// GPIO_NC,																			/* 45 I2C5_SDA */					/* Touch Panel */
	// NATIVE_PU1K_CSEN_INVTX(1),															/* 46 I2C4_SDA */					/* I2C Audio | Touch PAD */
	// GPIO_NC,																			/* 47 I2C6_SDA */					/* INA Device */
	// GPIO_NC,																			/* 48 I2C5_SCL */					/* Touch Panel */
	// GPIO_NC,																			/* 49 I2C_NFC_SDA */				/* ?? */
	// NATIVE_PU1K_CSEN_INVTX(1),															/* 50 I2C4_SCL */					/* I2C Audio | Touch PAD */
	// GPIO_NC,																			/* 51 I2C6_SCL */					/* INA Decice */
	// GPIO_NC,																			/* 52 I2C_NFC_SCL */				/* ?? */

	/*SW45 */ { .pad_conf0 = 0x00C10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW46 */ { .pad_conf0 = 0x00C10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW47 */ { .pad_conf0 = 0x00C10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW48 */ { .pad_conf0 = 0x00C10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW49 */ GPIO_SKIP,
	/*SW50 */ { .pad_conf0 = 0x00C10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW51 */ { .pad_conf0 = 0x00C10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW52 */ GPIO_SKIP,

	// /*
	//   FED85400: 01 03 C1 00 20 00 C0 05-01 03 C1 00 20 00 C0 05  *.... ....... ...*
	//   FED85410: 01 03 C1 00 20 00 C0 05-01 03 C1 00 20 00 C0 05  *.... ....... ...*
	//   FED85420: 01 03 C1 00 20 00 C0 05-01 03 C1 00 20 00 C0 05  *.... ....... ...*
	//   FED85430: 01 03 C1 00 20 00 C0 05-01 03 C1 00 20 00 C0 05  *.... ....... ...*
	// */
	// NATIVE_PU1K_CSEN_INVTX(1),															/* 60 I2C1_SDA */					/* PMIC */
	// GPIO_NC,																			/* 61 I2C0_SDA */					/* 3rd party Sensor Card */
	// GPIO_NC,																			/* 62 I2C2_SDA */					/* MIPI_CSI CAMERAS, FLASH */
	// NATIVE_PU1K_CSEN_INVTX(1),															/* 63 I2C1_SCL */					/* PMIC */
	// GPIO_NC,																			/* 64 I2C3_SDA */					/* MIPI_CSI CAMERAS, FLASH */
	// GPIO_NC,																			/* 65 I2C0_SCL */					/* 3rd party Sensor Card */
	// GPIO_NC,																			/* 66 I2C2_SCL */					/* MIPI_CSI CAMERAS, FLASH */
	// GPIO_NC,																			/* 67 I2C3_SCL */					/* MIPI_CSI CAMERAS, FLASH */

	/*SW60 */ { .pad_conf0 = 0x00C10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW61 */ { .pad_conf0 = 0x00C10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW62 */ { .pad_conf0 = 0x00C10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW63 */ { .pad_conf0 = 0x00C10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW64 */ { .pad_conf0 = 0x00C10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW65 */ { .pad_conf0 = 0x00C10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW66 */ { .pad_conf0 = 0x00C10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW67 */ { .pad_conf0 = 0x00C10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },


	// /*
	//   FED85800: 00 80 11 00 00 00 C0 05-00 82 C1 0C 11 00 C0 05  *................*
	//   FED85810: 01 03 11 00 00 00 C0 05-00 03 11 00 00 00 C0 05  *................*
	//   FED85820: 01 03 91 00 00 00 C0 05-02 00 11 00 00 00 C0 05  *................*
	//   FED85830: 01 03 91 00 00 00 C0 05-01 03 91 00 00 00 C0 05  *................*
	// */
	// GPIO_NC,																			/* 75 SATA_GP0 */						/* TOUCH_PNL_RST_N */
	// GPIO_NC,																			/* 76 SATA_GP1 */						/* TOUCH_INT_N */
	// Native_M1,																			/* 77 SATA_LEDN */						/* SATA_LED_N */
	// Native_M1,																			/* 78 SATA_GP2 */						/* SATA_DEVSLP_R */
	// GPIO_NC,																			/* 79 MF_SMB_ALERTB */					/* MF_SMB_ALERTB */
	// Native_M1,																			/* 80 SATA_GP3 */						/* eMMC_RST_N */
	// GPIO_NC,																			/* 81 NFC_DEV_WAKE , MF_SMB_CLK */		/* MF_SMB_CLK */
	// GPIO_NC,																			/* 82 NFC_FW_DOWNLOAD, MF_SMB_DATA */	/* MF_SMB_DATA */

	/*SW75 */ { .pad_conf0 = 0x00118000, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW76 */ { .pad_conf0 = 0x0CC18200, .pad_conf1 = 0x05C00011, .wake_mask = 0, .int_mask = 1, .gpe = 0 },
	/*SW77 */ { .pad_conf0 = 0x00110301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW78 */ { .pad_conf0 = 0x00110300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW79 */ GPIO_SKIP,
	/*SW80 */ { .pad_conf0 = 0x00110002, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW81 */ GPIO_SKIP,
	/*SW82 */ GPIO_SKIP,


	// /*
	//   FED85C00: 01 03 91 00 00 00 C0 05-01 03 91 00 00 00 C0 05  *................*
	//   FED85C10: 00 03 11 00 00 00 C0 05-00 03 91 00 00 00 C0 05  *................*
	//   FED85C20: 00 03 11 00 00 00 C0 05-00 03 91 00 00 00 C0 05  *................*
	//   FED85C30: 00 03 11 00 00 00 C0 05-00 03 11 00 20 00 C0 05  *............ ...*
	// */
	// Native_M1,																			/* 90 PCIE_CLKREQ0B */				/* RTL8111G (CLKREQ_N) */
	// Native_M1,																			/* 91 PCIE_CLKREQ1B */				/* NGFF(CLKREQ_N) */
	// GPIO_NC,																			/* 92 GP_SSP_2_CLK */				/* ?? */
	// Native_M1,																			/* 93 PCIE_CLKREQ2B */				/* On uSOM NIC CLK Request */
	// GPIO_NC,																			/* 94 GP_SSP_2_RXD */				/* ?? */
	// Native_M1,																			/* 95 PCIE_CLKREQ3B */				/* Tied to Micro SD */				//FIXME: What does this pin do? Is it even a CLKREQ?
	// GPIO_NC,																			/* 96 GP_SSP_2_FS */				/* ?? */
	// GPIO_NC,																			/* 97 GP_SSP_2f_TXD */				/* ?? */

	/*SW90 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW91 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW92 */ GPIO_SKIP,
	/*SW93 */ { .pad_conf0 = 0x00910300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW94 */ GPIO_SKIP,
	/*SW95 */ { .pad_conf0 = 0x00910300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*SW96 */ GPIO_SKIP,
	/*SW97 */ GPIO_SKIP,


	GPIO_END
};


/* North Community */
static const struct soc_gpio_map  gpn_gpio_map[] = {
//	/*
//	  FED8C400: 02 81 11 00 00 00 C0 05-00 82 01 2C 21 00 C0 05  *...........,!...*
//	  FED8C410: 02 82 91 00 01 80 91 00-00 80 11 00 00 00 C0 05  *................*
//	  FED8C420: 02 81 91 00 00 00 C0 05-02 81 11 00 00 00 C0 05  *................*
//	  FED8C430: 03 82 91 00 01 80 91 00-00 81 11 00 00 00 C0 05  *................*
//	  FED8C440: 03 82 91 00 01 80 91 00-FF FF FF FF FF FF FF FF  *................*
//	*/
//	GPIO_OUT_HIGH,																		/* 00 GPIO_DFX0 */				/* LAN P0 RESET */						//FIXME: Is this a reset?
//	GPIO_NC,																			/* 01 GPIO_DFX3 */				/* ?? */
//	GPIO_INPUT_PU_20K,																	/* 02 GPIO_DFX7 */				/* RAMID */
//	GPIO_NC,																			/* 03 GPIO_DFX1 */				/* ?? */
//	GPIO_NC,																			/* 04 GPIO_DFX5 */				/* ?? */
//	GPIO_NC,																			/* 05 GPIO_DFX4 */				/* ?? */
//	GPIO_INPUT_PU_20K,																	/* 06 GPIO_DFX8 */				/* RAMID */
//	GPIO_NC,																			/* 07 GPIO_DFX2 */				/* ?? */
//	GPIO_INPUT_PU_20K,																	/* 08 GPIO_DFX6 */				/* RAMID */

	/*N00 */ { .pad_conf0 = 0x00118102, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N01 */ { .pad_conf0 = 0x2C018200, .pad_conf1 = 0x05C00021, .wake_mask = 0, .int_mask = 1, .gpe = 0 },
	/*N02 */ GPIO_SKIP,
	/*N03 */ { .pad_conf0 = 0x00118000, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N04 */ { .pad_conf0 = 0x00918102, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N05 */ { .pad_conf0 = 0x00118102, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N06 */ { .pad_conf0 = 0x00918203, .pad_conf1 = 0x00918001, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N07 */ { .pad_conf0 = 0x00918202, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N08 */ { .pad_conf0 = 0x00918203, .pad_conf1 = 0x00918001, .wake_mask = 0, .int_mask = 0, .gpe = 0 },

//	/*
//	  FED8C800: 00 82 01 3C 01 00 C0 05-00 82 11 00 00 00 C0 05  *...<............*
//	  FED8C810: 00 82 11 4C 01 00 C0 05-00 82 91 FC 01 00 C0 05  *...L............*
//	  FED8C820: 01 03 16 00 00 00 C0 05-00 02 91 00 00 00 C0 05  *................*
//	  FED8C830: 00 03 11 00 00 00 C0 05-01 03 96 00 00 00 C0 05  *................*
//	  FED8C840: 00 03 91 00 00 00 C0 05-02 81 91 00 00 00 C0 05  *................*
//	  FED8C850: 01 82 91 EC 01 00 C0 05-01 03 A1 00 00 00 C0 05  *................*
//	  FED8C860: 00 82 11 00 00 00 C0 05-FF FF FF FF FF FF FF FF  *................*
//	*/
//	GPIO_NC,																			/* 15 GPIO_SUS0 */				/* */
//	GPIO_NC,																			/* 16 SEC_GPIO_SUS10 */			/* */
//	GPIO_NC,																			/* 17 GPIO_SUS3 */				/*  */
//	GPI(trig_edge_low, L1, P_1K_H, non_maskable, NA, UNMASK_WAKE, NA),					/* 18 GPIO_SUS7 */				/* GPIO_SUS7 */
//	Native_M6,																			/* 19 GPIO_SUS1 */				/* GPIO_SUS1 */						//NOTE: configured as PCI_WAKE1_N, but is just a pin on the headers
//	Native_M1,																			/* 20 GPIO_SUS5 */				/* GPIO_SUS5 */						//FIXME: What's the native mode of this pin? docs only say strapping pin for Flash Descriptor Security Override
//	GPIO_NC,																			/* 21 SEC_GPIO_SUS11 */			/*  */
//	Native_M6,																			/* 22 GPIO_SUS4 */				/*  */								//Note: configired as PCI_WAKE4_N //FIXME: What's on this pin?
//	GPIO_NC,																			/* 23 SEC_GPIO_SUS8 */			/*  */
//	Native_M6,																			/* 24 GPIO_SUS2 */				/* GPIO_SUS2 */						//NOTE: configured as PCI_WAKE2_N, but is just a pin on the headers
//	GPIO_NC,																			/* 25 GPIO_SUS6 */				/*  */
//	Native_M1,																			/* 26 CX_PREQ_B */				/*  */
//	GPIO_NC,																			/* 27 SEC_GPIO_SUS9 */			/*  */

	
	/*N15 */ { .pad_conf0 = 0x3C018200, .pad_conf1 = 0x05C00001, .wake_mask = 1, .int_mask = 1, .gpe = SCI },
	/*N16 */ GPIO_SKIP,
	/*N17 */ { .pad_conf0 = 0x4C118200, .pad_conf1 = 0x05C00001, .wake_mask = 0, .int_mask = 1, .gpe = 0 },
	/*N18 */ { .pad_conf0 = 0xFC918200, .pad_conf1 = 0x05C00001, .wake_mask = 0, .int_mask = 1, .gpe = SMI },
	/*N19 */ { .pad_conf0 = 0x00160301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N20 */ { .pad_conf0 = 0x00910201, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N21 */ GPIO_SKIP,
	/*N22 */ { .pad_conf0 = 0x00960301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N23 */ GPIO_SKIP,
	/*N24 */ { .pad_conf0 = 0x00918102, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N25 */ { .pad_conf0 = 0xEC918201, .pad_conf1 = 0x05C00001, .wake_mask = 0, .int_mask = 1, .gpe = SCI },
	/*N26 */ GPIO_SKIP,
	/*N27 */ GPIO_SKIP,


//
//	/*
//	  FED8CC00: 01 03 A1 00 00 00 C0 05-00 03 21 00 00 00 C0 05  *..........!.....*
//	  FED8CC10: 01 03 01 00 60 00 C0 05-00 03 00 00 20 00 C0 05  *....`....... ...*
//	  FED8CC20: 01 03 A1 00 00 00 C0 05-01 03 A1 00 00 00 C0 05  *................*
//	  FED8CC30: 00 03 01 00 00 00 C0 05-01 03 A1 00 00 00 C0 05  *................*
//	  FED8CC40: 01 03 01 00 40 00 C0 05-00 03 01 00 00 00 C0 05  *....@...........*
//	  FED8CC50: 00 03 01 00 00 00 C0 05-01 03 A1 00 00 00 C0 05  *................*
//	*/
//	Native_M1,																			/* 30 TRST_B */					/*  */
//	Native_M1,																			/* 31 TCK */					/*  */
//	Native_M1,																			/* 32 PROCHOT_B */				/* PROCHOT_B */
//	GPIO_SKIP,																			/* 33 SVID0_DATA */				/*  */
//	Native_M1,																			/* 34 TMS */					/*  */
//	GPIO_NC,																			/* 35 CX_PRDY_B_2 */			/*  */
//	Native_M1,																			/* 36 TDO_2 */					/*  */
//	Native_M1,																			/* 37 CX_PRDY_B */				/*  */
//	GPIO_SKIP,																			/* 38 SVID0_ALERT_B */			/*  */
//	Native_M1,																			/* 39 TDO */					/*  */
//	GPIO_SKIP,																			/* 40 SVID0_CLK */				/*  */
//	Native_M1,																			/* 41 TDI */					/*  */

	/*N30 */ { .pad_conf0 = 0x00A10301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N31 */ { .pad_conf0 = 0x00210300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N32 */ { .pad_conf0 = 0x00010301, .pad_conf1 = 0x05C00060, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	GPIO_SKIP,
	/*N34 */ { .pad_conf0 = 0x00A10301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N35 */ { .pad_conf0 = 0x00A10301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N36 */ { .pad_conf0 = 0x00010300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N37 */ { .pad_conf0 = 0x00A10301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	GPIO_SKIP,
	/*N39 */ { .pad_conf0 = 0x00A10301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	GPIO_SKIP,
	/*N41 */ { .pad_conf0 = 0x00A10301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },


//
//	/*
//	  FED8D000: 00 80 11 00 00 00 C0 05-00 80 11 00 00 00 C0 05  *................*
//	  FED8D010: 00 80 11 00 00 00 C0 05-00 80 11 00 00 00 C0 05  *................*
//	  FED8D020: 00 80 11 00 00 00 C0 05-00 80 11 00 00 00 C0 05  *................*
//	  FED8D030: 00 80 11 00 00 00 C0 05-00 80 11 00 00 00 C0 05  *................*
//	  FED8D040: 02 81 11 00 00 00 C0 05-00 80 11 00 00 00 C0 05  *................*
//	  FED8D050: 00 80 11 00 00 00 C0 05-00 80 11 00 00 00 C0 05  *................*
//	*/
//	GPIO_NC,																			/* 45 GP_CAMERASB05 */			/*  */
//	GPIO_NC,																			/* 46 GP_CAMERASB02 */			/*  */
//	GPIO_NC,																			/* 47 GP_CAMERASB08 */			/*  */
//	GPIO_NC,																			/* 48 GP_CAMERASB00 */			/*  */
//	GPIO_NC,																			/* 49 GP_CAMERASBO6 */			/*  */
//	GPIO_NC,																			/* 50 GP_CAMERASB10 */			/*  */
//	GPIO_NC,																			/* 51 GP_CAMERASB03 */			/*  */
//	GPIO_NC,																			/* 52 GP_CAMERASB09 */			/*  */
//	GPIO_NC,																			/* 53 GP_CAMERASB01 */			/*  */
//	GPIO_NC,																			/* 54 GP_CAMERASB07 */			/*  */
//	GPIO_NC,																			/* 55 GP_CAMERASB11 */			/*  */
//	GPIO_NC,																			/* 56 GP_CAMERASB04 */			/*  */

	/*N45 */ { .pad_conf0 = 0x00118000, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N46 */ { .pad_conf0 = 0x00118000, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N47 */ { .pad_conf0 = 0x00118000, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N48 */ { .pad_conf0 = 0x00118000, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N49 */ { .pad_conf0 = 0x00118000, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N50 */ { .pad_conf0 = 0x00118000, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N51 */ { .pad_conf0 = 0x00118000, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N52 */ { .pad_conf0 = 0x00118000, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N53 */ { .pad_conf0 = 0x00118000, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N54 */ { .pad_conf0 = 0x00118000, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N55 */ { .pad_conf0 = 0x00118000, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N56 */ { .pad_conf0 = 0x00118000, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },


//
//	/*
//	  FED8D400: 00 03 01 00 00 00 C0 05-00 03 11 03 20 00 C0 05  *............ ...*
//	  FED8D410: 01 03 91 00 00 00 C0 04-00 03 01 00 00 00 C0 05  *................*
//	  FED8D420: 00 03 11 03 20 00 C0 05-00 03 01 00 00 00 C0 05  *.... ...........*
//	  FED8D430: 01 03 92 00 00 00 C0 05-01 03 91 00 00 00 C0 04  *................*
//	  FED8D440: 01 03 11 03 20 00 C0 05-00 03 01 00 00 00 C0 05  *.... ...........*
//	  FED8D450: 00 03 01 00 00 00 C0 05-01 03 92 00 00 00 C0 05  *................*
//	  FED8D460: 00 03 01 00 00 00 C0 05-FF FF FF FF FF FF FF FF  *................*
//	*/
//	GPIO_NC,																			/* 60 PANEL0_BKLTEN */			/*  */
//	GPIO_NC,																			/* 61 HV_DDI0_HPD */			/*  */
//	Native_M1,																			/* 62 HV_DDI2_DDC_SDA */		/* DDI2_SDA_1P8 */
//	Native_M1,																			/* 63 PANEL1_BKLTCTL */			/* PANEL1_BKLTCTL */
//	NATIVE_TX_RX_EN,																	/* 64 HV_DDI1_HPD */			/* DDI1_HPD */
//	GPIO_NC,																			/* 65 PANEL0_BKLTCTL */			/*  */
//	Native_M1,																			/* 66 HV_DDI0_DDC_SDA */		/* DDI0_1_SDA_1P8 */
//	Native_M1,																			/* 67 HV_DDI2_DDC_SCL */		/* DDI2_SCL_1P8 */
//	NATIVE_TX_RX_EN,																	/* 68 HV_DDI2_HPD */			/* DDI2_HPD */
//	Native_M1,																			/* 69 PANEL1_VDDEN */			/* PANEL1_VDDEN */
//	Native_M1,																			/* 70 PANEL1_BKLTEN */			/* PANEL1_BKLTEN */
//	GPIO_NC,																			/* 71 HV_DDI0_DDC_SCL */		/* DDI0_1_SCL_1P8 */
//	GPIO_NC,																			/* 72 PANEL0_VDDEN */			/*  */

	/*N60 */ { .pad_conf0 = 0x00010300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N61 */ { .pad_conf0 = 0x00110300, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N62 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x04C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N63 */ { .pad_conf0 = 0x00010300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N64 */ { .pad_conf0 = 0x00110301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N65 */ { .pad_conf0 = 0x00010300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N66 */ { .pad_conf0 = 0x00920301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N67 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x04C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N68 */ { .pad_conf0 = 0x00110301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N69 */ { .pad_conf0 = 0x00010300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N70 */ { .pad_conf0 = 0x00010300, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*N71 */ { .pad_conf0 = 0x00920301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },


	GPIO_END
};


/* East Community */
static const struct soc_gpio_map  gpe_gpio_map[] = {
	// /*
	//   FED94400: 01 03 91 00 00 00 C0 05-01 03 91 00 00 00 C0 05  *................*
	//   FED94410: 01 03 91 00 00 00 C0 05-01 03 91 00 00 00 C0 05  *................*
	//   FED94420: 01 03 11 00 00 00 C0 05-01 03 91 00 00 00 C0 05  *................*
	//   FED94430: 00 03 11 00 00 00 C0 05-00 03 91 00 00 00 C0 05  *................*
	//   FED94440: 01 03 91 00 00 00 C0 05-01 03 91 00 00 00 C0 05  *................*
	//   FED94450: 01 03 C1 00 00 00 C0 05-01 82 91 2C 41 00 C0 05  *...........,A...*
	// */
	// Native_M1,																			/* 00 PMU_SLP_S3_B */			/* PMU_SLP_S3_N */
	// Native_M1,																			/* 01 PMU_BATLOW_B */			/* PMU_BATLOW_B */
	// Native_M1,																			/* 02 SUS_STAT_B */				/* SUS_STAT_N */
	// Native_M1,																			/* 03 PMU_SLP_S0IX_B */			/* SLP_S0IX_N */
	// Native_M1,																			/* 04 PMU_AC_PRESENT */			/* PMU_AC_PRESENT */
	// Native_M1,																			/* 05 PMU_PLTRST_B */			/* PMU_PLTRST_B */
	// Native_M1,																			/* 06 PMU_SUSCLK */				/* PMU_SUSCLK_1P8 */
	// GPIO_NC,																			/* 07 PMU_SLP_LAN_B */			/*  */
	// Native_M1,																			/* 08 PMU_PWRBTN_B */			/* PMU_PWRBTN_N */
	// Native_M1,																			/* 09 PMU_SLP_S4_B */			/* PMU_SLP_S4_N */
	// NATIVE_FUNC(M1, P_1K_H, NA),														/* 10 PMU_WAKE_B */				/* PMU_WAKE_N */
	// GPIO_NC,																			/* 11 PMU_WAKE_LAN_B */			/* PMU_WAKE_LAN_N */				/* FIXME: Does this connect to onboard RTL8111? */
	
	/*E00 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E01 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E02 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E03 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E04 */ { .pad_conf0 = 0x00110301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E05 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E06 */ { .pad_conf0 = 0x00110301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E07 */ GPIO_SKIP,
	/*E08 */ { .pad_conf0 = 0X00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E09 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E10 */ { .pad_conf0 = 0X00C10301, .pad_conf1 = 0x05C00000, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E11 */ { .pad_conf0 = 0X2C918201, .pad_conf1 = 0X05C00041, .wake_mask = 1, .int_mask = 1, .gpe = 0 },
	

	// /*
	//   FED94800: 01 03 A1 00 20 00 C0 05-02 81 11 00 20 00 C0 05  *.... ....... ...*
	//   FED94810: 01 03 91 00 20 00 C0 05-01 03 A1 00 20 00 C0 05  *.... ....... ...*
	//   FED94820: 00 81 11 00 20 00 C0 05-01 03 A1 00 20 00 C0 05  *.... ....... ...*
	//   FED94830: 01 03 A1 00 20 00 C0 05-01 82 91 08 44 00 C0 05  *.... .......D...*
	//   FED94840: 01 03 91 00 20 00 C0 05-01 03 A1 00 20 00 C0 05  *.... ....... ...*
	//   FED94850: 00 03 11 00 20 00 C0 05-01 03 91 00 20 00 C0 05  *.... ....... ...*
	// */
	// GPIO_NC,																			/* 15 MF_GPIO_3 */				/*  */								/* FIXME: NC? Braswell doesn't have an ISH */
	// GPIO_NC,																			/* 16 MF_GPIO_7 */				/*  */								/* FIXME: NC? Braswell doesn't have an ISH */
	// GPIO_NC,																			/* 17 MF_I2C1_SCL */			/*  */								/* FIXME: NC? Braswell doesn't have an ISH */
	// GPIO_NC,																			/* 18 MF_GPIO_1 */				/*  */								/* FIXME: NC? Braswell doesn't have an ISH */
	// GPIO_NC,																			/* 19 MF_GPIO_5 */				/*  */								/* FIXME: NC? Braswell doesn't have an ISH */
	// GPIO_NC,																			/* 20 MF_GPIO_9 */				/*  */								/* FIXME: NC? Braswell doesn't have an ISH */
	// GPIO_NC,																			/* 21 MF_GPIO_0 */				/*  */								/* FIXME: NC? Braswell doesn't have an ISH */
	// GPIO_NC,																			/* 22 MF_GPIO_4 */				/*  */								/* FIXME: NC? Braswell doesn't have an ISH */
	// GPIO_NC,																			/* 23 MF_GPIO_8 */				/*  */								/* FIXME: NC? Braswell doesn't have an ISH */
	// GPIO_NC,																			/* 24 MF_GPIO_2 */				/*  */								/* FIXME: NC? Braswell doesn't have an ISH */
	// GPIO_NC,																			/* 25 MF_GPIO_6 */				/*  */								/* FIXME: NC? Braswell doesn't have an ISH */
	// GPIO_NC,																			/* 26 MF_I2C1_SDA */			/*  */								/* FIXME: NC? Braswell doesn't have an ISH */

	/*E15 */ { .pad_conf0 = 0x00A10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E16 */ { .pad_conf0 = 0x00118102, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E17 */ GPIO_SKIP,
	/*E18 */ { .pad_conf0 = 0x00A10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E19 */ { .pad_conf0 = 0x00118100, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E20 */ { .pad_conf0 = 0x00A10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E21 */ { .pad_conf0 = 0x00A10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E22 */ { .pad_conf0 = 0x08918201, .pad_conf1 = 0x05C00044, .wake_mask = 0, .int_mask = 1, .gpe = 0 },
	/*E23 */ { .pad_conf0 = 0x00910301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E24 */ { .pad_conf0 = 0x00A10301, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E25 */ { .pad_conf0 = 0x00110300, .pad_conf1 = 0x05C00020, .wake_mask = 0, .int_mask = 0, .gpe = 0 },
	/*E26 */ GPIO_SKIP,
	

	GPIO_END
};


static struct soc_gpio_config gpio_config = {
	/* BSW */
	.north = gpn_gpio_map,
	.southeast = gpse_gpio_map,
	.southwest  = gpsw_gpio_map,
	.east = gpe_gpio_map
};

struct soc_gpio_config *mainboard_get_gpios(void)
{

	return &gpio_config;
}
