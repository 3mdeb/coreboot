/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <program_loading.h>
#include <superio/aspeed/common/aspeed.h>
#include <superio/aspeed/ast2400/ast2400.h>
#include <console/uart.h>
#include <bootblock_common.h>
#include <arch/io.h>

void mainboard_post(uint8_t value)
{
	outb(value, 0x82);
}

void bootblock_mainboard_early_init(void)
{
	const pnp_devfn_t serial_dev = PNP_DEV(0x2e, AST2400_SUART1);
	aspeed_enable_uart_pin(serial_dev);
	aspeed_enable_serial(serial_dev, CONFIG_TTYS0_BASE);
}
