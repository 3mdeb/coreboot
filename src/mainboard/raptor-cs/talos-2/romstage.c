/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/isteps.h>
#include <program_loading.h>

void scom_init(void);

void main(void)
{
    console_init();
    scom_init();
    run_ramstage();
}
