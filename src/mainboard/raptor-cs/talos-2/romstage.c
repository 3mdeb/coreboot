/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/istep_8_9.h>
#include <cpu/power/istep_8_10.h>
#include <program_loading.h>

void main(void)
{
    console_init();
    istep_8_9();
    istep_8_10();
    run_ramstage();
}
