/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>

#define XBUS_PHY_FIR_ACTION0 (0x0000000000000000ULL)
#define XBUS_PHY_FIR_ACTION1 (0x2068680000000000ULL)
#define XBUS_PHY_FIR_MASK    (0xDF9797FFFFFFC000ULL)

#define ATTR_PROC_FABRIC_X_LINKS_CNFG (1)

#define FBC_IOE_TL_FIR_ACTION0 (0x0000000000000000ULL)
#define FBC_IOE_TL_FIR_ACTION1 (0x0049000000000000ULL)

#define FBC_IOE_TL_FIR_MASK       (0xFF24F0303FFFF11FULL)
#define FBC_IOE_TL_FIR_MASK_X0_NF (0x00C00C0C00000880ULL)

#define FBC_IOE_DL_FIR_ACTION0 (0)
#define FBC_IOE_DL_FIR_ACTION1 (0x303c00000001ffc)
#define FBC_IOE_DL_FIR_MASK    (0xfcfc3fffffffe003)

// Power Bus PB West Mode Configuration Register
#define PB_WEST_MODE_CFG_REG                (0x501180A)
// Power Bus PB CENT FIR Register
#define PB_CENT_SM0_PB_CENT_FIR_REG         (0x5011C00)
// Power Bus PB CENT Mode Register
#define PB_CENT_MODE_CFG_REG                (0x5011C0A)
// Power Bus PB CENT GP command RATE DP0 Register
#define PB_CENT_GP_COMMAND_RATE_DP0_REG     (0x5011C26)
// Power Bus PB CENT GP command RATE DP1 Register
#define PB_CENT_GP_COMMAND_RATE_DP1_REG     (0x5011C27)
// Power Bus PB CENT RGP command RATE DP0 Register
#define PB_CENT_RGP_COMMAND_RATE_DP0_REG    (0x5011C28)
// Power Bus PB CENT RGP command RATE DP1 Register
#define PB_CENT_RGP_COMMAND_RATE_DP1_REG    (0x5011C29)
// Power Bus PB CENT SP command RATE DP0 Register
#define PB_CENT_SP_COMMAND_RATE_DP0_REG     (0x5011C2A)
// Power Bus PB CENT SP command RATE DP1 Register
#define PB_CENT_SP_COMMAND_RATE_DP1_REG     (0x5011C2B)
// Power Bus PB East Mode Configuration Register
#define PB_EAST_MODE_CFG_REG                (0x501200A)
// Power Bus PBEN IOX Domain FIR0 Mask Register
#define PU_PB_IOE_FIR_MASK_REG              (0x5013403)
// Power Bus PBEN IOX Domain FIR Action 0 Register
#define PU_PB_IOE_FIR_ACTION0_REG           (0x5013406)
// Power Bus PBEN IOX Domain FIR Action 1 Register
#define PU_PB_IOE_FIR_ACTION1_REG           (0x5013407)
// Power bus Electrical Framer/Parser 01 Configuration Register
#define PB_ELE_PB_FRAMER_PARSER_01_CFG_REG  (0x501340A)
// Power Bus Electrical Framer/Parser 23 Configuration Register
#define PB_ELE_PB_FRAMER_PARSER_23_CFG_REG  (0x501340B)
// Power Bus Electrical Framer/Parser 45 Configuration Register
#define PB_ELE_PB_FRAMER_PARSER_45_CFG_REG  (0x501340C)
// Power Bus Electrical Link Data Buffer 01 Configuration Register
#define PB_ELE_PB_DATA_BUFF_01_CFG_REG      (0x5013410)
// Power Bus Electrical Link Data Buffer 23 Configuration Register
#define PB_ELE_PB_DATA_BUFF_23_CFG_REG      (0x5013411)
// Power Bus Electrical Link Data Buffer 45 Configuration Register
#define PB_ELE_PB_DATA_BUFF_45_CFG_REG      (0x5013412)
// Power Bus Electrical Miscellaneous Configuration Register
#define PB_ELE_MISC_CFG_REG                 (0x5013423)
// Power Bus Electrical Link Trace Configuration Register
#define PB_ELE_LINK_TRACE_CFG_REG           (0x5013424)
// ELL FIR Mask Register
#define XBUS_LL0_IOEL_FIR_MASK_REG          (0x6011803)
// ELL FIR Action 0 Register
#define XBUS_LL0_IOEL_FIR_ACTION0_REG       (0x6011806)
// ELL FIR Action 1 Register
#define XBUS_LL0_IOEL_FIR_ACTION1_REG       (0x6011807)
// Power Bus ELL Configuration Register
#define PB_ELL_CFG_REG                      (0x601180A)
// Power Bus ELL Replay Threshold Register
#define PB_ELL_REPLAY_TRESHOLD_REG          (0x6011818)
// Power Bus ELL SL ECC Threshold Register
#define PB_ELL_SL_ECC_TRESHOLD_REG          (0x6011819)
#define XBUS_FIR_MASK_REG                   (0x6010C03)
#define XBUS_FIR_ACTION0_REG                (0x6010C06)
#define XBUS_FIR_ACTION1_REG                (0x6010C07)

void istep_8_10(void);
void p9_io_xbus_scominit(const uint8_t);
void p9_xbus_g0_scom(void);
void p9_xbus_g1_scom(void);

void istep_8_9(void);
void p9_fbc_ioe_dl_scom(void);
void p9_fbc_ioe_tl_scom(void);
void p9_fbc_no_hp_scom(void);
void ioe_tl_fir(void);
void ioel_fir(void);


