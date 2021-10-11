/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_OPS_H
#define CPU_PPC64_OPS_H

static const uint32_t ATTN_OP             = 0x00000200;
static const uint32_t BLR_OP              = 0x4E800020;
static const uint32_t BR_OP               = 0x48000000;
static const uint32_t BCCTR_OP            = 0x4C000000;
static const uint32_t ORI_OP              = 0x60000000;
static const uint32_t LIS_OP              = 0x3C000000;
static const uint32_t MTSPR_OP            = 0x7C000000;
static const uint32_t SKIP_SPR_REST_INST  = 0x4800001C;
static const uint32_t MR_R0_TO_R10_OP     = 0x7C0A0378;
static const uint32_t MR_R0_TO_R21_OP     = 0x7C150378;
static const uint32_t MR_R0_TO_R9_OP      = 0x7C090378;
static const uint32_t MTLR_R30_OP         = 0x7FC803A6;
static const uint32_t MFLR_R30_OP         = 0x7FC802A6;

#endif /* CPU_PPC64_OPS_H */
