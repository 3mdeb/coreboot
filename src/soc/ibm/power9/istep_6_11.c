/* SPDX-License-Identifier: GPL-2.0-only */

#include <timer.h>
#include <cpu/power/istep_6.h>

void istep_6_11()
{
    printk(BIOS_EMERG, "starting istep 14.1\n");
    report_istep(6, 11);
    printk(BIOS_EMERG, "ending istep 14.1\n");
}

static void startOCCFromSRAM(TARGETING::Target* i_proc)
{
    const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>master_proc_target(i_proc);

    // executed only for master processor!!!
    pm_pss_init(master_proc_target);

    pm_occ_fir_init(i_target);
    clear_occ_special_wakeups(master_proc_target);
    write_scom(PU_OCB_OCI_OIRR0A_SCOM, 0x218780f800000000);
    write_scom(PU_OCB_OCI_OIRR1A_SCOM, 0x0003d03c00000000);
    write_scom(PU_OCB_OCI_OIRR0B_SCOM, 0x2181801800000000);
    write_scom(PU_OCB_OCI_OIRR1B_SCOM, 0x0003d00c00000000);
    write_scom(PU_OCB_OCI_OIRR0C_SCOM, 0x010280ac00000000);
    write_scom(PU_OCB_OCI_OIRR1C_SCOM, 0x0001901400000000);

    uint64_t l_start405MainInstr = makeStart405Instruction();
    p9_pm_occ_control(
        master_proc_target,
        p9occ_ctrl::PPC405_START,
        p9occ_ctrl::PPC405_BOOT_WITHOUT_BL,
        l_start405MainInstr);

    write_scom(OCB_OITR0, 0xffffffffffffffff);
    write_scom(OCB_OIEPR0, 0xffffffffffffffff);
}

static void pm_pss_init(const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target)
{
    uint64_t l_data64;
    write_scom(
        PU_SPIMPSS_ADC_CTRL_REG0,
        (read_scom(PU_SPIMPSS_ADC_CTRL_REG0)
      & ~PPC_BITMASK(0, 11))
      | PPC_BIT(2));

    // TODO: src/import/chips/p9/procedures/hwp/pm/p9_pm_pss_init.C:201
    // 5 bits of l_hwctrl_nr_of_frames are written to 4 bit field??
    fapi2::putScom(
        i_target,
        PU_SPIPSS_ADC_CTRL_REG1,
        (read_scom(PU_SPIPSS_ADC_CTRL_REG1)
      & ~PPC_BITMASK(0, 17))
      | PPC_BIT(0) | PPC_BIT(11) | PPC_BIT(13););

    write_scom(
        PU_SPIPSS_ADC_CTRL_REG2,
        read_scom(PU_SPIPSS_ADC_CTRL_REG2) & ~PPC_BITMASK(0, 16));
    write_scom(
        PU_SPIPSS_ADC_WDATA_REG,
        0);
    write_scom(
        PU_SPIPSS_P2S_CTRL_REG0,
        (read_scom(PU_SPIPSS_P2S_CTRL_REG0)
      & ~PPC_BITMASK(0, 11))
      | PPC_BIT(2));
    write_scom(
        PU_SPIPSS_P2S_CTRL_REG1,
        read_scom(PU_SPIPSS_P2S_CTRL_REG1)
      & ~PPC_BITMASK(1, 3)
      | PPC_BIT(0)  | PPC_BIT(10) | PPC_BIT(12)
      | PPC_BIT(17));
    write_scom(
        PU_SPIPSS_P2S_CTRL_REG2,
        read_scom(PU_SPIPSS_P2S_CTRL_REG2) & ~PPC_BITMASK(0, 16));
    write_scom(
        PU_SPIPSS_P2S_WDATA_REG,
        0);
    write_scom(
        PU_SPIPSS_100NS_REG,
        (read_scom(PU_SPIPSS_100NS_REG) & 0xFFFFFFFF)
      | (FREQ_PB_MHZ / 40) << 32);
}

static void clear_occ_special_wakeups(const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target)
{
    // EX targets CHIPLET_IDs [0x10, 0x10, 0x11, 0x11, 0x12, 0x12, 0x13, 0x13, 0x14, 0x14]
    auto l_exChiplets = i_target.getChildren<fapi2::TARGET_TYPE_EX>(fapi2::TARGET_STATE_FUNCTIONAL);
    for (auto l_ex_chplt : l_exChiplets)
    {
        uint64_t data = read_scom_for_chiplet(l_ex_chplt, EX_PPM_SPWKUP_OCC);
        write_scom_for_chiplet(l_ex_chplt, EX_PPM_SPWKUP_OCC, data & ~PPC_BIT(0));
    }
}

#define OCB_OITR0 (0xc0060040)
#define OCB_OIEPR0 (0xc0060060)

#define OCC_BRANCH_INSTR (0x4B00000200000000)
#define BRANCH_ADDR_MASK (0x00FFFFFC)

static uint64_t makeStart405Instruction(void)
{
    uint64_t l_epAddr;
    readSRAM(
        OCC_405_SRAM_ADDRESS + OCC_OFFSET_MAIN_EP,
        &l_epAddr,
        8);

    // The branch instruction is of the form 0x4BXXXXX200000000, where X
    // is the address of the 405 main's entry point (alligned as shown).
    // Example: If 405 main's EP is FFF5B570, then the branch instruction
    // will be 0x4bf5b57200000000. The last two bits of the first byte of
    // the branch instruction must be '2' according to the OCC instruction
    // set manual.

    // OCC_BRANCH_INSTR = 0x4B00000200000000
    // BRANCH_ADDR_MASK = 0x00FFFFFC
    return OCC_BRANCH_INSTR | (((uint64_t)(BRANCH_ADDR_MASK & l_epAddr)) << 32);
}

inline uint64_t getMSR(void)
{
    uint64_t msr;
    asm volatile("mfmsr %0" : "=r" (msr));
    return msr;
}

inline void setMSR(uint64_t msr)
{
    asm volatile("mtmsr %0; isync" :: "r" (msr));
}

void setCheckstopData(void)
{
    setMSR(getMSR(void) | 0x1000);
}

static void writeSRAM(
    const uint32_t i_addr,
    uint64_t * i_dataBuf,
    size_t i_dataLen)
{
    pm_ocb_setup(i_addr);
    put_ocb_indirect(
        i_dataLen / 8,
        i_addr,
        i_dataBuf);
}

static void readSRAM(
    const uint32_t i_addr,
    uint64_t * io_dataBuf,
    size_t i_dataLen)
{
    pm_ocb_setup(i_addr);
    get_ocb_indirect(
        i_dataLen / 8,
        i_addr,
        io_dataBuf);
}

static void pm_occ_fir_reset(void)
{
    putScom(
        iv_fir_address + MASK_WOR_INCR,
        0xFFFFFFFFFFFFFFFF);
    putScom(
        iv_fir_address + MASK_WAND_INCR,
        0xFFFFFFFFFFFFFFFF & ~OCC_HB_NOTIFY);
    putScom(
        iv_action0_address,
        read_scom(iv_action0_address) | OCC_HB_NOTIFY);
    putScom(
        iv_action1_address,
        read_scom(iv_action1_address) & ~OCC_HB_NOTIFY);
}

static void pm_ocb_setup(const uint32_t i_ocb_bar)
{
    write_scom(OCBCSRn_OR[0], PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_MODE));
    write_scom(OCBCSRn_CLEAR[0], PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_TYPE));
    write_scom(OCBARn[0], i_ocb_bar << 32);
}

static void put_ocb_indirect(
    const uint32_t i_ocb_req_length,
    const uint32_t i_oci_address,
    uint64_t* io_ocb_buffer)
{
    write_scom(PU_OCB_PIB_OCBAR0, i_oci_address << 32);
    uint64_t ocb_pib = read_scom(PU_OCB_PIB_OCBCSR0_RO);
    if((ocb_pib & OCB_PIB_OCBCSR0_OCB_STREAM_MODE)
    && (ocb_pib & OCB_PIB_OCBCSR0_OCB_STREAM_TYPE))
    {
        uint64_t stream_push_control = read_scom(PU_OCB_OCI_OCBSHCS0_SCOM);
        if (stream_push_control & OCB_OCI_OCBSHCS0_PUSH_ENABLE)
        for(uint8_t l_counter = 0; l_counter < 4; l_counter++;)
        {
            if (!(stream_push_control & OCB_OCI_OCBSHCS0_PUSH_FULL))
            {
                break;
            }
            // Hostboot has delay of 0 here
            wait_us(1, false);
            stream_push_control = read_scom(PU_OCB_OCI_OCBSHCS0_SCOM);
        }
    }
    for(uint32_t l_index = 0; l_index < i_ocb_req_length; l_index++)
    {
        l_data64.insertFromRight(io_ocb_buffer[l_index], 0, 64);
        write_scom(PU_OCB_PIB_OCBDR0, l_data64);
    }
}

static void get_ocb_indirect(
    const uint32_t i_ocb_req_length,
    const uint32_t i_oci_address,
    uint64_t* io_ocb_buffer)
{
    write_scom(PU_OCB_PIB_OCBAR0, i_oci_address << 32);
    for(uint32_t l_loopCount = 0; l_loopCount < i_ocb_req_length; l_loopCount++)
    {
        io_ocb_buffer[l_loopCount] = read_scom(PU_OCB_PIB_OCBDR0);
    }
}

static void p9_pm_ocb_indir_access(
    const p9ocb::PM_OCB_ACCESS_OP i_ocb_op,
    const uint32_t                i_ocb_req_length,
    const uint32_t                i_oci_address,
    uint64_t*                     io_ocb_buffer)
{
    write_scom(PU_OCB_PIB_OCBAR0, i_oci_address << 32);
    if(i_ocb_op == p9ocb::OCB_PUT)
    {
        uint64_t ocb_pib = read_scom(PU_OCB_PIB_OCBCSR0_RO);
        if((ocb_pib & OCB_PIB_OCBCSR0_OCB_STREAM_MODE)
        && (ocb_pib & OCB_PIB_OCBCSR0_OCB_STREAM_TYPE))
        {
            uint64_t stream_push_control = read_scom(PU_OCB_OCI_OCBSHCS0_SCOM);
            if (stream_push_control & OCB_OCI_OCBSHCS0_PUSH_ENABLE)
            for(uint8_t l_counter = 0; l_counter < 4; l_counter++;)
            {
                if (!(stream_push_control & OCB_OCI_OCBSHCS0_PUSH_FULL))
                {
                    break;
                }
                // Hostboot has delay of 0 here
                wait_us(1, false);
                stream_push_control = read_scom(PU_OCB_OCI_OCBSHCS0_SCOM);
            }
        }
        for(uint32_t l_index = 0; l_index < i_ocb_req_length; l_index++)
        {
            l_data64.insertFromRight(io_ocb_buffer[l_index], 0, 64);
            write_scom(PU_OCB_PIB_OCBDR0, l_data64);
        }
    }
    else if(i_ocb_op == p9ocb::OCB_GET)
    {
        for(uint32_t l_loopCount = 0; l_loopCount < i_ocb_req_length; l_loopCount++)
        {
            io_ocb_buffer[l_loopCount] = read_scom(PU_OCB_PIB_OCBDR0);
        }
    }
}

static void pm_occ_fir_init(void)
{
    iv_mask = read_scom(iv_mask_address);

    write_scom(iv_proc, iv_fir_address, 0);
    write_scom(iv_proc, iv_action0_address, C405_ECC_UE);
    write_scom(
        iv_proc,
        iv_action1_address,
        C405_ECC_CE               | C405_OCI_MC_CHK
      | C405DCU_M_TIMEOUT         | GPE0_ERR
      | GPE0_OCISLV_ERR           | GPE1_ERR
      | GPE1_OCISLV_ERR           | GPE2_OCISLV_ERR
      | GPE3_OCISLV_ERR           | JTAGACC_ERR
      | OCB_DB_OCI_RDATA_PARITY   | OCB_DB_OCI_SLVERR
      | OCB_DB_OCI_TIMEOUT        | OCB_DB_PIB_DATA_PARITY_ERR
      | OCB_IDC0_ERR              | OCB_IDC1_ERR
      | OCB_IDC2_ERR              | OCB_IDC3_ERR
      | OCB_PIB_ADDR_PARITY_ERR   | OCC_CMPLX_FAULT
      | OCC_CMPLX_NOTIFY          | SRAM_CE
      | SRAM_DATAOUT_PERR         | SRAM_OCI_ADDR_PARITY_ERR
      | SRAM_OCI_BE_PARITY_ERR    | SRAM_OCI_WDATA_PARITY
      | SRAM_READ_ERR             | SRAM_SPARE_DIRERR0
      | SRAM_SPARE_DIRERR1        | SRAM_SPARE_DIRERR2
      | SRAM_SPARE_DIRERR3        | SRAM_UE
      | SRAM_WRITE_ERR            | SRT_FSM_ERR
      | STOP_RCV_NOTIFY_PRD);
    write_scom(
        iv_proc,
        iv_fir_address + MASK_WOR_INCR,
        iv_mask             | C405ICU_M_TIMEOUT
      | CME_ERR_NOTIFY      | EXT_TRAP
      | FIR_PARITY_ERR_DUP  | FIR_PARITY_ERR
      | GPE0_HALTED         | GPE0_WD_TIMEOUT
      | GPE1_HALTED         | GPE1_WD_TIMEOUT
      | GPE2_ERR            | GPE2_HALTED
      | GPE2_WD_TIMEOUT     | GPE3_ERR
      | GPE3_HALTED         | GPE3_WD_TIMEOUT
      | OCB_ERR             | OCC_FW0
      | OCC_FW1             | OCC_HB_NOTIFY
      | PPC405_CHIP_RESET   | PPC405_CORE_RESET
      | PPC405_DBGSTOPACK   | PPC405_SYS_RESET
      | PPC405_WAIT_STATE   | SPARE_59
      | SPARE_60            | SPARE_61
      | SPARE_ERR_38);
    write_scom(
        iv_proc,
        iv_fir_address + MASK_WAND_INCR,
        iv_mask                 & ~C405_ECC_CE
     & ~C405_ECC_UE             & ~C405_OCI_MC_CHK
     & ~C405DCU_M_TIMEOUT       & ~GPE0_ERR
     & ~GPE0_OCISLV_ERR         & ~GPE1_ERR
     & ~GPE1_OCISLV_ERR         & ~GPE2_OCISLV_ERR
     & ~GPE3_OCISLV_ERR         & ~JTAGACC_ERR
     & ~OCB_DB_OCI_RDATA_PARITY & ~OCB_DB_OCI_SLVERR
     & ~OCB_DB_OCI_TIMEOUT      & ~OCB_DB_PIB_DATA_PARITY_ERR
     & ~OCB_IDC0_ERR            & ~OCB_IDC1_ERR
     & ~OCB_IDC2_ERR            & ~OCB_IDC3_ERR
     & ~OCB_PIB_ADDR_PARITY_ERR & ~OCC_CMPLX_FAULT
     & ~OCC_CMPLX_NOTIFY        & ~SRAM_CE
     & ~SRAM_DATAOUT_PERR       & ~SRAM_OCI_ADDR_PARITY_ERR
     & ~SRAM_OCI_BE_PARITY_ERR  & ~SRAM_OCI_WDATA_PARITY
     & ~SRAM_READ_ERR           & ~SRAM_SPARE_DIRERR0
     & ~SRAM_SPARE_DIRERR1      & ~SRAM_SPARE_DIRERR2
     & ~SRAM_SPARE_DIRERR3      & ~SRAM_UE
     & ~SRAM_WRITE_ERR          & ~SRT_FSM_ERR
     & ~STOP_RCV_NOTIFY_PRD);
}

static void p9_pm_pba_bar_config(
    int bar_index,
    int bar_address)
{
    write_scom(PBA_BARs[bar_index], i_pba_bar_addr & 0x1FFFFFFFFFFFFFFF);
    write_scom(PBA_BARMSKs[bar_index], 0x300000);
}
