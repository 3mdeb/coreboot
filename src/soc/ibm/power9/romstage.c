/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/isteps.h>
#include <console/console.h>
#include <arch/cpu.h>

asmlinkage void scom_init(void);
asmlinkage void car_stage_entry(void);

asmlinkage void scom_init(void)
{
    istep_8_9();
    istep_8_10();
    printk(BIOS_EMERG, "Hello from SoC/scom_init()!\n");
}


asmlinkage void car_stage_entry(void)
{
}
