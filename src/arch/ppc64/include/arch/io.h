/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _ASM_IO_H
#define _ASM_IO_H

#include <stdint.h>

#define MMIO_GROUP0_CHIP0_LPC_BASE_ADDR 0x0006030000000000
#define LPC_ADDR_START 0xC0000000
#define LPCHC_IO_SPACE 0xD0010000
#define LPC_BASE_ADDR (MMIO_GROUP0_CHIP0_LPC_BASE_ADDR + LPCHC_IO_SPACE - LPC_ADDR_START)

static inline void eieio()
{
    asm volatile("eieio" ::: "memory");
}

static inline void outb(uint8_t value, uint16_t port)
{
	uint8_t *l_ptr = (uint8_t *)(LPC_BASE_ADDR + port);
	const uint8_t *i_ptr = (const uint8_t *)&value;
	*l_ptr = *i_ptr;
	eieio();
}

static inline void outw(uint16_t value, uint16_t port)
{
	uint16_t *l_ptr = (uint16_t *)(LPC_BASE_ADDR + port);
	const uint16_t *i_ptr = (const uint16_t *)&value;
	*l_ptr = *i_ptr;
	eieio();
}

static inline void outl(uint32_t value, uint16_t port)
{
	uint32_t *l_ptr = (uint32_t *)(LPC_BASE_ADDR + port);
	const uint32_t *i_ptr = (const uint32_t *)&value;
	*l_ptr = *i_ptr;
	eieio();
}

static inline uint8_t inb(uint16_t port)
{
	uint8_t buffer;
	uint8_t *l_ptr = (uint8_t *)(LPC_BASE_ADDR + port);
	uint8_t *o_ptr = (uint8_t *)&buffer;
	*o_ptr = *l_ptr;
	eieio();
	return buffer;
}

static inline uint16_t inw(uint16_t port)
{
	uint16_t buffer;
	uint16_t *l_ptr = (uint16_t *)(LPC_BASE_ADDR + port);
	uint16_t *o_ptr = (uint16_t *)&buffer;
	*o_ptr = *l_ptr;
	eieio();
	return buffer;
}

static inline uint32_t inl(uint16_t port)
{
	uint32_t buffer;
	uint32_t *l_ptr = (uint32_t *)(LPC_BASE_ADDR + port);
	uint32_t *o_ptr = (uint32_t *)&buffer;
	*o_ptr = *l_ptr;
	eieio();
	return buffer;
}

#endif
