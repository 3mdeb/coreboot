/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/scom.h>
#include <cpu/power/occ.h>
#include <timer.h>


static void pm_ocb_setup(uint32_t ocb_bar)
{
	write_scom(OCBCSRn_OR[0], PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_MODE));
	write_scom(OCBCSRn_CLEAR[0], PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_TYPE));
	write_scom(OCBARn[0], (uint64_t)ocb_bar << 32);
}

static void put_ocb_indirect(uint32_t ocb_req_length, uint32_t oci_address,
			     uint64_t *ocb_buffer)
{
	write_scom(PU_OCB_PIB_OCBAR0, (uint64_t)oci_address << 32);
	uint64_t ocb_pib = read_scom(PU_OCB_PIB_OCBCSR0_RO);
	if ((ocb_pib & OCB_PIB_OCBCSR0_OCB_STREAM_MODE) &&
	    (ocb_pib & OCB_PIB_OCBCSR0_OCB_STREAM_TYPE)) {
		uint64_t stream_push_control = read_scom(PU_OCB_OCI_OCBSHCS0_SCOM);
		if (stream_push_control & OCB_OCI_OCBSHCS0_PUSH_ENABLE)
			for (uint8_t counter = 0; counter < 4; counter++) {
				if (!(stream_push_control & OCB_OCI_OCBSHCS0_PUSH_FULL))
					break;
				// Hostboot has delay of 0 here
				wait_us(1, false);
				stream_push_control = read_scom(PU_OCB_OCI_OCBSHCS0_SCOM);
			}
	}
	for (uint32_t index = 0; index < ocb_req_length; index++)
		write_scom(PU_OCB_PIB_OCBDR0, ocb_buffer[index]);
}

static void get_ocb_indirect(uint32_t ocb_req_length, uint32_t oci_address,
			     uint64_t *ocb_buffer)
{
	write_scom(PU_OCB_PIB_OCBAR0, (uint64_t)oci_address << 32);
	for (uint32_t loopCount = 0; loopCount < ocb_req_length; loopCount++)
		ocb_buffer[loopCount] = read_scom(PU_OCB_PIB_OCBDR0);
}

void writeOCCSRAM(uint32_t address, uint64_t * buffer, size_t data_length)
{
	pm_ocb_setup(address);
	put_ocb_indirect(data_length / 8, address, buffer);
}

void readOCCSRAM(uint32_t address, uint64_t * buffer, size_t data_length)
{
	pm_ocb_setup(address);
	get_ocb_indirect(data_length / 8, address, buffer);
}

void clear_occ_special_wakeups(void)
{
	for (size_t chiplet_index = 0; chiplet_index < NUMBER_OF_EX_CHIPLETS; ++chiplet_index)
		scom_and_for_chiplet(EX_CHIPLETS[chiplet_index], EX_PPM_SPWKUP_OCC,
				     ~PPC_BIT(0));
}
