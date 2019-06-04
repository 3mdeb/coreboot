/*
 * This file is part of the libpayload project.
 *
 * Copyright 2013 Google Inc.
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

	.align 16
	.global exception_stack_end
exception_stack_end:
	.quad 0
	.global exception_state
exception_state:
	.quad 0

/* Some temporary variables which are used while saving exception state. */
vector:
	.quad 0
error_code:
	.quad 0
old_rsp:
	.quad 0
old_rax:
	.quad 0
old_rdx:
	.quad 0

	.align 16

.code64

/*
 * Each exception vector has a small stub associated with it which sets aside
 * the error code, if any, records which vector we entered from, and calls
 * the common exception entry point. Some exceptions have error codes and some
 * don't, so we have a macro for each type.
 */

	.macro stub num
exception_stub_\num:
	movq	$0, error_code
	movq	$\num, vector
	jmp	exception_common
	.endm

	.macro stub_err num
exception_stub_\num:
	popq	error_code
	movq	$\num, vector
	jmp	exception_common
	.endm

	.altmacro
	.macro	user_defined_stubs from, to
	stub	\from
	.if	\to-\from
	user_defined_stubs	%(from+1),\to
	.endif
	.endm

	stub 0
	stub 1
	stub 2
	stub 3
	stub 4
	stub 5
	stub 6
	stub 7
	stub_err 8
	stub 9
	stub_err 10
	stub_err 11
	stub_err 12
	stub_err 13
	stub_err 14
	stub 15
	stub 16
	stub_err 17
	stub 18
	stub 19
	stub 20
	stub 21
	stub 22
	stub 23
	stub 24
	stub 25
	stub 26
	stub 27
	stub 28
	stub 29
	stub_err 30
	stub 31
	/* Split the macro so we avoid a stack overflow. */
	user_defined_stubs 32, 63
	user_defined_stubs 64, 127
	user_defined_stubs 128, 191
	user_defined_stubs 192, 255

exception_common:
	/*
	 * Save off the stack pointer and old eax value and install the
	 * exception stack. eax points to the old stack which has the
	 * exception ip, cs, and flags.
	 */
	movq	%rax, old_rax
	movq	24(%rsp), %rax
	movq	%rax, old_rsp
	movq	%rdx, old_rdx
	movq	%rsp, %rax
	movq	exception_stack_end, %rsp

	/*
	 * Push values onto the top of the exception stack to form an
	 * exception state structure.
	 */
	pushq	vector
	pushq	error_code
	mov	%gs, %rdx
	pushq	%rdx
	mov	%fs, %rdx
	pushq	%rdx
	mov	%es, %rdx
	pushq	%rdx
	mov	%ds, %rdx
	pushq	%rdx
	mov	%ss, %rdx
	pushq	%rdx
	pushq	8(%rax)
	pushq	16(%rax)
	pushq	(%rax)
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%r11
	pushq	%r10
	pushq	%r9
	pushq	%r8
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp
	pushq	old_rsp
	pushq	%rbx
	pushq	old_rdx
	pushq	%rcx
	pushq	old_rax

	/*
	 * Call the C exception handler. It will find the exception state
	 * using the exception_state global pointer. Not
	 * passing parameters means we don't have to worry about what ABI
	 * is being used.
	 */
	movq	%rsp, exception_state
	call	exception_dispatch

	/*
	 * Restore state from the exception state structure, including any
	 * changes that might have been made.
	 */
	popq	old_rax
	popq	%rcx
	popq	old_rdx
	popq	%rbx
	popq	old_rsp

	movq	old_rsp, %rax
	andq	$~(0xf), %rax
	subq	$40, %rax

	popq	%rbp
	popq	%rsi
	popq	%rdi
	popq	%r8
	popq	%r9
	popq	%r10
	popq	%r11
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	(%rax)
	popq	16(%rax)
	popq	8(%rax)
	popq	%rdx
	mov	%rdx, %ss
	popq	%rdx
	mov	%rdx, %ds
	popq	%rdx
	mov	%rdx, %es
	popq	%rdx
	mov	%rdx, %fs
	popq	%rdx
	mov	%rdx, %gs

	movq	%rax, %rsp
	movq	old_rax, %rax
	movq	old_rdx, %rdx

	/* Return from the exception. */
	iretq

	/*
	 * Record the target and construct the actual entry at init time. This
	 * is necessary because the linker doesn't want to construct the entry
	 * for us.
	 */
	.macro interrupt_gate target
	.quad \target
	.quad \target
	.endm

	.altmacro
	.macro	user_defined_gates from, to
	interrupt_gate	exception_stub_\from
	.if	\to-\from
	user_defined_gates	%(from+1),\to
	.endif
	.endm

	.align 8
	.global	idt
idt:
	interrupt_gate exception_stub_0
	interrupt_gate exception_stub_1
	interrupt_gate exception_stub_2
	interrupt_gate exception_stub_3
	interrupt_gate exception_stub_4
	interrupt_gate exception_stub_5
	interrupt_gate exception_stub_6
	interrupt_gate exception_stub_7
	interrupt_gate exception_stub_8
	interrupt_gate exception_stub_9
	interrupt_gate exception_stub_10
	interrupt_gate exception_stub_11
	interrupt_gate exception_stub_12
	interrupt_gate exception_stub_13
	interrupt_gate exception_stub_14
	interrupt_gate exception_stub_15
	interrupt_gate exception_stub_16
	interrupt_gate exception_stub_17
	interrupt_gate exception_stub_18
	interrupt_gate exception_stub_19
	interrupt_gate exception_stub_20
	interrupt_gate exception_stub_21
	interrupt_gate exception_stub_22
	interrupt_gate exception_stub_23
	interrupt_gate exception_stub_24
	interrupt_gate exception_stub_25
	interrupt_gate exception_stub_26
	interrupt_gate exception_stub_27
	interrupt_gate exception_stub_28
	interrupt_gate exception_stub_29
	interrupt_gate exception_stub_30
	interrupt_gate exception_stub_31
	user_defined_gates 32, 63
	user_defined_gates 64, 127
	user_defined_gates 128, 191
	user_defined_gates 192, 255
idt_end:

/* IDT pointer for use with lidt */
idt_ptr:
	.word idt_end - idt - 1
	.quad idt

	.global exception_init_asm
exception_init_asm:
	/*
	 * Loop over the entries which start out as two copies of the target
	 * address. We can turn them into real interrupt gates by selectively
	 * replacing certain bit fields.
	 */
	movq	$idt, %rax
	movq	$0x00008e0000180000, %rcx
1:
	movq	(%rax), %rdi
	movq	%rdi, %rsi
	movq	%rdi, %rdx
	andl	$0xffff0000, %esi
	shlq	$32, %rsi
	andq	$0x0000ffff, %rdx
	orq	%rdx, %rsi
	orq	%rcx, %rsi
	movq	%rsi, (%rax)
	shrq	$32, %rdi
	movq	%rdi, 8(%rax)
	addq	$16, %rax
	cmpq	$idt_end, %rax
	jne	1b

	/* Install the IDT. */
	lidt	idt_ptr

	ret
