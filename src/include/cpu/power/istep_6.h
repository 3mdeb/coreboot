/* SPDX-License-Identifier: GPL-2.0-only */

#include <commonlib/region.h>
#include <cpu/power/scom.h>
#include <types.h>

void istep_6_11(void);

#define FREQ_PB_MHZ (1866)

#define MASTER_PROC (1)

#define OCC_FW0                     (0)
#define OCC_FW1                     (1)
#define CME_ERR_NOTIFY              (2)
#define STOP_RCV_NOTIFY_PRD         (3)
#define OCC_HB_NOTIFY               (4)
#define GPE0_WD_TIMEOUT             (5)
#define GPE1_WD_TIMEOUT             (6)
#define GPE2_WD_TIMEOUT             (7)
#define GPE3_WD_TIMEOUT             (8)
#define GPE0_ERR                    (9)
#define GPE1_ERR                    (10)
#define GPE2_ERR                    (11)
#define GPE3_ERR                    (12)
#define OCB_ERR                     (13)
#define SRAM_UE                     (14)
#define SRAM_CE                     (15)
#define SRAM_READ_ERR               (16)
#define SRAM_WRITE_ERR              (17)
#define SRAM_DATAOUT_PERR           (18)
#define SRAM_OCI_WDATA_PARITY       (19)
#define SRAM_OCI_BE_PARITY_ERR      (20)
#define SRAM_OCI_ADDR_PARITY_ERR    (21)
#define GPE0_HALTED                 (22)
#define GPE1_HALTED                 (23)
#define GPE2_HALTED                 (24)
#define GPE3_HALTED                 (25)
#define EXT_TRAP                    (26)
#define PPC405_CORE_RESET           (27)
#define PPC405_CHIP_RESET           (28)
#define PPC405_SYS_RESET            (29)
#define PPC405_WAIT_STATE           (30)
#define PPC405_DBGSTOPACK           (31)
#define OCB_DB_OCI_TIMEOUT          (32)
#define OCB_DB_OCI_RDATA_PARITY     (33)
#define OCB_DB_OCI_SLVERR           (34)
#define OCB_PIB_ADDR_PARITY_ERR     (35)
#define OCB_DB_PIB_DATA_PARITY_ERR  (36)
#define OCB_IDC0_ERR                (37)
#define OCB_IDC1_ERR                (38)
#define OCB_IDC2_ERR                (39)
#define OCB_IDC3_ERR                (40)
#define SRT_FSM_ERR                 (41)
#define JTAGACC_ERR                 (42)
#define SPARE_ERR_38                (43)
#define C405_ECC_UE                 (44)
#define C405_ECC_CE                 (45)
#define C405_OCI_MC_CHK             (46)
#define SRAM_SPARE_DIRERR0          (47)
#define SRAM_SPARE_DIRERR1          (48)
#define SRAM_SPARE_DIRERR2          (49)
#define SRAM_SPARE_DIRERR3          (50)
#define GPE0_OCISLV_ERR             (51)
#define GPE1_OCISLV_ERR             (52)
#define GPE2_OCISLV_ERR             (53)
#define GPE3_OCISLV_ERR             (54)
#define C405ICU_M_TIMEOUT           (55)
#define C405DCU_M_TIMEOUT           (56)
#define OCC_CMPLX_FAULT             (57)
#define OCC_CMPLX_NOTIFY            (58)
#define SPARE_59                    (59)
#define SPARE_60                    (60)
#define SPARE_61                    (61)
#define FIR_PARITY_ERR_DUP          (62)
#define FIR_PARITY_ERR              (63)

#define OCB_OCI_OCBSHCS0_PUSH_FULL   (0)
#define OCB_OCI_OCBSHCS0_PUSH_ENABLE (31)

#define OCB_PIB_OCBCSR0_OCB_STREAM_MODE (4)
#define OCB_PIB_OCBCSR0_OCB_STREAM_TYPE (5)

#define PU_PBABAR0 (0x05012B00)
#define PU_PBABAR1 (0x05012B01)
#define PU_PBABAR2 (0x05012B02)
#define PU_PBABAR3 (0x05012B03)

#define PU_OCB_PIB_OCBCSR0_OR (0x0006D013)
#define PU_OCB_PIB_OCBCSR1_OR (0x0006D033)
#define PU_OCB_PIB_OCBCSR2_OR (0x0006D053)
#define PU_OCB_PIB_OCBCSR3_OR (0x0006D073)

#define PU_OCB_PIB_OCBAR0 (0x0006D010)
#define PU_OCB_PIB_OCBDR0 (0x0006D015)
#define PU_OCB_PIB_OCBCSR0_RO (0x0006D011)
#define PU_OCB_OCI_OCBSHCS0_SCOM (0x0006C204)

#define OCC_405_SRAM_ADDRESS (0xFFF40000)
#define OCC_OFFSET_MAIN_EP (0x6C)

#define OCB_OITR0 (0xc0060040)
#define OCB_OIEPR0 (0xc0060060)

#define OCC_BRANCH_INSTR (0x4B00000200000000)
#define BRANCH_ADDR_MASK (0x00FFFFFC)

#define OCC_OFFSET_LENGTH (0x48)
#define OCC_OFFSET_FREQ (0x94)
#define OCC_OFFSET_IPL_FLAG (0x92)
#define OCC_OFFSET_GPE0_LENGTH (0x64)
#define OCC_OFFSET_GPE1_LENGTH (0x68)
#define OCC_MODIFIED_SECTION_SIZE ((OCC_OFFSET_LENGTH) + (OCC_OFFSET_FREQ))
#define OCC_LENGTH (0x120000)

#define OCC_GPE0_SRAM_ADDRESS (0xFFF01000)
#define OCC_GPE1_SRAM_ADDRESS (0xFFF10000)

#define PU_SRAM_SRBV3_SCOM (0x0006A007)

#define PU_SPIMPSS_ADC_CTRL_REG0 (0x00070000)
#define PU_SPIPSS_ADC_CTRL_REG1 (0x00070001)
#define PU_SPIPSS_ADC_CTRL_REG2 (0x00070002)
#define PU_SPIPSS_ADC_WDATA_REG (0x00070010)

#define PU_SPIPSS_P2S_CTRL_REG0 (0x00070040)
#define PU_SPIPSS_P2S_CTRL_REG1 (0x00070041)
#define PU_SPIPSS_P2S_CTRL_REG2 (0x00070042)
#define PU_SPIPSS_P2S_WDATA_REG (0x00070050)

#define PU_SPIPSS_100NS_REG (0x00070028)


extern void mount_part_from_pnor(const char *part_name,
				 struct mmap_helper_region_device *mdev);

const uint64_t PBA_BARs[4] =
{
    PU_PBABAR0,
    PU_PBABAR1,
    PU_PBABAR2,
    PU_PBABAR3
};
