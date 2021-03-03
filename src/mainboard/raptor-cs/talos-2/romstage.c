/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/vpd.h>
#include <cpu/power/istep_13.h>
#include <program_loading.h>
#include <device/smbus_host.h>
#include <lib.h>	// hexdump
#include <spd_bin.h>
#include <endian.h>

mcbist_data_t mem_data;

static void dump_mca_data(mca_data_t *mca)
{
	printk(BIOS_SPEW, "\tCL =\t%d\n", mca->cl);
	printk(BIOS_SPEW, "\tCCD_L =\t%d\n", mca->nccd_l);
	printk(BIOS_SPEW, "\tWTR_S =\t%d\n", mca->nwtr_s);
	printk(BIOS_SPEW, "\tWTR_L =\t%d\n", mca->nwtr_l);
	printk(BIOS_SPEW, "\tFAW =\t%d\n", mca->nfaw);
	printk(BIOS_SPEW, "\tRCD =\t%d\n", mca->nrcd);
	printk(BIOS_SPEW, "\tRP =\t%d\n", mca->nrp);
	printk(BIOS_SPEW, "\tRAS =\t%d\n", mca->nras);
	printk(BIOS_SPEW, "\tWR =\t%d\n", mca->nwr);
	printk(BIOS_SPEW, "\tRRD_S =\t%d\n", mca->nrrd_s);
	printk(BIOS_SPEW, "\tRRD_L =\t%d\n", mca->nrrd_l);
	printk(BIOS_SPEW, "\tRFC =\t%d\n", mca->nrfc);
}

static inline bool is_proper_dimm(spd_raw_data spd, int slot)
{
	dimm_attr attr;
	if (spd == NULL)
		return false;

	if (spd_decode_ddr4(&attr, spd) != SPD_STATUS_OK) {
		printk(BIOS_ERR, "Malformed SPD for slot %d\n", slot);
		return false;
	}

	if (attr.dram_type != SPD_MEMORY_TYPE_DDR4_SDRAM || attr.dimm_type != SPD_DIMM_TYPE_RDIMM ||
	    attr.ecc_extension == false) {
		printk(BIOS_ERR, "Bad DIMM type in slot %d\n", slot);
		return false;
	}

	return true;
}

/*
 * All time conversion functions assume that both MCS have the same frequency.
 * Change it if proven otherwise by adding a second argument - memory speed or
 * MCS index.
 */
static inline uint64_t ps_to_nck(uint64_t ps)
{
	/*
	 * Speed is in MT/s, we need to divide it by 2 to get MHz.
	 * tCK(avg) should be rounded down to the next valid speed bin, which
	 * corresponds to value obtained by using standardized MT/s values.
	 */
	uint64_t tck_in_ps = 1000000 / (mem_data.speed / 2);

	/* Algorithm taken from JEDEC Standard No. 21-C */
	return ((ps * 1000 / tck_in_ps) + 974) / 1000;
}

static inline uint64_t mtb_ftb_to_nck(uint64_t mtb, int8_t ftb)
{
	/* ftb is signed (always byte?) */
	return ps_to_nck(mtb * 125 + ftb);
}

static inline uint64_t ns_to_nck(uint64_t ns)
{
	return ps_to_nck(ns * 1000);
}

static void mark_nonfunctional(int mcs, int mca)
{
	mem_data.mcs[mcs].mca[mca].functional = false;

	/* Propagate upwards */
	if (mem_data.mcs[mcs].mca[mca ^ 1].functional == false) {
		mem_data.mcs[mcs].functional = false;
		if (mem_data.mcs[mcs ^ 1].functional == false)
			die("No functional MCS left");
	}
}

static uint64_t find_min_mtb_ftb(rdimm_data_t *dimm, int mtb_idx, int ftb_idx)
{
	uint64_t val0 = 0, val1 = 0;

	if (dimm[0].present)
		val0 = mtb_ftb_to_nck(dimm[0].spd[mtb_idx], (int8_t)dimm[0].spd[ftb_idx]);
	if (dimm[1].present)
		val1 = mtb_ftb_to_nck(dimm[1].spd[mtb_idx], (int8_t)dimm[1].spd[ftb_idx]);

	return (val0 < val1) ? val1 : val0;
}

static uint64_t find_min_multi_mtb(rdimm_data_t *dimm, int mtb_l, int mtb_h, uint8_t mask, int shift)
{
	uint64_t val0 = 0, val1 = 0;

	if (dimm[0].present)
		val0 = dimm[0].spd[mtb_l] | ((dimm[0].spd[mtb_h] & mask) << shift);
	if (dimm[1].present)
		val1 = dimm[1].spd[mtb_l] | ((dimm[1].spd[mtb_h] & mask) << shift);

	return (val0 < val1) ? mtb_ftb_to_nck(val1, 0) : mtb_ftb_to_nck(val0, 0);
}

/* This is most of step 7 condensed into one function */
static void prepare_dimm_data(void)
{
	int i, mcs, mca;
	int tckmin = 0x06;		// Platform limit

	struct spd_block blk = {
		.addr_map = { DIMM0, DIMM1, DIMM2, DIMM3, DIMM4, DIMM5, DIMM6, DIMM7},
	};

	get_spd_smbus(&blk);
	hexdump(blk.spd_array[2], blk.len);		// Remove when no longer useful
	dump_spd_info(&blk);

	/*
	 * We need to find the highest common (for all DIMMs and the platform)
	 * supported frequency, meaning we need to compare minimum clock cycle times
	 * and choose the highest value. For the range supported by the platform we
	 * can check MTB only.
	 *
	 * TODO1: maximum for 2 DIMMs on one port (channel) is 2400 MT/s, this loop
	 * doesn't check for that.
	 *
	 * TODO2: check if we can have different frequencies across MCSs.
	 */
	for (i = 0; i < CONFIG_DIMM_MAX; i++) {
		if (is_proper_dimm(blk.spd_array[i], i)) {
			mcs = i / DIMMS_PER_MCS;
			mca = (i % DIMMS_PER_MCS) / MCA_PER_MCS;
			int dimm_idx = i % 2;	// (i % DIMMS_PER_MCS) % MCA_PER_MCS

			mem_data.mcs[mcs].functional = true;
			mem_data.mcs[mcs].mca[mca].functional = true;

			rdimm_data_t *dimm = &mem_data.mcs[mcs].mca[mca].dimm[dimm_idx];

			dimm->present = true;
			dimm->spd = blk.spd_array[i];
			/*
			 * SPD fields in spd.h are not compatible with DDR4 and those in
			 * spd_bin.h are just a few of all required.
			 *
			 * TODO: add fields that are lacking to either of those files or
			 * add a file specific to DDR4 SPD.
			 */
			dimm->mranks = ((blk.spd_array[i][12] >> 3) & 0x7) + 1;
			dimm->log_ranks = dimm->mranks * (((blk.spd_array[i][6] >> 4) & 0x7) + 1);

			if (blk.spd_array[i][18] > tckmin)
				tckmin = blk.spd_array[i][18];
		}
	}

	/*
	 * There is one (?) MCBIST per CPU. Fail if there are no supported DIMMs
	 * connected, otherwise assume it is functional. There is no reason to redo
	 * this test in the rest of isteps.
	 *
	 * TODO: 2 CPUs with one DIMM (in total) will not work with this code.
	 */
	if (mem_data.mcs[0].functional == false && mem_data.mcs[1].functional == false)
		die("No DIMMs detected, aborting\n");

	switch (tckmin) {
		/* For CWL assume 1tCK write preamble */
		case 0x06:
			mem_data.speed = 2666;
			mem_data.cwl = 14;
			break;
		case 0x07:
			mem_data.speed = 2400;
			mem_data.cwl = 12;
			break;
		case 0x08:
			mem_data.speed = 2133;
			mem_data.cwl = 11;
			break;
		case 0x09:
			mem_data.speed = 1866;
			mem_data.cwl = 10;
			break;
		default:
			die("Unsupported tCKmin: %d ps (+/- 125)\n", tckmin * 125);
	}

	/* Now that we know our speed, we can calculate the rest of the data */
	mem_data.nrefi = ns_to_nck(7800);
	mem_data.nrtp = ps_to_nck(7500);
	printk(BIOS_SPEW, "Common memory parameters:\n"
	                  "\tspeed =\t%d MT/s\n"
	                  "\tREFI =\t%d clock cycles\n"
	                  "\tCWL =\t%d clock cycles\n"
	                  "\tRTP =\t%d clock cycles\n",
	                  mem_data.speed, mem_data.nrefi, mem_data.cwl, mem_data.nrtp);

	for (mcs = 0; mcs < MCS_PER_PROC; mcs++) {
		if (!mem_data.mcs[mcs].functional) continue;
		for (mca = 0; mca < MCA_PER_MCS; mca++) {
			if (!mem_data.mcs[mcs].mca[mca].functional) continue;

			rdimm_data_t *dimm = mem_data.mcs[mcs].mca[mca].dimm;
			uint32_t val0, val1, common;
			int min;	/* Minimum compatible with both DIMMs is the bigger value */

			/* CAS Latency */
			val0 = dimm[0].present ? le32_to_cpu(*(uint32_t *)&dimm[0].spd[20]) : -1;
			val1 = dimm[1].present ? le32_to_cpu(*(uint32_t *)&dimm[1].spd[20]) : -1;
			/* Assuming both DIMMs are in low CL range, true for all DDR4 speed bins */
			common = val0 & val1;

			/* tAAmin - minimum CAS latency time */
			min = find_min_mtb_ftb(dimm, 24, 123);
			while (min <= 36 && ((common >> (min - 7)) & 1) == 0)
				min++;

			if (min > 36) {
				/* Maybe just die() instead? */
				printk(BIOS_WARNING, "Cannot find CL supported by all DIMMs under MCS%d, MCA%d."
				       " Marking as nonfunctional.\n", mcs, mca);
				mark_nonfunctional(mcs, mca);
				continue;
			}

			mem_data.mcs[mcs].mca[mca].cl = min;

			/*
			 * There are also minimal values in Table 170 of JEDEC Standard No. 79-4C which
			 * probably should also be honored. Some of them (e.g. RRD) depend on the page
			 * size, which depends on DRAM width. On tested DIMM they are just right - it is
			 * either minimal legal value or rounded up to whole clock cycle. Can we rely on
			 * vendors to put sane values in SPD or do we have to check them for validity?
			 */

			/* Minimum CAS to CAS Delay Time, Same Bank Group */
			mem_data.mcs[mcs].mca[mca].nccd_l = find_min_mtb_ftb(dimm, 40, 117);

			/* Minimum Write to Read Time, Different Bank Group */
			mem_data.mcs[mcs].mca[mca].nwtr_s = find_min_multi_mtb(dimm, 44, 43, 0x0F, 8);

			/* Minimum Write to Read Time, Same Bank Group */
			mem_data.mcs[mcs].mca[mca].nwtr_l = find_min_multi_mtb(dimm, 45, 43, 0xF0, 4);

			/* Minimum Four Activate Window Delay Time */
			mem_data.mcs[mcs].mca[mca].nfaw = find_min_multi_mtb(dimm, 37, 36, 0x0F, 8);

			/* Minimum RAS to CAS Delay Time */
			mem_data.mcs[mcs].mca[mca].nrcd = find_min_mtb_ftb(dimm, 25, 122);

			/* Minimum Row Precharge Delay Time */
			mem_data.mcs[mcs].mca[mca].nrp = find_min_mtb_ftb(dimm, 26, 121);

			/* Minimum Active to Precharge Delay Time */
			mem_data.mcs[mcs].mca[mca].nras = find_min_multi_mtb(dimm, 28, 27, 0x0F, 8);

			/* Minimum Write Recovery Time */
			mem_data.mcs[mcs].mca[mca].nwr = find_min_multi_mtb(dimm, 42, 41, 0x0F, 8);

			/* Minimum Activate to Activate Delay Time, Different Bank Group */
			mem_data.mcs[mcs].mca[mca].nrrd_s = find_min_mtb_ftb(dimm, 38, 119);

			/* Minimum Activate to Activate Delay Time, Same Bank Group */
			mem_data.mcs[mcs].mca[mca].nrrd_l = find_min_mtb_ftb(dimm, 39, 118);

			/* Minimum Refresh Recovery Delay Time */
			/* Assuming no fine refresh mode */
			mem_data.mcs[mcs].mca[mca].nrfc = find_min_multi_mtb(dimm, 30, 31, 0xFF, 8);

			printk(BIOS_SPEW, "MCS%d, MCA%d times (in clock cycles):\n", mcs, mca);
			dump_mca_data(&mem_data.mcs[mcs].mca[mca]);
		}
	}
}

void main(void)
{
	console_init();

	vpd_pnor_main();

	prepare_dimm_data();

	report_istep(13,1);	// no-op
	istep_13_2();
	istep_13_3();
	istep_13_4();
	report_istep(13,5);	// no-op
	istep_13_6();
	report_istep(13,7);	// no-op

	run_ramstage();
}
