/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SOC_IBM_POWER9_RS4_H
#define __SOC_IBM_POWER9_RS4_H

#include <stdint.h>

#define RS4_MAGIC (uint16_t)0x5253 // "RS"

/* Header of an RS4 compressed ring */
struct ring_hdr {
	uint16_t magic;		// Always "RS"
	uint8_t  version;
	uint8_t  type;
	uint16_t size;		// Header + data size in BE
	uint16_t ring_id;
	uint32_t scan_addr;
	uint8_t	 data[];
} __attribute__((packed));

#endif // __SOC_IBM_POWER9_RS4_H
