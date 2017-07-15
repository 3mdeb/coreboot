/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008-2009 coresystems GmbH
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

#include <arch/acpi.h>
#include <arch/io.h>
#include <console/console.h>
#include <cpu/x86/smm.h>

#include <soc/nvs.h>
#include <soc/pm.h>
#include <soc/gpio.h>

#include "onboard.h"

/* The wake gpio is SUS_GPIO[0]. */
#define WAKE_GPIO_EN SUS_GPIO_EN0

int mainboard_io_trap_handler(int smif)
{
	switch (smif) {
	case 0x99:
		printk(BIOS_DEBUG, "Sample\n");
		smm_get_gnvs()->smif = 0;
		break;
	default:
		return 0;
	}

	/*
	 * On success, the IO Trap Handler returns 0
	 * On failure, the IO Trap Handler returns a value != 0
	 *
	 * For now, we force the return value to 0 and log all traps to
	 * see what's going on.
	 */
	//gnvs->smif = 0;
	return 1;
}

/*
 * The entire 32-bit ALT_GPIO_SMI register is passed as a parameter. Note, that
 * this includes the enable bits in the lower 16 bits.
 */
void mainboard_smi_gpi(uint32_t alt_gpio_smi)
{
#if IS_ENABLED(CONFIG_EC_GOOGLE_CHROMEEC)
	if (alt_gpio_smi & (1 << EC_SMI_GPI)) {
		/* Process all pending events */
		while (mainboard_smi_ec() != 0)
			;
	}
#endif
}

void mainboard_smi_sleep(uint8_t slp_typ)
{
	/* Disable USB charging if required */
	switch (slp_typ) {
	case ACPI_S3:
		/* Enable wake pin in GPE block. */
		enable_gpe(WAKE_GPIO_EN);
		break;
	case ACPI_S5:
		break;
	}

}

int mainboard_smi_apmc(uint8_t apmc)
{
	switch (apmc) {
	case APM_CNT_ACPI_ENABLE:
		break;
	case APM_CNT_ACPI_DISABLE:
		break;
	}
	return 0;
}
