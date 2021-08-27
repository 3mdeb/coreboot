/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/mmio.h>
#include <assert.h>
#include <console/console.h>
#include <cpu/power/scom.h>
#include <string.h>

#include "homer.h"

#define CODE_SIZE(x) ((x ## _end) - (x))

extern uint8_t sys_reset_int[];
extern uint8_t sys_reset_int_end[];
extern uint8_t ext_int[];
extern uint8_t ext_int_end[];
extern uint8_t hyp_virt_int[];
extern uint8_t hyp_virt_int_end[];

void configure_xive(int core)
{
	uint64_t tmp;
	printk(BIOS_ERR, "sys_reset_int code size = %#lx\n", CODE_SIZE(sys_reset_int));
	printk(BIOS_ERR, "ext_int code size = %#lx\n", CODE_SIZE(ext_int));
	printk(BIOS_ERR, "hyp_virt_int code size = %#lx\n", CODE_SIZE(hyp_virt_int));

	//~ LINKER_BUG_ON(CODE_SIZE(sys_reset_int) < 0x20);

	memcpy((void *)0x100, sys_reset_int, CODE_SIZE(sys_reset_int));
	memcpy((void *)0x500, ext_int, CODE_SIZE(ext_int));
	memcpy((void *)0xEA0, hyp_virt_int, CODE_SIZE(hyp_virt_int));

	/* IVPE BAR */
	write_scom(0x05013012, 0x8006020000000000);

	/* FSP BAR */
	write_scom(0x0501290B, 0x0006030100000000);

	/* PSI HB BAR */
	write_scom(0x0501290A, 0x0006030203000000);
	write_scom(0x0501290A, 0x0006030203000001);

	/* Disable VPC Pull error */
	scom_and(0x05013179, ~PPC_BIT(30));

	/* PSI HB ESB BAR */
	write_scom(0x05012916, 0x00060302031C0000);
	write_scom(0x05012916, 0x00060302031C0001);

	/* XIVE IC BAR */
	write_scom(0x05013010, 0x8006030203100000);

	printk(BIOS_ERR, "BARs set\n");

	/* Set HB mode on P3PC register */
	scom_or(0x05013110, PPC_BIT(33));

	/* Disable PSI interrupts */
	write_scom(0x05012913, PPC_BIT(3));

	printk(BIOS_ERR, "HB mode set, PSI interrupts masked\n");

	void *esb_bar = (void *)0x00060302031C0000;
	/* Mask all interrupt sources */
	for (int i = 0; i < 14; i++) {
		tmp = read64(esb_bar + i*0x1000 + 0xD00);
		eieio();
		tmp = read64(esb_bar + i*0x1000 + 0x800);
		assert(tmp == 1);
	}

	printk(BIOS_ERR, "LSI interrupts masked\n");

	/* Route interrupts to CEC instead of FSP */
	void *hb_bar = (void *)0x0006030203000000;
	write64(hb_bar + 0x20, read64(hb_bar + 0x20) | PPC_BIT(3));

	/* Enable PSIHB interrupts */
	write64(hb_bar + 0x58, read64(hb_bar + 0x58) | PPC_BIT(0));

	/* Route interrupts to first thread of active core */
	int offset = (core < 16) ? 0x48 : 0x68;
	void *xive_ic_bar = (void *)0x0006030203100000;
	write64(xive_ic_bar + 0x400 + offset, PPC_BIT(4 * (core % 16)));
	eieio();

	/* Configure LSI mode for HB CEC interrupts */
	void *ivpe_bar = (void *)0x0006020000000000;
	write8(ivpe_bar + 0x38, 0x81);
	eieio();

	/* Route LSI to master processor */
	write64(hb_bar + 0x68, 0x0006030203102000);
	write64(hb_bar + 0x68, 0x0006030203102001);
	write64(hb_bar + 0x58, 0);

	/* Enable LSI interrupts */
	tmp = read64(xive_ic_bar + 0x3000 + 0xC00);

	/* Unmask PSU interrupts */
	tmp = read64(esb_bar + 0xD*0x1000 + 0xC00);
	eieio();
	tmp = read64(esb_bar + 0xD*0x1000 + 0x800);
	assert(tmp == 0);
	/* EOI, just in case */
	tmp = read64(esb_bar + 0xD*0x1000);
}
