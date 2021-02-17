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
	/* QEMU with hb-mode=on starts at address 0x10, while hardware at 0x0. */
#if !CONFIG(BOOTBLOCK_IN_SEEPROM)
	nop
	nop
	nop
	nop
	FIXUP_ENDIAN
#endif

	/* Set program priority to medium */
	or		%r2, %r2, %r2

	/* Stack */
	lis		%r1, _estack@ha
	addi	%r1, %r1, _estack@l

#if CONFIG(BOOTBLOCK_IN_SEEPROM)
	/* L3 cache is invalid except for [0, end of HBBL] range. Make the rest of
	 * it valid, or embrace the checstops. End of loaded part of HBBL is at
	 * address 0x8 (see https://github.com/open-power/hostboot/blob/master/src/import/chips/p9/procedures/hwp/nest/p9_sbe_hb_structures.H#L84),
	 * the first 4 bytes are for the jump into HBBL). */
	ld	%r5, 12(0)

	/* Validate and initialize to zeroes the rest of first 2MB of cache */
	/* FIXME: this overwrites the code if clearing >2MB. Why we don't have 10MB? */
	lis		%r6, 0x20
	addi	%r6, %r6, -1
1:
	dcbz	0, %r5
	addi	%r5, %r5, 128
	cmpld	cr7, %r5, %r6
	blt		cr7, 1b
#endif

	/* Enable floating point and vector operations */
	/* Vector operations are sometimes generated for code like 
	 * 'uint8_t x[32] = {0}', this results in an exception when vector registers
	 * (VEC) are not enabled. VSX (vector-scalar extension) is also enabled,
	 * there is no reason not to. Floating point must also be enabled for VSX.
	 */
	mfmsr	%r3
	ori		%r3, %r3, 0x2000	/* FP = 1 */
	oris	%r3, %r3, 0x0280	/* VEC = 1, VSX = 1 */
	mtmsr	%r3

	/* Load official procedure descriptor address for main() */
	lis		%r12, main@ha
	addi	%r12, %r12, main@l

	/* Load TOC pointer and jump to main() */
	ld		%r2, 8(%r12)
	b main
