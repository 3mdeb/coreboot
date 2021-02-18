#include <stdint.h>

#define X0_ENABLED (true)
#define X1_ENABLED (true)
#define X2_ENABLED (true)
#define X0_IS_PAIRED (true)
#define X1_IS_PAIRED (true)
#define X2_IS_PAIRED (true)
#define DD2X_PARTS (true)
#define OPTICS_IS_A_BUS (true)
#define ATTR_PROC_FABRIC_A_LINKS_CNFG (0)
#define ATTR_PROC_FABRIC_X_LINKS_CNFG (0)

#define EPS_TYPE_LE (0x01),
#define EPS_TYPE_HE (0x02),
#define EPS_TYPE_HE_F8 (0x03)
#define ATTR_PROC_EPS_TABLE_TYPE (EPS_TYPE_LE) // default value

#define CHIP_IS_NODE (0x01)
#define CHIP_IS_GROUP (0x02)
#define ATTR_PROC_FABRIC_PUMP_MODE (CHIP_IS_NODE) // default value

#define FBC_IOE_TL_FIR_ACTION0 (0x0000000000000000ULL)
#define FBC_IOE_TL_FIR_ACTION1 (0x0049000000000000ULL)

#define FBC_IOE_TL_FIR_MASK (0xFF24F0303FFFF11FULL)
#define P9_FBC_UTILS_MAX_ELECTRICAL_LINKS (3)
#define FBC_IOE_TL_FIR_MASK_X0_NF (0x00C00C0C00000880ULL)
#define FBC_IOE_TL_FIR_MASK_X1_NF (0x0018030300000440ULL)
#define FBC_IOE_TL_FIR_MASK_X2_NF (0x000300C0C0000220ULL)
#define PU_PB_IOE_FIR_MASK_REG (0x5013403)

#define FBC_IOE_DL_FIR_ACTION0 (0)
#define FBC_IOE_DL_FIR_ACTION1 (0x303c00000001ffc)
#define FBC_IOE_DL_FIR_MASK (0xfcfc3fffffffe003)

#define BOTH (0x0)
#define EVEN_ONLY (0x1)
#define ODD_ONLY (0x2)
#define NONE (0x3)
#define ATTR_LINK_TRAIN BOTH

#define PROC_CHIPLET (0x10) // this is probably wrong
#define XBUS_PHY_FIR_ACTION0 (0x0000000000000000ULL)
#define XBUS_PHY_FIR_ACTION1 (0x2068680000000000ULL)
#define XBUS_PHY_FIR_MASK    (0xDF9797FFFFFFC000ULL)
#define XBUS_FIR_MASK_REG    (0x06010C0)
#define l_TGT0_ATTR_IO_XBUS_MASTER_MODE (0) // default value

void istep810(void);
void p9_xbus_g1_scom(uint64_t TGT0, uint64_t TGT1, uint64_t TGT2);
void p9_xbus_g0_scom(uint64_t TGT0, uint64_t TGT1, uint64_t TGT2);

void p9_fbc_ioe_tl_scom(void);
void istep89(void);
void p9_fbc_no_hp_scom(void);
void tl_fir(void);
void p9_fbc_ioe_dl_scom(uint64_t TGT0);
