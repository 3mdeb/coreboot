/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008-2009 coresystems GmbH
 * Copyright (C) 2014 Vladimir Serbinenko
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <delay.h>
#include <stdint.h>
#include <elog.h>
#include <arch/beep.h>
#include <arch/io.h>
#include <console/console.h>
#include <cpu/x86/smm.h>
#include <southbridge/intel/bd82x6x/pch.h>
#include <southbridge/intel/common/pmutil.h>
#include <southbridge/intel/common/pmbase.h>
#include <superio/smsc/sch5545/sch5545.h>

static uint8_t intruder_already_reported;

static void dump_pmbase(void)
{

	printk(BIOS_SPEW, "PM1_STS: %04x\n"
			  "PM1_EN: %04x\n"
			  "PM1_CNT: %08x\n"
			  "PM1_TMR: %08x\n"
			  "GPE0_STS: %08x\n"
			  "GPE0_EN: %08x\n"
			  "SMI_EN: %08x\n"
			  "SMI_STS: %08x\n"
			  "ALT_GP_SMI_EN: %04x\n"
			  "ALT_GP_SMI_STS: %04x\n"
			  "GPE_CNTL: %02x\n",
			  read_pmbase16(PM1_STS), read_pmbase16(PM1_EN),
			  read_pmbase32(PM1_CNT), read_pmbase32(PM1_TMR),
			  read_pmbase32(GPE0_STS), read_pmbase32(GPE0_EN),
			  read_pmbase32(SMI_EN), read_pmbase32(SMI_STS),
			  read_pmbase16(ALT_GP_SMI_EN),
			  read_pmbase16(ALT_GP_SMI_STS), read_pmbase8(GPE_CNTL)
		);
}


void mainboard_smi_gpi(u32 gpi_sts)
{
	printk(BIOS_SPEW, "%s: gpi_sts: %08x\n", __func__, gpi_sts);
	dump_pmbase();
}

int mainboard_smi_apmc(u8 data)
{
	u8 val;
	switch (data) {
	case APM_CNT_ACPI_ENABLE:
		printk(BIOS_SPEW, "%s: APM CNT EN: %02x\n", __func__, data);
		dump_pmbase();
		/* Enable wake on PS2 */
		val = inb(SCH5545_RUNTIME_REG_BASE + SCH5545_RR_PME_EN1);
		val |= (SCH5545_KBD_PME_EN | SCH5545_MOUSE_PME_EN);
		outb(val, SCH5545_RUNTIME_REG_BASE + SCH5545_RR_PME_EN1);

		/* Clear pending and enable PMEs */
		outb(SCH5545_GLOBAL_PME_STS, SCH5545_RUNTIME_REG_BASE + SCH5545_RR_PME_STS);
		outb(SCH5545_GLOBAL_PME_EN, SCH5545_RUNTIME_REG_BASE + SCH5545_RR_PME_EN);
		break;
	case APM_CNT_ACPI_DISABLE:
		printk(BIOS_SPEW, "%s: APM CNT DIS: %02x\n", __func__, data);
		dump_pmbase();
		/* Disable wake on PS2 */
		val = inb(SCH5545_RUNTIME_REG_BASE + SCH5545_RR_PME_EN1);
		val &= ~(SCH5545_KBD_PME_EN | SCH5545_MOUSE_PME_EN);
		outb(val, SCH5545_RUNTIME_REG_BASE + SCH5545_RR_PME_EN1);

		/* Clear pending and disable PMEs */
		outb(SCH5545_GLOBAL_PME_STS, SCH5545_RUNTIME_REG_BASE + SCH5545_RR_PME_STS);
		outb(0, SCH5545_RUNTIME_REG_BASE + SCH5545_RR_PME_EN);
		break;
	default:
		break;
	}
	return 0;
}

void mainboard_smi_sleep(u8 slp_typ)
{
	printk(BIOS_SPEW, "%s: SMI sleep: %02x\n", __func__, slp_typ);
	dump_pmbase();
	if (slp_typ == 3) {

	}
}

static void beep_start(unsigned int frequency)
{
	unsigned int count = 1193180 / frequency;

	/* Switch on the speaker */
	outb(inb(0x61)|3, 0x61);

	/* Set command for counter 2, 2 byte write */
	outb(0xB6, 0x43);

	/* Select desired Hz */
	outb(count & 0xff, 0x42);
	outb((count >> 8) & 0xff, 0x42);

}

void mainboard_smi_intruder(u32 tco_sts)
{
	/* Disable interrupt for intruder detection temporarily */
	write_pmbase16(TCO2_CNT, read_pmbase16(TCO2_CNT) & ~4);
	/* Signal the case was opened */
	if ((tco_sts & (1 << 16)) && CONFIG(ELOG) && !intruder_already_reported)
		elog_add_event_byte(ELOG_TYPE_INTRUDER_DETECTION,
				    ELOG_CASE_OPENED);

	/*
	 * Software should wait before reading the intruder status again after
	 * clearing it. However beep already block for ~100usecs which is more
	 * than required 65usec according to PCH datasheet. Wait for 1 ms in
	 * order to ensure that INTR_DET will assert if the case is still open.
	 */
	udelay(1000);
	if ((read_pmbase16(TCO2_STS) & 1) && !intruder_already_reported) {
		/* Case still open */
		beep_start(2500);
		intruder_already_reported = 1;
	} else {
		/* Case is already closed */
		intruder_already_reported = 0;
		/* Switch off the speaker */
		outb(inb(0x61) & 0xfc, 0x61);
		if (CONFIG(ELOG))
			elog_add_event_byte(ELOG_TYPE_INTRUDER_DETECTION,
					    ELOG_CASE_CLOSED);
	}
	/* Clear the status again regardless of the result */
	write_pmbase16(TCO2_STS ,read_pmbase16(TCO2_STS) | 1);

	/* Enable SMI back for intruder detection */
	write_pmbase16(TCO2_CNT, read_pmbase16(TCO2_CNT) | 4);
}

