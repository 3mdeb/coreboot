/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <program_loading.h>
#include <superio/aspeed/common/aspeed.h>
#include <superio/aspeed/ast2400/ast2400.h>
#include <console/uart.h>

void bootblock_main(void);


static void early_config_superio(void)
{
	const pnp_devfn_t serial_dev = PNP_DEV(0x2e, AST2400_SUART1);
	aspeed_enable_uart_pin(serial_dev);
	aspeed_enable_serial(serial_dev, CONFIG_TTYS0_BASE);
}

/* The qemu part of all this is very, very non-hardware like.
 * So it gets its own bootblock.
 */
void bootblock_main(void)
{
	early_config_superio();
	if (CONFIG(BOOTBLOCK_CONSOLE)) {
		console_init();
	}

	run_romstage();
}
