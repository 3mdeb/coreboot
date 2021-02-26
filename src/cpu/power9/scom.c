/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <cpu/power/scom.h>
#include <cpu/power/spr.h>		// HMER
#include <console/console.h>

#define XSCOM_DATA_IND_READ		PPC_BIT(0)
#define XSCOM_DATA_IND_COMPLETE		PPC_BIT(32)
#define XSCOM_DATA_IND_ERR		PPC_BITMASK(33,35)
#define XSCOM_DATA_IND_DATA		PPC_BITMASK(48,63)
#define XSCOM_DATA_IND_FORM1_DATA	PPC_BITMASK(12,63)
#define XSCOM_IND_MAX_RETRIES		10

#define XSCOM_RCVED_STAT_REG		0x00090018
#define XSCOM_LOG_REG			0x00090012
#define XSCOM_ERR_REG			0x00090013

/*
 * WARNING:
 * Indirect access uses the same approach as Hostboot, yet all our tests so far
 * were unsuccessful. It is possible that the devices we were trying to access
 * must be initialized or otherwise enabled first. Because of that we decided to
 * leave it as it is for now, with heavy debugging, and return to this when we
 * are sure that it doesn't work because of error in implementation.
 */
void write_scom_indirect(uint64_t reg_address, uint64_t value)
{
	uint64_t addr;
	uint64_t data;
	addr = reg_address & 0x7FFFFFFF;
	data = reg_address & XSCOM_ADDR_IND_ADDR;
	data |= value & XSCOM_ADDR_IND_DATA;
<<<<<<< HEAD

	write_scom_direct(addr, data);

	for (int retries = 0; retries < XSCOM_IND_MAX_RETRIES; ++retries) {
		data = read_scom_direct(addr);
		if((data & XSCOM_DATA_IND_COMPLETE) && ((data & XSCOM_DATA_IND_ERR) == 0)) {
			return;
		}
		else if(data & XSCOM_DATA_IND_COMPLETE) {
			printk(BIOS_EMERG, "SCOM WR error  %16.16llx = %16.16llx : %16.16llx\n",
			       reg_address, value, data);
		}
		// TODO: delay?
=======
	printk(BIOS_EMERG, "----------------\n");
	printk(BIOS_EMERG, "SCOM WRITE\n\n");
	/*
	 * Reading 0x90013 here doesn't make sense. This register reports errors in
	 * the most recent XSCOM access, and is cleared as soon as a new access is
	 * attempted, meaning we can only see that reading 0x90013 was successful.
	 * It is possible to read this register through 'pdbg' from BMC which
	 * doesn't use XSCOM, in which case 0x90013 isn't polluted.
	 */
	printk(BIOS_EMERG, "*0x90010 = %llX,\n*0x90011 = %llX,\n*0x90012 = %llX,\n*0x90013 = %llX\n",
		read_scom_direct(0x90010),
		read_scom_direct(0x90011),
		read_scom_direct(0x90012),
		read_scom_direct(0x90013));
	printk(BIOS_EMERG, "#hmer = %llX,\n#fail = %llX,\n#done = %llX,\n#status = %llX\n",
		read_hmer(),
		read_hmer() & SPR_HMER_XSCOM_FAIL,
		read_hmer() & SPR_HMER_XSCOM_DONE,
		read_hmer() & SPR_HMER_XSCOM_STATUS);
	printk(BIOS_EMERG, "WRITING addr = %llX, data = %llX\n", addr, data);
	for(unsigned int i = 0; i < 0xFFFF; ++i) {
		asm volatile("nop" :::"memory");
	}
	write_scom_direct(addr, data);
	printk(BIOS_EMERG, "#hmer = %llX,\n#fail = %llX,\n#done = %llX,\n#status = %llX\n",
		read_hmer(),
		read_hmer() & SPR_HMER_XSCOM_FAIL,
		read_hmer() & SPR_HMER_XSCOM_DONE,
		read_hmer() & SPR_HMER_XSCOM_STATUS);
	printk(BIOS_EMERG, "*0x90010 = %llX,\n*0x90011 = %llX,\n*0x90012 = %llX,\n*0x90013 = %llX\n",
		read_scom_direct(0x90010),
		read_scom_direct(0x90011),
		read_scom_direct(0x90012),
		read_scom_direct(0x90013));
	for(int retries = 0; retries < XSCOM_IND_MAX_RETRIES; ++retries) {
		printk(BIOS_EMERG, "READING addr = %llX\n", addr);
		for(unsigned int i = 0; i < 0xFFFF; ++i) {
			asm volatile("nop" :::"memory");
		}
		data = read_scom_direct(addr);
		printk(BIOS_EMERG, "#hmer = %llX,\n#fail = %llX,\n#done = %llX,\n#status = %llX\n",
			read_hmer(),
			read_hmer() & SPR_HMER_XSCOM_FAIL,
			read_hmer() & SPR_HMER_XSCOM_DONE,
			read_hmer() & SPR_HMER_XSCOM_STATUS);
		printk(BIOS_EMERG, "*0x90010 = %llX,\n*0x90011 = %llX,\n*0x90012 = %llX,\n*0x90013 = %llX\n",
		read_scom_direct(0x90010),
		read_scom_direct(0x90011),
		read_scom_direct(0x90012),
		read_scom_direct(0x90013));
		printk(BIOS_EMERG, "READ data = %llX\n", data);
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
>>>>>>> cpu/power9: fix directory structure and timer build fault
	}
}

uint64_t read_scom_indirect(uint64_t reg_address)
{
	uint64_t addr;
<<<<<<< HEAD
	uint64_t data;
	addr = reg_address & 0x7FFFFFFF;
	data = XSCOM_DATA_IND_READ | (reg_address & XSCOM_ADDR_IND_ADDR);

	write_scom_direct(addr, data);

	for (int retries = 0; retries < XSCOM_IND_MAX_RETRIES; ++retries) {
		data = read_scom_direct(addr);
		if((data & XSCOM_DATA_IND_COMPLETE) && ((data & XSCOM_DATA_IND_ERR) == 0)) {
			break;
		}
		else if(data & XSCOM_DATA_IND_COMPLETE) {
			printk(BIOS_EMERG, "SCOM RD error  %16.16llx : %16.16llx\n",
			       reg_address, data);
		}
		// TODO: delay?
=======
	uint64_t data = -1;
	addr = reg_address & 0x7FFFFFFF;
	data = XSCOM_DATA_IND_READ | (reg_address & XSCOM_ADDR_IND_ADDR);
	printk(BIOS_EMERG, "----------------\n");
	printk(BIOS_EMERG, "SCOM READ\n\n");
	printk(BIOS_EMERG, "*0x90010 = %llX,\n*0x90011 = %llX,\n*0x90012 = %llX,\n*0x90013 = %llX\n",
		read_scom_direct(0x90010),
		read_scom_direct(0x90011),
		read_scom_direct(0x90012),
		read_scom_direct(0x90013));
	printk(BIOS_EMERG, "#hmer = %llX,\n#fail = %llX,\n#done = %llX,\n#status = %llX\n",
		read_hmer(),
		read_hmer() & SPR_HMER_XSCOM_FAIL,
		read_hmer() & SPR_HMER_XSCOM_DONE,
		read_hmer() & SPR_HMER_XSCOM_STATUS);
	printk(BIOS_EMERG, "WRITING addr = %llX, data = %llX\n", addr, data);
	for(unsigned int i = 0; i < 0xFFFF; ++i) {
		asm volatile("nop" :::"memory");
	}
	write_scom_direct(addr, data);
	printk(BIOS_EMERG, "#hmer = %llX,\n#fail = %llX,\n#done = %llX,\n#status = %llX\n",
		read_hmer(),
		read_hmer() & SPR_HMER_XSCOM_FAIL,
		read_hmer() & SPR_HMER_XSCOM_DONE,
		read_hmer() & SPR_HMER_XSCOM_STATUS);
	printk(BIOS_EMERG, "*0x90010 = %llX,\n*0x90011 = %llX,\n*0x90012 = %llX,\n*0x90013 = %llX\n",
		read_scom_direct(0x90010),
		read_scom_direct(0x90011),
		read_scom_direct(0x90012),
		read_scom_direct(0x90013));
	for(int retries = 0; retries < XSCOM_IND_MAX_RETRIES; ++retries) {
		printk(BIOS_EMERG, "READING addr = %llX\n", addr);
		for(unsigned int i = 0; i < 0xFFFF; ++i) {
			asm volatile("nop" :::"memory");
		}
		data = read_scom_direct(addr);
		printk(BIOS_EMERG, "#hmer = %llX,\n#fail = %llX,\n#done = %llX,\n#status = %llX\n",
			read_hmer(),
			read_hmer() & SPR_HMER_XSCOM_FAIL,
			read_hmer() & SPR_HMER_XSCOM_DONE,
			read_hmer() & SPR_HMER_XSCOM_STATUS);
		printk(BIOS_EMERG, "*0x90010 = %llX,\n*0x90011 = %llX,\n*0x90012 = %llX,\n*0x90013 = %llX\n",
		read_scom_direct(0x90010),
		read_scom_direct(0x90011),
		read_scom_direct(0x90012),
		read_scom_direct(0x90013));
		printk(BIOS_EMERG, "READ data = %llX\n", data);
		printk(BIOS_EMERG, "#hmer = %llX,\n#fail = %llX,\n#done = %llX,\n#status = %llX\n",
			read_hmer(),
			read_hmer() & SPR_HMER_XSCOM_FAIL,
			read_hmer() & SPR_HMER_XSCOM_DONE,
			read_hmer() & SPR_HMER_XSCOM_STATUS);
		if((data & XSCOM_DATA_IND_COMPLETE) && ((data & XSCOM_DATA_IND_ERR) == 0)) {
			printk(BIOS_EMERG, "SUCCESS\n");
			printk(BIOS_EMERG, "----------------\n");
		}
		else if(data & XSCOM_DATA_IND_COMPLETE) {
			printk(BIOS_EMERG, "FINISHED WITH ERROR\n");
			printk(BIOS_EMERG, "----------------\n");
		}
>>>>>>> cpu/power9: fix directory structure and timer build fault
	}

	return data & XSCOM_DATA_IND_DATA;
}

/* This function should be rarely called, don't make it inlined */
void reset_scom_engine(void)
{
	/*
	 * With cross-CPU SCOM accesses, first register should be cleared on the
	 * executing CPU, the other two on target CPU. In that case it may be
	 * necessary to do the remote writes in assembly directly to skip checking
	 * HMER and possibly end in a loop.
	 */
	write_scom_direct(XSCOM_RCVED_STAT_REG, 0);
	write_scom_direct(XSCOM_LOG_REG, 0);
	write_scom_direct(XSCOM_ERR_REG, 0);
	eieio();
}
