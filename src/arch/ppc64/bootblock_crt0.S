/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Early initialization code for POWER8.
 */

#define FIXUP_ENDIAN						   \
	tdi   0,0,0x48;	  /* Reverse endian of b . + 8		*/ \
	b     $+44;	  /* Skip trampoline if endian is good	*/ \
	.long 0xa600607d; /* mfmsr r11				*/ \
	.long 0x01006b69; /* xori r11,r11,1			*/ \
	.long 0x00004039; /* li r10,0				*/ \
	.long 0x6401417d; /* mtmsrd r10,1			*/ \
	.long 0x05009f42; /* bcl 20,31,$+4			*/ \
	.long 0xa602487d; /* mflr r10				*/ \
	.long 0x14004a39; /* addi r10,r10,20			*/ \
	.long 0xa6035a7d; /* mtsrr0 r10				*/ \
	.long 0xa6037b7d; /* mtsrr1 r11				*/ \
	.long 0x2400004c  /* rfid				*/

/* SBE sets HRMOR to 130MB, ie 2MB into 10MB cache. Our bootblock is loaded
 * 12 KB above that address - it is used as a place for interrupt vectors in
 * Hostboot, but it isn't actually a part of the HBBL partition to save space
 * in PNOR image. */

.section ".text._start", "ax", %progbits
.globl _start
_start:
	/* FIXUP_ENDIAN */

	/* Fill first 12 KB with jumps to real entry point to accomodate for
	 * exception vectors.
	 * TODO: check which vectors are actually used, if any, and add useful
	 * handlers */
	.rept 0x3000/4
	b _real_start
	.endr

_real_start:
	/* Set program priority to medium */
	or		%r2, %r2, %r2

/*
#define MMIO_GROUP0_CHIP0_LPC_BASE_ADDR 0x8006030000000000
#define LPC_ADDR_START 0xC0000000
#define LPCHC_IO_SPACE 0xD0010000
#define LPC_BASE_ADDR (MMIO_GROUP0_CHIP0_LPC_BASE_ADDR + LPCHC_IO_SPACE - LPC_ADDR_START)
        LPC_BASE_ADDR (MMIO_GROUP0_CHIP0_LPC_BASE_ADDR + 0x10010000)
*/
	lis		%r10, 0x8006
	ori		%r10, %r10, 0x0300
	rotldi	%r10, %r10, 32
	oris	%r10, %r10, 0x1001

	li		%r3, 0x82	/* port number */
	li		%r2, 0x88	/* value */
	stbcix	%r2, %r10, %r3
	eieio

	/* Stack */
	lis		%r1, _estack@ha
	addi	%r1, %r1, _estack@l

	/* Clear .bss section */
	lis		%r5, _bss@ha
	addi	%r5, %r5, _bss@l
	lis		%r6, _ebss@ha
	addi	%r6, %r6, _ebss@l
	addi	%r6, %r6, -1    /* end before _ebss */
1:
	dcbz	0, %r5
	addi	%r5, %r5, 128
	cmpld	cr7, %r5, %r6
	blt		cr7, 1b

	/* Load official procedure descriptor address for main() */
	lis		%r12, main@ha
	addi	%r12, %r12, main@l

	/* Load TOC pointer and jump to main() */
	ld		%r2, 8(%r12)
	b main
