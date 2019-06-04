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

#include <arch/exception.h>
#include <exception.h>
#include <libpayload.h>
#include <stdint.h>
#include <arch/apic.h>

#define IF_FLAG				(1 << 9)

u64 exception_stack[0x400] __attribute__((aligned(8)));

static interrupt_handler handlers[256];

static const char *names[EXC_COUNT] = {
	[EXC_DE]  = "Divide by Zero",
	[EXC_DB]  = "Debug",
	[EXC_NMI] = "Non-Maskable-Interrupt",
	[EXC_BP]  = "Breakpoint",
	[EXC_OF]  = "Overflow",
	[EXC_BR]  = "Bound Range",
	[EXC_UD]  = "Invalid Opcode",
	[EXC_NM]  = "Device Not Available",
	[EXC_DF]  = "Double Fault",
	[EXC_TS]  = "Invalid TSS",
	[EXC_NP]  = "Segment Not Present",
	[EXC_SS]  = "Stack Fault",
	[EXC_GP]  = "General Protection Fault",
	[EXC_PF]  = "Page Fault",
	[EXC_MF]  = "x87 Floating Point",
	[EXC_AC]  = "Alignment Check",
	[EXC_MC]  = "Machine Check",
	[EXC_XF]  = "SIMD Floating Point",
	[EXC_SX]  = "Security",
};

static void print_segment_error_code(u64 code)
{
	printf("%#llx - descriptor %#llx in the ", code, (code >> 3) & 0x1FFF);
	if (code & (0x1 << 1)) {
		printf("IDT");
	} else {
		if (code & 0x04)
			printf("LDT");
		else
			printf("GDT");
	}
	if (code & (0x1 << 0))
		printf(", external to the CPU");
	else
		printf(", internal to the CPU");
}

static void print_page_fault_error_code(u64 code)
{
	printf("%#llx -", code);
	if (code & (0x1 << 0))
		printf(" page protection");
	else
		printf(" page not present");
	if (code & (0x1 << 1))
		printf(", write");
	else
		printf(", read");
	if (code & (0x1 << 2))
		printf(", user");
	else
		printf(", supervisor");
	if (code & (0x1 << 3))
		printf(", reserved bits set");
	if (code & (0x1 << 4))
		printf(", instruction fetch");
}

static void print_raw_error_code(u64 code)
{
	printf("%#llx", code);
}

static void dump_stack(uintptr_t addr, size_t bytes)
{
	int i, j;
	const int line = 4;
	u64 *ptr = (u64 *)(addr & ~(line * sizeof(*ptr) - 1));

	printf("Dumping stack:\n");
	for (i = bytes / sizeof(*ptr); i >= 0; i -= line) {
		printf("%p: ", ptr + i);
		for (j = i; j < i + line; j++)
			printf("%016llx ", *(ptr + j));
		printf("\n");
	}
}

static void dump_exception_state(void)
{
	printf("%s Exception\n", names[exception_state->vector]);

	printf("Error code: ");
	switch (exception_state->vector) {
	case EXC_PF:
		print_page_fault_error_code(exception_state->error_code);
		break;
	case EXC_TS:
	case EXC_NP:
	case EXC_SS:
	case EXC_GP:
		print_segment_error_code(exception_state->error_code);
		break;
	case EXC_DF:
	case EXC_AC:
	case EXC_SX:
		print_raw_error_code(exception_state->error_code);
		break;
	default:
		printf("n/a");
		break;
	}
	printf("\n");
	printf("RIP:    0x%016llx\n", exception_state->regs.rip);
	printf("CS:     0x%04llx\n",  exception_state->regs.cs);
	printf("RFLAGS: 0x%016llx\n", exception_state->regs.rflags);
	printf("RAX:    0x%016llx\n", exception_state->regs.rax);
	printf("RCX:    0x%016llx\n", exception_state->regs.rcx);
	printf("RDX:    0x%016llx\n", exception_state->regs.rdx);
	printf("RBX:    0x%016llx\n", exception_state->regs.rbx);
	printf("RSP:    0x%016llx\n", exception_state->regs.rsp);
	printf("RBP:    0x%016llx\n", exception_state->regs.rbp);
	printf("RSI:    0x%016llx\n", exception_state->regs.rsi);
	printf("RDI:    0x%016llx\n", exception_state->regs.rdi);
	printf("R8:     0x%016llx\n", exception_state->regs.r8);
	printf("R9:     0x%016llx\n", exception_state->regs.r9);
	printf("R10:    0x%016llx\n", exception_state->regs.r10);
	printf("R11:    0x%016llx\n", exception_state->regs.r11);
	printf("R12:    0x%016llx\n", exception_state->regs.r12);
	printf("R13:    0x%016llx\n", exception_state->regs.r13);
	printf("R14:    0x%016llx\n", exception_state->regs.r14);
	printf("R15:    0x%016llx\n", exception_state->regs.r15);
	printf("DS:     0x%04llx\n",  exception_state->regs.ds);
	printf("ES:     0x%04llx\n",  exception_state->regs.es);
	printf("SS:     0x%04llx\n",  exception_state->regs.ss);
	printf("FS:     0x%04llx\n",  exception_state->regs.fs);
	printf("GS:     0x%04llx\n",  exception_state->regs.gs);
}

void exception_dispatch(void)
{
	die_if(exception_state->vector >= ARRAY_SIZE(handlers),
	       "Invalid vector %llu\n", exception_state->vector);

	u8 vec = exception_state->vector;

	if (handlers[vec]) {
		handlers[vec](vec);
		goto success;
	} else if (vec >= EXC_COUNT
		   && CONFIG(LP_IGNORE_UNKNOWN_INTERRUPTS)) {
		goto success;
	} else if (vec >= EXC_COUNT
		   && CONFIG(LP_LOG_UNKNOWN_INTERRUPTS)) {
		printf("Ignoring interrupt vector %u\n", vec);
		goto success;
	}

	die_if(vec >= EXC_COUNT || !names[vec], "Bad exception vector %u\n",
	       vec);

	dump_exception_state();
	dump_stack(exception_state->regs.rsp, 512);
	/* We don't call apic_eoi because we don't want to ack the interrupt and
	   allow another interrupt to wake the processor. */
	halt();
	return;

success:
	if (CONFIG(LP_ENABLE_APIC))
		apic_eoi(vec);
}

void exception_init(void)
{
	exception_stack_end = exception_stack + ARRAY_SIZE(exception_stack);
	exception_init_asm();
}

void set_interrupt_handler(u8 vector, interrupt_handler handler)
{
	handlers[vector] = handler;
}

static u64 rflags(void)
{
	u64 rflags;
	asm volatile(
		"pushf\n\t"
		"pop %0\n\t"
	: "=rm" (rflags));
	return rflags;
}

void enable_interrupts(void)
{
	asm volatile (
		"sti\n"
		: : : "cc"
	);
}
void disable_interrupts(void)
{
	asm volatile (
		"cli\n"
		: : : "cc"
	);
}

int interrupts_enabled(void)
{
	return !!(rflags() & IF_FLAG);
}
