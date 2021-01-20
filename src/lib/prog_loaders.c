/* SPDX-License-Identifier: GPL-2.0-only */


#include <stdlib.h>
#include <cbfs.h>
#include <cbmem.h>
#include <console/console.h>
#include <fallback.h>
#include <halt.h>
#include <lib.h>
#include <program_loading.h>
#include <reset.h>
#include <romstage_handoff.h>
#include <rmodule.h>
#include <stage_cache.h>
#include <symbols.h>
#include <timestamp.h>
#include <fit_payload.h>
#include <security/vboot/vboot_common.h>
#include <arch/io.h>

/* Only can represent up to 1 byte less than size_t. */
const struct mem_region_device addrspace_32bit =
	MEM_REGION_DEV_RO_INIT(0, ~0UL);

int prog_locate(struct prog *prog)
{
	struct cbfsf file;

	if (prog_locate_hook(prog))
		return -1;

	if (cbfs_boot_locate(&file, prog_name(prog), NULL))
		return -1;

	cbfsf_file_type(&file, &prog->cbfs_type);

	cbfs_file_data(prog_rdev(prog), &file);

	return 0;
}

/* Bits in HMER/HMEER */
#define SPR_HMER_MALFUNCTION_ALERT	PPC_BIT(0)
#define SPR_HMER_PROC_RECV_DONE		PPC_BIT(2)
#define SPR_HMER_PROC_RECV_ERROR_MASKED	PPC_BIT(3)
#define SPR_HMER_TFAC_ERROR		PPC_BIT(4)
#define SPR_HMER_TFMR_PARITY_ERROR	PPC_BIT(5)
#define SPR_HMER_XSCOM_FAIL		PPC_BIT(8)
#define SPR_HMER_XSCOM_DONE		PPC_BIT(9)
#define SPR_HMER_PROC_RECV_AGAIN	PPC_BIT(11)
#define SPR_HMER_WARN_RISE		PPC_BIT(14)
#define SPR_HMER_WARN_FALL		PPC_BIT(15)
#define SPR_HMER_SCOM_FIR_HMI		PPC_BIT(16)
#define SPR_HMER_TRIG_FIR_HMI		PPC_BIT(17)
#define SPR_HMER_HYP_RESOURCE_ERR	PPC_BIT(20)
#define SPR_HMER_XSCOM_STATUS		PPC_BITMASK(21,23)

void read_scom_direct(uint32_t, uint64_t*);
void read_scom_direct(uint32_t reg_address, uint64_t *buffer)
{
	asm volatile(
		"ldcix %0, %1, %2":
		"=r"(*buffer):
		"b"(0x800603FC00000000),
		"r"(reg_address << 3));
	eieio();
}

void write_scom_direct(uint32_t, uint64_t*);
void write_scom_direct(uint32_t reg_address, uint64_t *buffer)
{
	asm volatile(
		"stdcix %0, %1, %2"::
		"b"(*buffer),
		"b"(0x800603FC00000000),
		"r"(reg_address << 3));
	eieio();
}

uint64_t read_hmer(void);
uint64_t read_hmer(void)
{
	unsigned long val;
	asm volatile("mfspr %0,%1" : "=r"(val) : "i"(0x150) : "memory");
	return val;
}

#define PPC_BIT(bit)		(0x8000000000000000UL >> (bit))
#define PPC_BITMASK(bs,be)	((PPC_BIT(bs) - PPC_BIT(be)) | PPC_BIT(bs))
#define XSCOM_ADDR_IND_FLAG		PPC_BIT(0)
#define XSCOM_ADDR_IND_ADDR		PPC_BITMASK(12,31)
#define XSCOM_ADDR_IND_DATA		PPC_BITMASK(48,63)

#define XSCOM_DATA_IND_READ		PPC_BIT(0)
#define XSCOM_DATA_IND_COMPLETE		PPC_BIT(32)
#define XSCOM_DATA_IND_ERR		PPC_BITMASK(33,35)
#define XSCOM_DATA_IND_DATA		PPC_BITMASK(48,63)
#define XSCOM_DATA_IND_FORM1_DATA	PPC_BITMASK(12,63)
#define XSCOM_IND_MAX_RETRIES 10

void write_scom_indirect(uint64_t, uint64_t);
void write_scom_indirect(uint64_t reg_address, uint64_t value)
{
	uint64_t addr;
	uint64_t data;
	addr = reg_address & 0x7FFFFFFF;
	data = reg_address & XSCOM_ADDR_IND_ADDR;
	data |= value & XSCOM_ADDR_IND_DATA;
	printk(BIOS_EMERG, "----------------\n");
	printk(BIOS_EMERG, "SCOM WRITE\n\n");
	printk(BIOS_EMERG, "hmer = %llX, fail = %llX, done = %llX, status = %llX\n",
		read_hmer(),
		read_hmer() & SPR_HMER_XSCOM_FAIL,
		read_hmer() & SPR_HMER_XSCOM_DONE,
		read_hmer() & SPR_HMER_XSCOM_STATUS);
	printk(BIOS_EMERG, "WRITING addr = %llX, data = %llX\n", addr, data);
	write_scom_direct(addr, &data);
	printk(BIOS_EMERG, "hmer = %llX, fail = %llX, done = %llX, status = %llX\n",
		read_hmer(),
		read_hmer() & SPR_HMER_XSCOM_FAIL,
		read_hmer() & SPR_HMER_XSCOM_DONE,
		read_hmer() & SPR_HMER_XSCOM_STATUS);
	for(int retries = 0; retries < XSCOM_IND_MAX_RETRIES; ++retries) {
		printk(BIOS_EMERG, "READING addr = %llX\n", addr);
		read_scom_direct(addr, &data);
		printk(BIOS_EMERG, "READ data = %llX\n", data);
		printk(BIOS_EMERG, "hmer = %llX, fail = %llX, done = %llX, status = %llX\n",
			read_hmer(),
			read_hmer() & SPR_HMER_XSCOM_FAIL,
			read_hmer() & SPR_HMER_XSCOM_DONE,
			read_hmer() & SPR_HMER_XSCOM_STATUS);
		if((data & XSCOM_DATA_IND_COMPLETE) && ((data & XSCOM_DATA_IND_ERR) == 0)) {
			printk(BIOS_EMERG, "SUCCESS\n");
			printk(BIOS_EMERG, "----------------\n");
			return;
		}
		else if(data & XSCOM_DATA_IND_COMPLETE) {
			printk(BIOS_EMERG, "FINISHED WITH ERROR\n");
			printk(BIOS_EMERG, "----------------\n");
			return;
		}
	}
}

void read_scom_indirect(uint64_t, uint64_t*);
void read_scom_indirect(uint64_t reg_address, uint64_t *buffer)
{
	uint64_t addr;
	uint64_t data;
	addr = reg_address & 0x7FFFFFFF;
	data = XSCOM_DATA_IND_READ | (reg_address & XSCOM_ADDR_IND_ADDR);
	printk(BIOS_EMERG, "----------------\n");
	printk(BIOS_EMERG, "SCOM READ\n\n");
	printk(BIOS_EMERG, "hmer = %llX, fail = %llX, done = %llX, status = %llX\n",
		read_hmer(),
		read_hmer() & SPR_HMER_XSCOM_FAIL,
		read_hmer() & SPR_HMER_XSCOM_DONE,
		read_hmer() & SPR_HMER_XSCOM_STATUS);
	printk(BIOS_EMERG, "WRITING addr = %llX, data = %llX\n", addr, data);
	write_scom_direct(addr, &data);
		printk(BIOS_EMERG, "hmer = %llX, fail = %llX, done = %llX, status = %llX\n",
		read_hmer(),
		read_hmer() & SPR_HMER_XSCOM_FAIL,
		read_hmer() & SPR_HMER_XSCOM_DONE,
		read_hmer() & SPR_HMER_XSCOM_STATUS);
	for(int retries = 0; retries < XSCOM_IND_MAX_RETRIES; ++retries) {
		printk(BIOS_EMERG, "READING addr = %llX\n", addr);
		read_scom_direct(addr, &data);
		printk(BIOS_EMERG, "READ data = %llX\n", data);
		printk(BIOS_EMERG, "hmer = %llX, fail = %llX, done = %llX, status = %llX\n",
			read_hmer(),
			read_hmer() & SPR_HMER_XSCOM_FAIL,
			read_hmer() & SPR_HMER_XSCOM_DONE,
			read_hmer() & SPR_HMER_XSCOM_STATUS);
		if((data & XSCOM_DATA_IND_COMPLETE) && ((data & XSCOM_DATA_IND_ERR) == 0)) {
			*buffer = data & XSCOM_DATA_IND_DATA;
			printk(BIOS_EMERG, "SUCCESS\n");
			printk(BIOS_EMERG, "----------------\n");
			return;
		}
		else if(data & XSCOM_DATA_IND_COMPLETE) {
			printk(BIOS_EMERG, "FINISHED WITH ERROR\n");
			printk(BIOS_EMERG, "----------------\n");
			return;
		}
	}

}

void run_romstage(void)
{
	uint64_t buffer;
	long long unsigned int hmer = 0;
	asm volatile("mtspr 336, %0" :: "r" (hmer) : "memory");
	read_scom_indirect(0x8000C8000701103Full, &buffer);
	buffer = 0x7777777777777777;
	asm volatile("mtspr 336, %0" :: "r" (hmer) : "memory");
	write_scom_indirect(0x8000C8000701103Full, buffer);
	asm volatile("mtspr 336, %0" :: "r" (hmer) : "memory");
	read_scom_indirect(0x8000C8000701103Full, &buffer);
	struct prog romstage =
		PROG_INIT(PROG_ROMSTAGE, CONFIG_CBFS_PREFIX "/romstage");
	vboot_run_logic();

	if (ENV_X86 && CONFIG(BOOTBLOCK_NORMAL)) {
		if (legacy_romstage_selector(&romstage))
			goto fail;
	} else {
		if (prog_locate(&romstage))
			goto fail;
	}

	timestamp_add_now(TS_START_COPYROM);

	if (cbfs_prog_stage_load(&romstage))
		goto fail;

	timestamp_add_now(TS_END_COPYROM);

	console_time_report();

	prog_run(&romstage);

fail:
	if (CONFIG(BOOTBLOCK_CONSOLE))
		die_with_post_code(POST_INVALID_ROM,
				   "Couldn't load romstage.\n");
	halt();
}

int __weak prog_locate_hook(struct prog *prog) { return 0; }

static void run_ramstage_from_resume(struct prog *ramstage)
{
	if (!romstage_handoff_is_resume())
		return;

	/* Load the cached ramstage to runtime location. */
	stage_cache_load_stage(STAGE_RAMSTAGE, ramstage);

	prog_set_arg(ramstage, cbmem_top());

	if (prog_entry(ramstage) != NULL) {
		printk(BIOS_DEBUG, "Jumping to image.\n");
		prog_run(ramstage);
	}

	printk(BIOS_ERR, "ramstage cache invalid.\n");
	board_reset();
}

static int load_relocatable_ramstage(struct prog *ramstage)
{
	struct rmod_stage_load rmod_ram = {
		.cbmem_id = CBMEM_ID_RAMSTAGE,
		.prog = ramstage,
	};

	return rmodule_stage_load(&rmod_ram);
}

void run_ramstage(void)
{
	struct prog ramstage =
		PROG_INIT(PROG_RAMSTAGE, CONFIG_CBFS_PREFIX "/ramstage");

	if (ENV_POSTCAR)
		timestamp_add_now(TS_END_POSTCAR);

	/* Call "end of romstage" here if postcar stage doesn't exist */
	if (ENV_ROMSTAGE)
		timestamp_add_now(TS_END_ROMSTAGE);

	/*
	 * Only x86 systems using ramstage stage cache currently take the same
	 * firmware path on resume.
	 */
	if (ENV_X86 && !CONFIG(NO_STAGE_CACHE))
		run_ramstage_from_resume(&ramstage);

	vboot_run_logic();

	if (prog_locate(&ramstage))
		goto fail;

	timestamp_add_now(TS_START_COPYRAM);

	if (ENV_X86) {
		if (load_relocatable_ramstage(&ramstage))
			goto fail;
	} else {
		if (cbfs_prog_stage_load(&ramstage))
			goto fail;
	}

	stage_cache_add(STAGE_RAMSTAGE, &ramstage);

	timestamp_add_now(TS_END_COPYRAM);

	console_time_report();

	/* This overrides the arg fetched from the relocatable module */
	prog_set_arg(&ramstage, cbmem_top());

	prog_run(&ramstage);

fail:
	die_with_post_code(POST_INVALID_ROM, "Ramstage was not loaded!\n");
}

#if ENV_PAYLOAD_LOADER // gc-sections should take care of this

static struct prog global_payload =
	PROG_INIT(PROG_PAYLOAD, CONFIG_CBFS_PREFIX "/payload");

void payload_load(void)
{
	struct prog *payload = &global_payload;

	timestamp_add_now(TS_LOAD_PAYLOAD);

	if (prog_locate(payload))
		goto out;

	switch (prog_cbfs_type(payload)) {
	case CBFS_TYPE_SELF: /* Simple ELF */
		selfload_check(payload, BM_MEM_RAM);
		break;
	case CBFS_TYPE_FIT: /* Flattened image tree */
		if (CONFIG(PAYLOAD_FIT_SUPPORT)) {
			fit_payload(payload);
			break;
		} /* else fall-through */
	default:
		die_with_post_code(POST_INVALID_ROM,
				   "Unsupported payload type.\n");
		break;
	}

out:
	if (prog_entry(payload) == NULL)
		die_with_post_code(POST_INVALID_ROM, "Payload not loaded.\n");
}

void payload_run(void)
{
	struct prog *payload = &global_payload;

	/* Reset to booting from this image as late as possible */
	boot_successful();

	printk(BIOS_DEBUG, "Jumping to boot code at %p(%p)\n",
		prog_entry(payload), prog_entry_arg(payload));

	post_code(POST_ENTER_ELF_BOOT);

	timestamp_add_now(TS_SELFBOOT_JUMP);

	/* Before we go off to run the payload, see if
	 * we stayed within our bounds.
	 */
	checkstack(_estack, 0);

	prog_run(payload);
}

#endif
