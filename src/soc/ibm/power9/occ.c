/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/scom.h>
#include <cpu/power/occ.h>
#include <timer.h>


static void pm_ocb_setup(const uint32_t i_ocb_bar)
{
    write_scom(OCBCSRn_OR[0], PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_MODE));
    write_scom(OCBCSRn_CLEAR[0], PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_TYPE));
    write_scom(OCBARn[0], (uint64_t)i_ocb_bar << 32);
}

static void put_ocb_indirect(
    const uint32_t i_ocb_req_length,
    const uint32_t i_oci_address,
    uint64_t* io_ocb_buffer)
{
    write_scom(PU_OCB_PIB_OCBAR0, (uint64_t)i_oci_address << 32);
    uint64_t ocb_pib = read_scom(PU_OCB_PIB_OCBCSR0_RO);
    if((ocb_pib & OCB_PIB_OCBCSR0_OCB_STREAM_MODE)
    && (ocb_pib & OCB_PIB_OCBCSR0_OCB_STREAM_TYPE))
    {
        uint64_t stream_push_control = read_scom(PU_OCB_OCI_OCBSHCS0_SCOM);
        if (stream_push_control & OCB_OCI_OCBSHCS0_PUSH_ENABLE)
        for(uint8_t l_counter = 0; l_counter < 4; l_counter++)
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
        write_scom(PU_OCB_PIB_OCBDR0, io_ocb_buffer[l_index]);
    }
}

static void get_ocb_indirect(
    const uint32_t i_ocb_req_length,
    const uint32_t i_oci_address,
    uint64_t* io_ocb_buffer)
{
    write_scom(PU_OCB_PIB_OCBAR0, (uint64_t)i_oci_address << 32);
    for(uint32_t l_loopCount = 0; l_loopCount < i_ocb_req_length; l_loopCount++)
    {
        io_ocb_buffer[l_loopCount] = read_scom(PU_OCB_PIB_OCBDR0);
    }
}

void writeOCCSRAM(
    const uint32_t address,
    uint64_t * buffer,
    size_t data_length)
{
    pm_ocb_setup(address);
    put_ocb_indirect(
        data_length / 8,
        address,
        buffer);
}

void readOCCSRAM(
    const uint32_t address,
    uint64_t * buffer,
    size_t data_length)
{
    pm_ocb_setup(address);
    get_ocb_indirect(
        data_length / 8,
        address,
        buffer);
}

uint64_t makeStart405Instruction(void)
{
    uint64_t l_epAddr;
    readOCCSRAM(
        OCC_405_SRAM_ADDRESS + OCC_OFFSET_MAIN_EP,
        &l_epAddr,
        8);

    // The branch instruction is of the form 0x4BXXXXX200000000, where X
    // is the address of the 405 main's entry point (alligned as shown).
    // Example: If 405 main's EP is FFF5B570, then the branch instruction
    // will be 0x4bf5b57200000000. The last two bits of the first byte of
    // the branch instruction must be '2' according to the OCC instruction
    // set manual.
    return OCC_BRANCH_INSTR | (((uint64_t)(BRANCH_ADDR_MASK & l_epAddr)) << 32);
}

void clear_occ_special_wakeups(void)
{
    for(size_t chiplet_index = 0;
        chiplet_index < NUMBER_OF_EX_CHIPLETS;
        ++chiplet_index)
    {
        write_scom_for_chiplet(
            EX_CHIPLETS[chiplet_index],
            EX_PPM_SPWKUP_OCC,
            read_scom_for_chiplet(EX_CHIPLETS[chiplet_index],
            EX_PPM_SPWKUP_OCC) & ~PPC_BIT(0));
    }
}
