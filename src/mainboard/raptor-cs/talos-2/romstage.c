/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/isteps.h>
#include <program_loading.h>

void main(void)
{
    console_init();
    istep89();
    istep810();
    run_ramstage();
}
