/* SPDX-License-Identifier: GPL-2.0-only */

void call_proc_xbus_enable_ridi(void);
void p9_chiplet_scominit(void);
void p9_cxa_scom(void);
void p9_nx_scom(void);
void p9_vas_scom(void);
void p9_fbc_ioo_dl_scom(void);

#define CHIP_EC (0x20)
#define SECURE_MEMORY (CHIP_EC >= 0x22)

#define LEN(a) (sizeof(a)/sizeof(*a))
uint64_t obus_chiplets[] = {OB0_CHIPLET_ID, OB3_CHIPLET_ID}

#define PU_PB_CENT_SM0_PB_CENT_FIR_REG (0x0000000005011C00)
#define PU_PB_CENT_SM0_PB_CENT_FIR_MASK_REG_SPARE_13 PPC_BIT(13)

#define XBUS_0_LL0_LL0_LL0_IOEL_FIR_MASK_REG (0x06011803)
#define XBUS_FIR_MASK_REG (0x06010C03)
#define PU_PB_IOE_FIR_MASK_REG_OR (0x05013405)
#define PU_PB_CENT_SM1_EXTFIR_MASK_REG_OR (0x05011C33)
#define XBUS_1_LL1_LL1_LL1_IOEL_FIR_MASK_REG (0x06011C03)
#define XBUS_1_FIR_MASK_REG (0x06011003)
#define XBUS_2_LL2_LL2_LL2_IOEL_FIR_MASK_REG (0x06012003)
#define XBUS_2_FIR_MASK_REG (0x06011403)
#define PU_IOE_PB_IOO_FIR_ACTION0_REG (0x05013806)
#define PU_IOE_PB_IOO_FIR_ACTION1_REG (0x05013807)
#define PU_IOE_PB_IOO_FIR_MASK_REG (0x05013803)
#define OBUS_LL0_PB_IOOL_FIR_ACTION0_REG (0x09010806)
#define OBUS_LL0_PB_IOOL_FIR_ACTION1_REG (0x09010807)
#define OBUS_LL0_LL0_LL0_PB_IOOL_FIR_MASK_REG (0x09010803)
#define PU_NMMU_MM_EPSILON_COUNTER_VALUE_REG (0x05012C1D)

#define FBC_IOE_TL_FIR_MASK_X2_NF (0x000300C0C0000220ULL)
#define FBC_EXT_FIR_MASK_X2_NF (0x2000000000000000ULL)
#define FBC_IOE_TL_FIR_MASK_X1_NF (0x0018030300000440ULL)
#define FBC_EXT_FIR_MASK_X1_NF (0x4000000000000000ULL)
#define FBC_IOE_DL_FIR_MASK_NF (0xFFFFFFFFFFFFFFFFULL)
#define XBUS_PHY_FIR_MASK_NF (0xFFFFFFFFFFFFC000ULL)
#define FBC_IOE_TL_FIR_MASK_X0_NF (0x00C00C0C00000880ULL)
#define FBC_EXT_FIR_MASK_X0_NF (0x8000000000000000ULL)
#define FBC_IOO_TL_FIR_ACTION0 (0x0000000000000000ULL)
#define FBC_IOO_TL_FIR_ACTION1 (0x0049200000000000ULL)
#define FBC_IOO_TL_FIR_MASK (0xFF2490000FFFF00FULL)
#define FBC_IOO_DL_FIR_ACTION0 (0x0000000000000000ULL)
#define FBC_IOO_DL_FIR_ACTION1 (0xFFFFFFFFFFFFFFFCULL)
#define FBC_IOO_DL_FIR_MASK (0xFCFC3FFFFCC00003ULL)
