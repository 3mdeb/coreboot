/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _BYTEORDER_H
#define _BYTEORDER_H

#define __BIG_ENDIAN 4321

#define PPC_BIT(bit)		(0x8000000000000000UL >> (bit))
#define PPC_BITMASK(bs,be)	((PPC_BIT(bs) - PPC_BIT(be)) | PPC_BIT(bs))

#define PPC_PLACE(val, pos, len) \
	__builtin_choose_expr( \
		PPC_PLACE_GOOD_ARGS(val, pos, len), \
		PPC_PLACE_IMPL(val, pos, len), \
		/* INCORRECT ARGUMENTS DETECTED */(void)0)

#define PPC_PLACE_GOOD_ARGS(val, pos, len) ( \
	/* pos value */ \
	__builtin_choose_expr( \
		__builtin_constant_p(pos), \
		((pos) >= 0) && ((pos) <= 63), \
		1) && \
	/* len value */ \
	__builtin_choose_expr( \
		__builtin_constant_p(len), \
		((len) >= 1) && ((len) <= 64), \
		1) && \
	/* range */ \
	__builtin_choose_expr( \
		__builtin_constant_p(pos) && __builtin_constant_p(len), \
		(pos) + (len) <= 64, \
		1) && \
	/* value */ \
	__builtin_choose_expr( \
		__builtin_constant_p(val) && __builtin_constant_p(len), \
		((val) & ~(((uint64_t)1 << (len)) - 1)) == 0, \
		1) \
	)

#define PPC_PLACE_IMPL(val, pos, len) \
	PPC_SHIFT((val) & (((uint64_t)1 << (len)) - 1), ((pos) + ((len) - 1)))

#ifndef __ASSEMBLER__

#include <types.h>
#define PPC_SHIFT(val, lsb)	(((uint64_t)(val)) << (63 - (lsb)))

#else
#define PPC_SHIFT(val, lsb)	((val) << (63 - (lsb)))
#endif

#endif /* _BYTEORDER_H */
