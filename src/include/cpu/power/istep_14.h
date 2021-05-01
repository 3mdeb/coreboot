#ifndef ISTEP_14_H
#define ISTEP_14_H

#include <cpu/power/scom.h>

#define MCA_MBA_FARB3Q (0x7010916)
#define MCS_MCSYNC (0x5010815)
#define MCS_MCMODE0 (0x5010811)
#define MCS_MCMODE0_ENABLE_EMER_THROTTLE (21)
#define MCS_MCSYNC_SYNC_GO_CH0 (16)
#define MCS_MCSYNC_CHANNEL_SELECT (0)
#define MCS_MCSYNC_CHANNEL_SELECT_LEN (8)
#define MCS_MCSYNC_SYNC_TYPE (8)
#define MCS_MCSYNC_SYNC_TYPE_LEN (8)
#define SUPER_SYNC_BIT (14)
#define MCS_MCSYNC_SYNC_GO_CH0 (16)
#define MBA_REFRESH_SYNC_BIT (8)
#define MAX_MC_SIDES_PER_PROC (2)
#define MCS_MCMODE0_DISABLE_MC_SYNC (27)
#define MCS_MCMODE0_DISABLE_MC_PAIR_SYNC (28)
#define PHB_CERR_RPT0_REG (0x4010c4a)
#define PHB_CERR_RPT1_REG (0x4010c4b)
#define PHB_NFIR_REG (0x4010c40)
#define PHB_NFIRWOF_REG (0x4010c48)
#define PHB_NFIRACTION0_REG (0x4010c46)
#define PCI_NFIR_ACTION0_REG (0x5b0f81e000000000)
#define PHB_NFIRACTION1_REG (0x4010c47)
#define PCI_NFIR_ACTION1_REG (0x7f0f81e000000000)
#define PHB_NFIRMASK_REG (0x4010c43)
#define PCI_NFIR_MASK_REG (0x30001c00000000)
#define PHB_PE_DFREEZE_REG (0x4010c55)
#define PHB_PBAIB_CERR_RPT_REG (0xd01084b)
#define PHB_PFIR_REG (0xd010840)
#define PHB_PFIRWOF_REG (0xd010848)
#define PHB_PFIRACTION0_REG (0xd010846)
#define PCI_PFIR_ACTION0_REG (0xb000000000000000)
#define PHB_PFIRACTION1_REG (0xd010847)
#define PCI_PFIR_ACTION1_REG (0xb000000000000000)
#define PHB_PFIRMASK_REG (0xd010843)
#define PCI_PFIR_MASK_REG (0xe00000000000000)
#define PHB_MMIOBAR0_REG (0x4010c4e)
#define PHB_MMIOBAR0_MASK_REG (0x4010c4f)
#define PHB_MMIOBAR1_REG (0x4010c50)
#define P9_PCIE_CONFIG_BAR_SHIFT (8)
#define PHB_PHBBAR_REG (0x4010c52)
#define PHB_BARE_REG (0x4010c54)
#define PHB_PHBRESET_REG (0xd01084a)
#define PHB_ACT0_REG (0xd01090e)
#define PHB_ACTION1_REG (0xd01090f)
#define PHB_MASK_REG (0xd01090b)
#define MAX_INTERLEAVE_GROUP_SIZE (0x40000000000)
#define PEC_PBCQHWCFG_REG_PE_DISABLE_OOO_MODE (0x16)
#define PEC_NESTTRC_REG (0x4010c03)
#define PEC_NESTTRC_REG_TRACE_MUX_SEL_A (0)
#define PEC_NESTTRC_REG_TRACE_MUX_SEL_A_LEN (4)
#define PEC_PBCQ_NESTTRC_SEL_A (9)
#define PEC_PBAIBHWCFG_REG_PE_OSMB_HOL_BLK_CNT (0x28)
#define PEC_PBAIBHWCFG_REG_PE_OSMB_HOL_BLK_CNT_LEN (3)
#define PEC_AIB_HWCFG_OSBM_HOL_BLK_CNT (7)
#define PEC_PBAIBHWCFG_REG (0xd010800)
#define FABRIC_ADDR_MSEL_END_BIT (14)
#define FABRIC_ADDR_MSEL_START_BIT (13)
#define PEC_PBCQHWCFG_REG (0x4010C00)

// Not sure about this
#define ATTR_PROC_PCIE_BAR_ENABLE(index) (false)

void istep_14_2(void);
void istep_14_3(void)
#endif
