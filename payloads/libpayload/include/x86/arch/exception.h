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

#ifndef _ARCH_EXCEPTION_H
#define _ARCH_EXCEPTION_H

#include <stdint.h>
#include <stddef.h>

void exception_init_asm(void);
void exception_dispatch(void);
void enable_interrupts(void);
void disable_interrupts(void);
/** Returns 1 if interrupts are enabled. */
int interrupts_enabled(void);

struct exception_state
{
	/* Careful: x86/gdb.c currently relies on the size and order of regs. */
#if CONFIG(LP_ARCH_X86_64)
	struct {
		size_t rax;
		size_t rcx;
		size_t rdx;
		size_t rbx;
		size_t rsp;
		size_t rbp;
		size_t rsi;
		size_t rdi;
		size_t r8;
		size_t r9;
		size_t r10;
		size_t r11;
		size_t r12;
		size_t r13;
		size_t r14;
		size_t r15;
		size_t rip;
		size_t rflags;
		size_t cs;
		size_t ss;
		size_t ds;
		size_t es;
		size_t fs;
		size_t gs;
	} regs;
#else
	struct {
		size_t eax;
		size_t ecx;
		size_t edx;
		size_t ebx;
		size_t esp;
		size_t ebp;
		size_t esi;
		size_t edi;
		size_t eip;
		size_t eflags;
		size_t cs;
		size_t ss;
		size_t ds;
		size_t es;
		size_t fs;
		size_t gs;
	} regs;
#endif
	size_t error_code;
	size_t vector;
} __packed;
extern struct exception_state *exception_state;

extern uintptr_t exception_stack[];
extern uintptr_t *exception_stack_end;

enum {
	EXC_DE = 0, /* Divide by zero */
	EXC_DB = 1, /* Debug */
	EXC_NMI = 2, /* Non maskable interrupt */
	EXC_BP = 3, /* Breakpoint */
	EXC_OF = 4, /* Overflow */
	EXC_BR = 5, /* Bound range */
	EXC_UD = 6, /* Invalid opcode */
	EXC_NM = 7, /* Device not available */
	EXC_DF = 8, /* Double fault */
	EXC_TS = 10, /* Invalid TSS */
	EXC_NP = 11, /* Segment not present */
	EXC_SS = 12, /* Stack */
	EXC_GP = 13, /* General protection */
	EXC_PF = 14, /* Page fault */
	EXC_MF = 16, /* x87 floating point */
	EXC_AC = 17, /* Alignment check */
	EXC_MC = 18, /* Machine check */
	EXC_XF = 19, /* SIMD floating point */
	EXC_SX = 30, /* Security */
	EXC_COUNT
};

#endif
