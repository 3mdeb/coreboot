/*
 * This file is part of the libpayload project.
 *
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
 * Copyright (C) 2017 Patrick Rudolph <siro@das-labor.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

	.code32
	.global _entry
	.text
	.align 4

/*
 * Our entry point - assume that the CPU is in 32 bit protected mode and
 * all segments are in a flat model.
 */
_entry:
	jmp _init

	.align 4

#define MB_MAGIC 0x1BADB002
#define MB_FLAGS 0x00010003
#define DEBUG

mb_header:
	.long MB_MAGIC
	.long MB_FLAGS
	.long -(MB_MAGIC + MB_FLAGS)
	.long mb_header
	.long _start
	.long _edata
	.long _end
	.long _init

#define CB_MAGIC_VALUE	0x12345678
#define CB_MAGIC	0x04
#define CB_ARGV		0x08
#define CB_ARGC		0x10

/*
 * This function saves off the previous stack and switches us to our
 * own execution environment.
 */
_init:
	/* No interrupts, please. */
	cli

	/* Store EAX and EBX */
	movl %eax, loader_eax
	movl %ebx, loader_ebx

	/* Copy argv[] and argc as demanded by the Payload API,
	 * see https://www.coreboot.org/Payload_API and exec.S.
	 */
	cmpl $CB_MAGIC_VALUE, CB_MAGIC(%esp)
	jne 1f

	movl CB_ARGV(%esp), %eax
	movl %eax, main_argv
	movl $0, (main_argv+4)

	movl CB_ARGC(%esp), %eax
	movl %eax, main_argc
1:
	/* Store current stack pointer and set up new stack. */
	movl %esp, %ebx
	movl $_stack, %esp
	andl $0xfffffff0, %esp

	/* Clear bss */
	movl $_edata, %edi
	addl $3, %edi
	andl $(~3), %edi
	movl %esp, %ecx
	subl %edi, %ecx
	shrl $2, %ecx
	xorl %eax, %eax
	rep stosl

	pushl $0	/* keep stack aligned for x64 calls (SysV ABI) */
	pushl $0
	pushl $0
	pushl %ebx

	/* Enable special x86 functions if present. */
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx

	movl $0, %eax
	cpuid
	/* Test if CPUID(eax=1) is available. */
	test %eax, %eax
	je cpuid_done

	/* Get CPU features. */
	movl $1, %eax
	cpuid

cpuid_fpu:
	/* Test if x87 FPU is present */
	test $1, %edx
	je cpuid_sse

	fninit
	movl %cr0, %eax
	andl $0xFFFFFFFB, %eax	/* clear EM */
	orl $0x00000022, %eax	/* set MP, NE */
	movl %eax, %cr0

cpuid_sse:
	/* Test if SSE is available */
	test $0x02000000, %edx
	je cpuid_done

	movl %cr4, %eax
	orl $0x00000600, %eax	/* set OSFXSR, OSXMMEXCPT */
	movl %eax, %cr4

cpuid_done:
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax

	/* Prepare GDT for 64-bit operation */
	lgdt	%cs:gdtptr
	ljmp $0x08, $1f
1:

	call _gen_long_mode_pgtbl32

#ifdef DEBUG
	mov	$0x3f8, %dx
	mov	$'C', %al
	out	%al, (%dx)
	mov	$'R', %al
	out	%al, (%dx)
	mov	$'3', %al
	out	%al, (%dx)
#endif

	/* Load identity mapped page tables */
	movl	$(_pagetables), %eax
	movl	%eax, %cr3

#ifdef DEBUG
	mov	$'+', %al
	out	%al, (%dx)
	mov	$'\r', %al
	out	%al, (%dx)
	mov	$'\n', %al
	out	%al, (%dx)

	/* Let UART catch up. Ideally we should wait until buffer is empty */
	movl	$2000000, %ecx
8:
	nop
	loop 8b

	mov	$'P', %al
	out	%al, (%dx)
	mov	$'A', %al
	out	%al, (%dx)
	mov	$'E', %al
	out	%al, (%dx)
#endif

	/* Enable PAE */
	movl	%cr4, %eax
	btsl	$5, %eax
	movl	%eax, %cr4

#ifdef DEBUG
	mov	$'+', %al
	out	%al, (%dx)
	mov	$'\r', %al
	out	%al, (%dx)
	mov	$'\n', %al
	out	%al, (%dx)

	movl	$2000000, %ecx
8:
	nop
	loop 8b

	mov	$'L', %al
	out	%al, (%dx)
	mov	$'M', %al
	out	%al, (%dx)
	mov	$'E', %al
	out	%al, (%dx)
#endif

	/* Enable long mode */
	movl	$0xC0000080, %ecx	/* 0xC0000080 = IA32_EFER */
	rdmsr
	btsl	$8, %eax
	wrmsr

#ifdef DEBUG
	mov	$0x3f8, %dx
	mov	$'+', %al
	out	%al, (%dx)
	mov	$'\r', %al
	out	%al, (%dx)
	mov	$'\n', %al
	out	%al, (%dx)

	movl	$2000000, %ecx
8:
	nop
	loop 8b

	mov	$'P', %al
	out	%al, (%dx)
	mov	$'G', %al
	out	%al, (%dx)
#endif

	/* Enable paging */
	movl	%cr0, %eax
	btsl	$31, %eax
	movl	%eax, %cr0

#ifdef DEBUG
	mov	$'+', %al
	out	%al, (%dx)
	mov	$'\r', %al
	out	%al, (%dx)
	mov	$'\n', %al
	out	%al, (%dx)

	movl	$2000000, %ecx
8:
	nop
	loop 8b

	mov	$'>', %al
	out	%al, (%dx)
#endif

	/* Use long jump to switch to 64-bit code segment */
	ljmp $0x18, $1f
.code64
1:

#ifdef DEBUG
	movl	$2000000, %ecx
8:
	nop
	loop 8b

	mov	$'G', %al
	out	%al, (%dx)
	mov	$'o', %al
	out	%al, (%dx)
	mov	$'!', %al
	out	%al, (%dx)
	mov	$'\r', %al
	out	%al, (%dx)
	mov	$'\n', %al
	out	%al, (%dx)

	movl	$2000000, %ecx
8:
	nop
	loop 8b
#endif

	/* Let's rock. */
	call start_main

	/* %eax has the return value - pass it on unmolested */
	/* Just in case */
	cli

	/* Ensure cache is clean. */
	invd

	push %rax

	/* Set  32-bit code segment and ss */
	mov	$0x10, %rcx
	call	SetCodeSelector

.code32
	/* Running in 32-bit compatibility mode */

	/* Use flat 32-bit data segment. */
	movl	$0x10, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %ss
	movl	%eax, %fs
	movl	%eax, %gs

	/* Disable paging */
	movl	%cr0, %eax
	andl	$0x7FFFFFFF, %eax
	movl	%eax, %cr0

	/* Disable long mode */
	movl	$0xC0000080, %ecx
	rdmsr
	btrl	$8, %eax
	wrmsr

	/* Disable PAE */
	movl	%cr4, %eax
	andl	$(~0x20), %eax
	movl	%eax, %cr4

	popl %eax
	addl $4, %esp	/* we pushed %rax, but poped %eax */

	/* Restore old stack. */
	popl %esp

	/* Return to the original context. */
	ret

	.align	4
.globl gdtptr
gdtptr:
	.word	gdt_end - gdt -1 /* compute the table limit */
	.long	gdt		 /* we know the offset */

	.align	4
gdt:
	/* selgdt 0, unused */
	.word	0x0000, 0x0000		/* dummy */
	.byte	0x00, 0x00, 0x00, 0x00

	/* selgdt 0x08, flat code segment */
	.word	0xffff, 0x0000
	.byte	0x00, 0x9b, 0xcf, 0x00 /* G=1 and 0x0f, So we get 4Gbytes
					  for limit */

	/* selgdt 0x10,flat data segment */
	.word	0xffff, 0x0000
	.byte	0x00, 0x93, 0xcf, 0x00

	/* selgdt 0x18, flat code segment (64-bit) */
	.word   0xffff, 0x0000
	.byte   0x00, 0x9b, 0xaf, 0x00

gdt_end:


/* Page table attributes: WB, User+Supervisor, Present, Writeable */
#define PAGE_ATTRIBUTES 0x007

/*
 * Creates identity mapped page entries.
 * Identity maps 0 - 4GiB using arbitraty page size.
 * Use offset:
 *  12         for 4 KiB pages
 *  12 + 9     for 2 MiB pages
 *  12 + 9 + 9 for 1 GiB pages
 *
 * edi: The destination (in _pagetables)
 * Clobers: esi, edi, ebx
 */
.code32
.macro gen_page_entries32 offset
	xorl	%esi, %esi
	xorl	%ebx, %ebx
1:
	movl	$(PAGE_ATTRIBUTES | (1 << 7)), 0x00(%edi)
	orl	%esi, 0x00(%edi)
	movl	%ebx, 0x04(%edi)
	addl	$(1 << (\offset)), %esi
	addl	$8, %edi
	cmpl	%ebx, %esi
	ja	1b
.endm

_gen_long_mode_pgtbl32:

	/* Setup long mode: identity map 0-4GiB */

	movl	$(_pagetables), %ecx
	xorl	%eax, %eax

	/* Generate one PM4LE entry - point to PDPE */
	leal	(0x1000)(%ecx), %edi
	movl	%edi, (%ecx)
	orl	$(PAGE_ATTRIBUTES), (%ecx)
	movl	%eax, 4(%ecx)

	/* Read CPUID(eax=0x80000001) EDX BIT26 */
	movl	$0x80000001, %eax
	cpuid
	btsl	$26, %edx
	jc	paging_1g_pages
paging_2m_pages:

	/* With 2 MiB hugepages we need 6 pages of 4096 byte */

	movl	$(PAGE_ATTRIBUTES), %ebx

	/* edi points to PDPE table, point esi to PDE tables */
	movl	%edi, %esi
	addl	$0x1000, %esi

	movl	$4, %eax
	/* Point to the to be generated PDE tables */
1:
	mov	%ebx, 0x0(%edi)
	or	%esi, 0x0(%edi)
	movl	$0, 0x4(%edi)
	add	$0x1000, %esi
	add	$8, %edi
	dec	%eax
	jnz	1b

	/* point edi to PDE tables */
	movl	$(_pagetables), %ecx
	leal	(0x2000)(%ecx), %edi

	/* Fill the PDE tables : Generate 4 * 512 PDE entries */
	gen_page_entries32 (12 + 9)

	ret

paging_1g_pages:
	/* With 1 GiB hugepages we need 2 pages of 4096 byte */

	/* edi points to PDPE table already */

	/* Fill the PDPE table : Generate 4 PDPE entries */
	gen_page_entries32 (12 + 9 + 9)

	ret

.code64
SetCodeSelector:
	# pop the return address from stack
	pop	%rbx

	# save rsp because we need to push it after ss
	mov	%rsp, %rdx

	# use iret to jump to a 32-bit offset in a new code segment
	# iret will pop cs:rip, flags, then ss:rsp
	mov	%ss, %ax	# need to push ss..
	push	%rax		# push ss instuction not valid in x64 mode,
				# so use ax
	push	%rdx		# the rsp to load
	pushfq			# push rflags
	push	%rcx		# cx is code segment selector from caller
	push	%rbx		# push the IP for the next instruction

	# the iretq will behave like ret, with the new cs/ss value loaded
	iretq

.align 4096
_pagetables:
	.fill 4096 * 6
