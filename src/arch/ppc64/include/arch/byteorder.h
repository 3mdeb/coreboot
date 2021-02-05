/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _BYTEORDER_H
#define _BYTEORDER_H

#define __BIG_ENDIAN 4321

#define PPC_BIT(bit)		(0x8000000000000000UL >> (bit))
#define PPC_BITMASK(bs,be)	((PPC_BIT(bs) - PPC_BIT(be)) | PPC_BIT(bs))

#endif /* _BYTEORDER_H */
