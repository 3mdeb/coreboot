/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _ASM_IO_H
#define _ASM_IO_H

#include <stdint.h>

#define MMIO_GROUP0_CHIP0_LPC_BASE_ADDR 0x0006030000000000
#define LPC_ADDR_START 0xC0000000
#define LPCHC_IO_SPACE 0xD0010000
#define LPC_BASE_ADDR (MMIO_GROUP0_CHIP0_LPC_BASE_ADDR + LPCHC_IO_SPACE - LPC_ADDR_START)

/* Enforce In-order Execution of I/O */
static inline void eieio(void)
{
	asm volatile("eieio" ::: "memory");
}

static inline void outb(uint8_t value, uint16_t port)
{
	uint8_t *l_ptr = (uint8_t *)(LPC_BASE_ADDR + port);
	*l_ptr = value;
	eieio();
}

static inline void outw(uint16_t value, uint16_t port)
{
	uint16_t *l_ptr = (uint16_t *)(LPC_BASE_ADDR + port);
	*l_ptr = value;
	eieio();
}

static inline void outl(uint32_t value, uint16_t port)
{
	uint32_t *l_ptr = (uint32_t *)(LPC_BASE_ADDR + port);
	*l_ptr = value;
	eieio();
}

static inline uint8_t inb(uint16_t port)
{
	uint8_t buffer;
	uint8_t *l_ptr = (uint8_t *)(LPC_BASE_ADDR + port);
	buffer = *l_ptr;
	eieio();
	return buffer;
}

static inline uint16_t inw(uint16_t port)
{
	uint16_t buffer;
	uint16_t *l_ptr = (uint16_t *)(LPC_BASE_ADDR + port);
	buffer = *l_ptr;
	eieio();
	return buffer;
}

static inline uint32_t inl(uint16_t port)
{
	uint32_t buffer;
	uint32_t *l_ptr = (uint32_t *)(LPC_BASE_ADDR + port);
	buffer = *l_ptr;
	eieio();
	return buffer;
}

#endif
