/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/scom.h>
#include <cpu/power/occ.h>
#include <timer.h>

#include "ops.h"

#define OCB_PIB_OCBCSR0_OCB_STREAM_MODE (4)
#define OCB_PIB_OCBCSR0_OCB_STREAM_TYPE (5)

#define OCB_OCI_OCBSHCS0_PUSH_ENABLE (31)
#define OCB_OCI_OCBSHCS0_PUSH_FULL   (0)

#define PU_OCB_PIB_OCBCSR0_RO (0x0006D011)
#define PU_OCB_PIB_OCBCSR1_RO (0x0006D031)

#define PU_OCB_OCI_OCBSHCS0_SCOM (0x0006C204)
#define PU_OCB_OCI_OCBSHCS1_SCOM (0x0006C214)

#define EX_PPM_SPWKUP_OCC (0x200F010C)
#define PU_OCB_PIB_OCBAR0 (0x0006D010)

#define PU_OCB_PIB_OCBDR0 (0x0006D015)
#define PU_OCB_PIB_OCBDR1 (0x0006D035)

#define PU_OCB_PIB_OCBCSR0_OR (0x0006D013)
#define PU_OCB_PIB_OCBCSR0_CLEAR (0x0006D012)

#define NUMBER_OF_EX_CHIPLETS (6)
static const chiplet_id_t EX_CHIPLETS[NUMBER_OF_EX_CHIPLETS] = {
	EP00_CHIPLET_ID,
	EP01_CHIPLET_ID,
	EP02_CHIPLET_ID,
	EP03_CHIPLET_ID,
	EP04_CHIPLET_ID,
	EP05_CHIPLET_ID
};

static void pm_ocb_setup(uint32_t ocb_bar)
{
	write_scom(PU_OCB_PIB_OCBCSR0_OR, PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_MODE));
	write_scom(PU_OCB_PIB_OCBCSR0_CLEAR, PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_TYPE));
	write_scom(PU_OCB_PIB_OCBAR0, (uint64_t)ocb_bar << 32);
}

static void check_ocb_mode(uint64_t OCBCSR_address, uint64_t OCBSHCS_address)
{
	uint64_t ocb_pib = read_scom(OCBCSR_address);

	/*
	 * The following check for circular mode is an additional check
	 * performed to ensure a valid data access.
	 */
	if ((ocb_pib & PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_MODE)) &&
	    (ocb_pib & PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_TYPE))) {
		/*
		 * Check if push queue is enabled. If not, let the store occur
		 * anyway to let the PIB error response return occur. (That is
		 * what will happen if this checking code were not here.)
		 */
		uint64_t stream_push_control = read_scom(OCBSHCS_address);

		if (stream_push_control & PPC_BIT(OCB_OCI_OCBSHCS0_PUSH_ENABLE)) {
			uint8_t counter = 0;
			for (counter = 0; counter < 4; counter++) {
				/* Proceed if the OCB_OCI_OCBSHCS0_PUSH_FULL is clear */
				if (!(stream_push_control & PPC_BIT(OCB_OCI_OCBSHCS0_PUSH_FULL)))
					break;

				/* Hostboot has delay of 0 here */
				wait_us(1, false);

				stream_push_control = read_scom(OCBSHCS_address);
			}

			if (counter == 4)
				die("Failed to write to circular buffer.\n");
		}
	}
}

static void put_ocb_indirect(uint32_t ocb_req_length, uint32_t oci_address,
			     uint64_t *ocb_buffer)
{
	write_scom(PU_OCB_PIB_OCBAR0, (uint64_t)oci_address << 32);

	check_ocb_mode(PU_OCB_PIB_OCBCSR0_RO, PU_OCB_OCI_OCBSHCS0_SCOM);

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

void write_occ_command(uint64_t write_data)
{
	check_ocb_mode(PU_OCB_PIB_OCBCSR1_RO, PU_OCB_OCI_OCBSHCS1_SCOM);
	write_scom(PU_OCB_PIB_OCBDR1, write_data);
}

void clear_occ_special_wakeups(void)
{
	for (size_t chiplet_index = 0; chiplet_index < NUMBER_OF_EX_CHIPLETS; ++chiplet_index)
		scom_and_for_chiplet(EX_CHIPLETS[chiplet_index], EX_PPM_SPWKUP_OCC,
				     ~PPC_BIT(0));
}

static uint32_t ppc_lis(uint16_t rt, uint16_t data)
{
	uint32_t inst;
	inst = LIS_OP;
	inst |= rt << (31 - 10);
	inst |= data;
	return inst;
}

static uint32_t ppc_ori(uint16_t rs, uint16_t ra, uint16_t data)
{
	uint32_t inst;
	inst = ORI_OP;
	inst |= rs << (31 - 10);
	inst |= ra << (31 - 15);
	inst |= data;
	return inst;
}

static uint32_t ppc_mtspr(uint16_t rs, uint16_t spr)
{
	enum { MTSPR_CONST1 = 467 };

	uint32_t temp = ((spr & 0x03FF) << (31 - 20));

	uint32_t inst;
	inst = MTSPR_OP;
	inst |= rs << (31 - 10);
	inst |= (temp & 0x0000F800) << 5;  // Perform swizzle
	inst |= (temp & 0x001F0000) >> 5;  // Perform swizzle
	inst |= MTSPR_CONST1 << 1;
	return inst;
}

static uint32_t ppc_bctr(void)
{
	enum { BCCTR_CONST1 = 528 };

	uint32_t inst;
	inst = BCCTR_OP;
	inst |= 20 << (31 - 10); // BO
	/* BI = 0 is taken care of by inst = 0 */
	inst |= BCCTR_CONST1 << 1;
	return inst;
}

static uint32_t ppc_b(uint32_t target_addr)
{
	uint32_t inst;
	inst = BR_OP;
	inst |= (target_addr & 0x03FFFFFF);
	return inst;
}

/* Sets up boot loader in SRAM and returns 32-bit jump instruction to it */
static uint64_t setup_memory_boot(void)
{
	enum {
		OCC_BOOT_OFFSET = 0x40,
		CTR = 9,
		OCC_SRAM_BOOT_ADDR = 0xFFF40000,
		OCC_SRAM_BOOT_ADDR2 = 0xFFF40002,
	};

	uint64_t sram_program[2];

	/* lis r1, 0x8000 */
	sram_program[0] = ((uint64_t)ppc_lis(1, 0x8000) << 32);

	/* ori r1, r1, OCC_BOOT_OFFSET */
	sram_program[0] |= ppc_ori(1, 1, OCC_BOOT_OFFSET);

	/* mtctr (mtspr r1, CTR) */
	sram_program[1] = ((uint64_t)ppc_mtspr(1, CTR) << 32);

	/* bctr */
	sram_program[1] |= ppc_bctr();

	/* Write to SRAM */
	writeOCCSRAM(OCC_SRAM_BOOT_ADDR, sram_program, sizeof(sram_program));

	return ((uint64_t)ppc_b(OCC_SRAM_BOOT_ADDR2) << 32);
}

void occ_start_from_mem(void)
{
	enum {
		OCB_PIB_OCR_CORE_RESET_BIT = 0,
		JTG_PIB_OJCFG_DBG_HALT_BIT = 6,

		PU_SRAM_SRBV0_SCOM = 0x0006A004,

		PU_JTG_PIB_OJCFG_AND = 0x0006D005,
		PU_OCB_PIB_OCR_CLEAR = 0x0006D001,
		PU_OCB_PIB_OCR_OR    = 0x0006D002,
	};

	write_scom(PU_OCB_PIB_OCBCSR0_OR, PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_MODE));

	/*
	 * Set up Boot Vector Registers in SRAM:
	 *  - set bv0-2 to all 0's (illegal instructions)
	 *  - set bv3 to proper branch instruction
	 */
	write_scom(PU_SRAM_SRBV0_SCOM, 0);
	write_scom(PU_SRAM_SRBV0_SCOM + 1, 0);
	write_scom(PU_SRAM_SRBV0_SCOM + 2, 0);
	write_scom(PU_SRAM_SRBV0_SCOM + 3, setup_memory_boot());

	write_scom(PU_JTG_PIB_OJCFG_AND, ~PPC_BIT(JTG_PIB_OJCFG_DBG_HALT_BIT));
	write_scom(PU_OCB_PIB_OCR_OR, PPC_BIT(OCB_PIB_OCR_CORE_RESET_BIT));
	write_scom(PU_OCB_PIB_OCR_CLEAR, PPC_BIT(OCB_PIB_OCR_CORE_RESET_BIT));
}
