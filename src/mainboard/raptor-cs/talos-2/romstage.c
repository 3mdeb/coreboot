/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/vpd.h>
#include <cpu/power/istep_13.h>
#include <cpu/power/istep_14.h>
#include <program_loading.h>
#include <lib.h>	// hexdump
#include <spd_bin.h>
#include <endian.h>

mcbist_data_t mem_data;

static void dump_mca_data(mca_data_t *mca)
{
	printk(BIOS_SPEW, "\tCL =      %d\n", mca->cl);
	printk(BIOS_SPEW, "\tCCD_L =   %d\n", mca->nccd_l);
	printk(BIOS_SPEW, "\tWTR_S =   %d\n", mca->nwtr_s);
	printk(BIOS_SPEW, "\tWTR_L =   %d\n", mca->nwtr_l);
	printk(BIOS_SPEW, "\tFAW =     %d\n", mca->nfaw);
	printk(BIOS_SPEW, "\tRCD =     %d\n", mca->nrcd);
	printk(BIOS_SPEW, "\tRP =      %d\n", mca->nrp);
	printk(BIOS_SPEW, "\tRAS =     %d\n", mca->nras);
	printk(BIOS_SPEW, "\tWR =      %d\n", mca->nwr);
	printk(BIOS_SPEW, "\tRRD_S =   %d\n", mca->nrrd_s);
	printk(BIOS_SPEW, "\tRRD_L =   %d\n", mca->nrrd_l);
	printk(BIOS_SPEW, "\tRFC =     %d\n", mca->nrfc);
	printk(BIOS_SPEW, "\tRFC_DLR = %d\n", mca->nrfc_dlr);

	int i;
	for (i = 0; i < 2; i++) {
		if (mca->dimm[i].present) {
			printk(BIOS_SPEW, "\tDIMM%d: %dRx%d ", i, mca->dimm[i].mranks,
			       (mca->dimm[i].width + 1) * 4);

			if (mca->dimm[i].log_ranks != mca->dimm[i].mranks)
				printk(BIOS_SPEW, "%dH 3DS ", mca->dimm[i].log_ranks / mca->dimm[i].mranks);

			printk(BIOS_SPEW, "%dGB\n", (1 << (mca->dimm[i].density - 2)) *
			       mca->dimm[i].log_ranks * (2 - mca->dimm[i].width));
		}
		else
			printk(BIOS_SPEW, "\tDIMM%d: not installed\n", i);
	}
}

/* TODO: add checks for same ranks configuration for both DIMMs under one MCA */
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
	 * TODO: check if we can have different frequencies across MCSs.
	 */
	for (i = 0; i < CONFIG_DIMM_MAX; i++) {
		if (is_proper_dimm(blk.spd_array[i], i)) {
			mcs = i / DIMMS_PER_MCS;
			mca = (i % DIMMS_PER_MCS) / MCA_PER_MCS;
			int dimm_idx = i % 2;	// (i % DIMMS_PER_MCS) % MCA_PER_MCS


			/* Maximum for 2 DIMMs on one port (channel, MCA) is 2400 MT/s */
			if (tckmin < 0x07 && mem_data.mcs[mcs].mca[mca].functional)
				tckmin = 0x07;

			mem_data.mcs[mcs].functional = true;
			mem_data.mcs[mcs].mca[mca].functional = true;

			rdimm_data_t *dimm = &mem_data.mcs[mcs].mca[mca].dimm[dimm_idx];

			dimm->present = true;
			dimm->spd = blk.spd_array[i];
			/* RCD address is the same as SPD, with one additional bit set */
			dimm->rcd_i2c_addr = blk.addr_map[i] | 0x08;
			/*
			 * SPD fields in spd.h are not compatible with DDR4 and those in
			 * spd_bin.h are just a few of all required.
			 *
			 * TODO: add fields that are lacking to either of those files or
			 * add a file specific to DDR4 SPD.
			 */
			dimm->width = blk.spd_array[i][12] & 7;
			dimm->mranks = ((blk.spd_array[i][12] >> 3) & 0x7) + 1;
			dimm->log_ranks = dimm->mranks * (((blk.spd_array[i][6] >> 4) & 0x7) + 1);
			dimm->density = blk.spd_array[i][4] & 0xF;

			if ((blk.spd_array[i][5] & 0x38) > 0x20)
				die("DIMMs with more than 16 row address bits are not supported\n");

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
			/* Assuming no fine refresh mode. */
			mem_data.mcs[mcs].mca[mca].nrfc = find_min_multi_mtb(dimm, 30, 31, 0xFF, 8);

			/* Minimum Refresh Recovery Delay Time for Different Logical Rank (3DS only) */
			/*
			 * This one is set per MCA, but it depends on DRAM density, which can be
			 * mixed between DIMMs under the same channel. We need to choose the bigger
			 * minimum time, which corresponds to higher density.
			 *
			 * Assuming no fine refresh mode.
			 */
			val0 = dimm[0].present ? dimm[0].spd[4] & 0xF : 0;
			val1 = dimm[1].present ? dimm[1].spd[4] & 0xF : 0;
			min = (val0 < val1) ? val1 : val0;

			switch (min) {
				case 0x4:
					mem_data.mcs[mcs].mca[mca].nrfc_dlr = ns_to_nck(90);
					break;
				case 0x5:
					mem_data.mcs[mcs].mca[mca].nrfc_dlr = ns_to_nck(120);
					break;
				case 0x6:
					mem_data.mcs[mcs].mca[mca].nrfc_dlr = ns_to_nck(185);
					break;
				default:
					die("Unsupported DRAM density\n");
			}

			printk(BIOS_SPEW, "MCS%d, MCA%d times (in clock cycles):\n", mcs, mca);
			dump_mca_data(&mem_data.mcs[mcs].mca[mca]);
		}
	}
}

#include <cpu/power/scom.h>
#include <cpu/power/scom_registers.h>

#define X0_ENABLED (true)
#define X1_ENABLED (true)
#define X2_ENABLED (true)
#define X0_IS_PAIRED (true)
#define X1_IS_PAIRED (true)
#define X2_IS_PAIRED (true)
#define DD2X_PARTS (true)
#define OPTICS_IS_A_BUS (true)
#define ATTR_PROC_FABRIC_A_LINKS_CNFG (0)
#define ATTR_PROC_FABRIC_X_LINKS_CNFG (0)

#define EPS_TYPE_LE (0x01),
#define EPS_TYPE_HE (0x02),
#define EPS_TYPE_HE_F8 (0x03)
#define ATTR_PROC_EPS_TABLE_TYPE (EPS_TYPE_LE) // default value

#define CHIP_IS_NODE (0x01)
#define CHIP_IS_GROUP (0x02)
#define ATTR_PROC_FABRIC_PUMP_MODE (CHIP_IS_NODE) // default value

#define FBC_IOE_TL_FIR_ACTION0 (0x0000000000000000ULL)
#define FBC_IOE_TL_FIR_ACTION1 (0x0049000000000000ULL)

#define FBC_IOE_TL_FIR_MASK (0xFF24F0303FFFF11FULL)
#define P9_FBC_UTILS_MAX_ELECTRICAL_LINKS (3)
#define FBC_IOE_TL_FIR_MASK_X0_NF (0x00C00C0C00000880ULL)
#define FBC_IOE_TL_FIR_MASK_X1_NF (0x0018030300000440ULL)
#define FBC_IOE_TL_FIR_MASK_X2_NF (0x000300C0C0000220ULL)
#define PU_PB_IOE_FIR_MASK_REG (0x5013403)

#define FBC_IOE_DL_FIR_ACTION0 (0)
#define FBC_IOE_DL_FIR_ACTION1 (0x303c00000001ffc)
#define FBC_IOE_DL_FIR_MASK (0xfcfc3fffffffe003)

#define BOTH (0x0)
#define EVEN_ONLY (0x1)
#define ODD_ONLY (0x2)
#define NONE (0x3)
#define ATTR_LINK_TRAIN BOTH

// this is probably wrong
#define PROC_CHIPLET (0x10)

void p9_fbc_ioe_tl_scom(void);
void istep89(void);
void p9_fbc_no_hp_scom(void);
void tl_fir(void);
void p9_fbc_ioe_dl_scom(uint64_t TGT0);

void istep89(void)
{
	printk(BIOS_EMERG, "starting istep 8.9\n");
	p9_fbc_no_hp_scom();
    p9_fbc_ioe_tl_scom();

    p9_fbc_ioe_dl_scom(XB_CHIPLET_ID);
    write_scom_for_chiplet(XB_CHIPLET_ID, XBUS_LL0_IOEL_FIR_ACTION0_REG, FBC_IOE_DL_FIR_ACTION0);
    write_scom_for_chiplet(XB_CHIPLET_ID, XBUS_LL0_IOEL_FIR_ACTION1_REG, FBC_IOE_DL_FIR_ACTION1);
    write_scom_for_chiplet(XB_CHIPLET_ID, XBUS_LL0_LL0_LL0_IOEL_FIR_MASK_REG, FBC_IOE_DL_FIR_MASK);
	printk(BIOS_EMERG, "ending istep 8.9\n");
}

void p9_fbc_ioe_dl_scom(uint64_t TGT0)
{   
    uint64_t pb_ioe_ll0_ioel_config = read_scom_for_chiplet(TGT0, ELL_CFG_REG);
    uint64_t pb_ioe_ll0_ioel_replay_threshold = read_scom_for_chiplet(TGT0, ELL_REPLAY_TRESHOLD_REG);
    uint64_t pb_ioe_ll0_ioel_sl_ecc_threshold = read_scom_for_chiplet(TGT0, ELL_SL_ECC_TRESHOLD_REG);
    if (ATTR_LINK_TRAIN == BOTH)
    {
        pb_ioe_ll0_ioel_config |= 0x8000000000000000;
    }
    else
    {
        pb_ioe_ll0_ioel_config &= 0x7FFFFFFFFFFFFFFF;
    }
    pb_ioe_ll0_ioel_config &= 0xFFEFFFFFFFFFFFFF;
    pb_ioe_ll0_ioel_config |= 0x280F000F00000000;
    pb_ioe_ll0_ioel_replay_threshold &= 0x0FFFFFFFFFFFFFFF;
    pb_ioe_ll0_ioel_replay_threshold |= 0x6FE0000000000000;
    pb_ioe_ll0_ioel_sl_ecc_threshold &= 0x7FFFFFFFFFFFFFFF;
    pb_ioe_ll0_ioel_sl_ecc_threshold |= 0x7F70000000000000;

    write_scom_for_chiplet(TGT0, ELL_CFG_REG, pb_ioe_ll0_ioel_config);
    write_scom_for_chiplet(TGT0, ELL_REPLAY_TRESHOLD_REG, pb_ioe_ll0_ioel_replay_threshold);
    write_scom_for_chiplet(TGT0, ELL_SL_ECC_TRESHOLD_REG, pb_ioe_ll0_ioel_sl_ecc_threshold);
}

void tl_fir(void)
{
    write_scom_for_chiplet(PROC_CHIPLET, PU_PB_IOE_FIR_ACTION0_REG, FBC_IOE_TL_FIR_ACTION0);
    write_scom_for_chiplet(PROC_CHIPLET, PU_PB_IOE_FIR_ACTION1_REG, FBC_IOE_TL_FIR_ACTION1);
    uint64_t l_fir_mask = FBC_IOE_TL_FIR_MASK;

    l_fir_mask |= FBC_IOE_TL_FIR_MASK_X0_NF;
    // bool l_x_functional[P9_FBC_UTILS_MAX_ELECTRICAL_LINKS] =
    // {
    //     false,
    //     false,
    //     false
    // };
    // uint64_t l_x_non_functional_mask[P9_FBC_UTILS_MAX_ELECTRICAL_LINKS] =
    // {
    //     FBC_IOE_TL_FIR_MASK_X0_NF,
    //     FBC_IOE_TL_FIR_MASK_X1_NF,
    //     FBC_IOE_TL_FIR_MASK_X2_NF
    // };
    // uint8_t l_unit_pos;
    // l_unit_pos = fapi2::ATTR_CHIP_UNIT_POS[XB_CHIPLET_ID];
    // l_x_functional[l_unit_pos] = true;
    // for(unsigned int ll = 0; ll < P9_FBC_UTILS_MAX_ELECTRICAL_LINKS; ++ll)
    // {
    //     if (!l_x_functional[ll])
    //     {
    //         l_fir_mask |= l_x_non_functional_mask[ll];
    //     }
    // }
    write_scom_for_chiplet(PROC_CHIPLET, PU_PB_IOE_FIR_MASK_REG, l_fir_mask);
}

void p9_fbc_no_hp_scom(void)
{
    uint64_t is_flat_8 = false;

    // REGISTERS read
    uint64_t pb_com_pb_west_mode = read_scom_for_chiplet(PROC_CHIPLET, PB_WEST_MODE_CFG);
    uint64_t pb_com_pb_cent_mode = read_scom_for_chiplet(PROC_CHIPLET, PB_CENT_MODE_CFG);
    uint64_t pb_com_pb_cent_gp_cmd_rate_dp0 = read_scom_for_chiplet(PROC_CHIPLET, PB_CENT_GP_COMMAND_RATE_DP0);
    uint64_t pb_com_pb_cent_gp_cmd_rate_dp1 = read_scom_for_chiplet(PROC_CHIPLET, PB_CENT_GP_COMMAND_RATE_DP1);
    uint64_t pb_com_pb_cent_rgp_cmd_rate_dp0 = read_scom_for_chiplet(PROC_CHIPLET, PB_CENT_RGP_COMMAND_RATE_DP0);
    uint64_t pb_com_pb_cent_rgp_cmd_rate_dp1 = read_scom_for_chiplet(PROC_CHIPLET, PB_CENT_RGP_COMMAND_RATE_DP1);
    uint64_t pb_com_pb_cent_sp_cmd_rate_dp0 = read_scom_for_chiplet(PROC_CHIPLET, PB_CENT_SP_COMMAND_RATE_DP0);
    uint64_t pb_com_pb_cent_sp_cmd_rate_dp1 = read_scom_for_chiplet(PROC_CHIPLET, PB_CENT_SP_COMMAND_RATE_DP1);
    uint64_t pb_com_pb_east_mode = read_scom_for_chiplet(PROC_CHIPLET, PB_EAST_MODE_CFG);

    if (ATTR_PROC_FABRIC_PUMP_MODE == CHIP_IS_GROUP
    || (ATTR_PROC_FABRIC_PUMP_MODE != CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG == 0))
    {
        pb_com_pb_cent_gp_cmd_rate_dp0 = 0;
        pb_com_pb_cent_gp_cmd_rate_dp1 = 0;
    }
    else if (ATTR_PROC_FABRIC_PUMP_MODE != CHIP_IS_GROUP
          && ATTR_PROC_FABRIC_X_LINKS_CNFG > 0 && ATTR_PROC_FABRIC_X_LINKS_CNFG < 3)
    {
        pb_com_pb_cent_gp_cmd_rate_dp0 = 0x030406171C243448;
        pb_com_pb_cent_gp_cmd_rate_dp1 = 0x040508191F283A50;
    }
    else if (ATTR_PROC_FABRIC_PUMP_MODE != CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG > 2)
    {
        pb_com_pb_cent_gp_cmd_rate_dp0 = 0x0304062832405C80;
        pb_com_pb_cent_gp_cmd_rate_dp1 = 0x0405082F3B4C6D98;
    }

    if (ATTR_PROC_FABRIC_PUMP_MODE == CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG == 0)
    {
        pb_com_pb_cent_rgp_cmd_rate_dp0 = 0;
        pb_com_pb_cent_rgp_cmd_rate_dp1 = 0;
        pb_com_pb_cent_sp_cmd_rate_dp0 = 0;
        pb_com_pb_cent_sp_cmd_rate_dp1 = 0;
    }
    else if (ATTR_PROC_FABRIC_PUMP_MODE == CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG > 0 && ATTR_PROC_FABRIC_X_LINKS_CNFG < 3)
    {
        pb_com_pb_cent_rgp_cmd_rate_dp0 = 0x030406080A0C1218;
        pb_com_pb_cent_rgp_cmd_rate_dp1 = 0x040508080A0C1218;
        pb_com_pb_cent_sp_cmd_rate_dp0 = 0x030406080A0C1218;
        pb_com_pb_cent_sp_cmd_rate_dp1 = 0x030406080A0C1218;
    }
    else if ((ATTR_PROC_FABRIC_PUMP_MODE == CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG == 3)
          || (ATTR_PROC_FABRIC_PUMP_MODE != CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG == 0))
    {
        pb_com_pb_cent_rgp_cmd_rate_dp0 = 0x0304060D10141D28;
        pb_com_pb_cent_rgp_cmd_rate_dp1 = 0x0405080D10141D28;
        pb_com_pb_cent_sp_cmd_rate_dp0 = 0x05070A0D10141D28;
        pb_com_pb_cent_sp_cmd_rate_dp1 = 0x05070A0D10141D28;
    }
    else if ((ATTR_PROC_FABRIC_PUMP_MODE == CHIP_IS_GROUP && is_flat_8)
          || (ATTR_PROC_FABRIC_PUMP_MODE != CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG > 0 && ATTR_PROC_FABRIC_X_LINKS_CNFG < 3))
    {
        pb_com_pb_cent_rgp_cmd_rate_dp0 = 0x030406171C243448;
        pb_com_pb_cent_rgp_cmd_rate_dp1 = 0x040508191F283A50;
        pb_com_pb_cent_sp_cmd_rate_dp0 = 0x080C12171C243448;
        pb_com_pb_cent_sp_cmd_rate_dp1 = 0x0A0D14191F283A50;
    }
    else if (ATTR_PROC_FABRIC_PUMP_MODE != CHIP_IS_GROUP && ATTR_PROC_FABRIC_X_LINKS_CNFG > 2)
    {
        pb_com_pb_cent_rgp_cmd_rate_dp0 = 0x0304062832405C80;
        pb_com_pb_cent_rgp_cmd_rate_dp1 = 0x0405082F3B4C6D98;
        pb_com_pb_cent_sp_cmd_rate_dp0 = 0x08141F2832405C80;
        pb_com_pb_cent_sp_cmd_rate_dp1 = 0x0A18252F3B4C6D98;
    }

    if (ATTR_PROC_FABRIC_X_LINKS_CNFG == 0 && ATTR_PROC_FABRIC_A_LINKS_CNFG == 0)
    {
        pb_com_pb_east_mode |= 0x0300000000000000;
    }
    else
    {
        pb_com_pb_east_mode &= 0xF1FFFFFFFFFFFFFF;
    }
    if (ATTR_PROC_FABRIC_PUMP_MODE != CHIP_IS_GROUP && is_flat_8)
    {
        pb_com_pb_west_mode &= 0xFFFF0003FFFFFFFF;
        pb_com_pb_west_mode |= 0x00003E8000000000;
        pb_com_pb_cent_mode &= 0xFFFF0003FFFFFFFF;
        pb_com_pb_cent_mode |= 0x00003E8000000000;
        pb_com_pb_east_mode &= 0xFFFF0003FFFFFFFF;
        pb_com_pb_east_mode |= 0x00003E8000000000;
    }
    else
    {
        pb_com_pb_west_mode &= 0xFFFF0003FFFFFFFF;
        pb_com_pb_west_mode |= 0x0000FAFC00000000;
        pb_com_pb_cent_mode &= 0xFFFF0003FFFFFFFF;
        pb_com_pb_cent_mode |= 0x00007EFC00000000;
        pb_com_pb_east_mode &= 0xFFFF0003FFFFFFFF;
        pb_com_pb_east_mode |= 0x000007EFC0000000;
    }

    pb_com_pb_west_mode &= 0xFFFFFFFC0FFFFFFF;
    pb_com_pb_west_mode |= 0x00000002a0000000;
    pb_com_pb_cent_mode &= 0xFFFFFFFC0FFFFFFF;
    pb_com_pb_cent_mode |= 0x00000002a0000000;
    pb_com_pb_east_mode &= 0xFFFFFFFC0FFFFFFF;
    pb_com_pb_east_mode |= 0x00000002a0000000;

    // REGISTERS write
    write_scom_for_chiplet(PROC_CHIPLET, PB_WEST_MODE_CFG, pb_com_pb_west_mode);
    write_scom_for_chiplet(PROC_CHIPLET, PB_CENT_MODE_CFG, pb_com_pb_cent_mode);
    write_scom_for_chiplet(PROC_CHIPLET, PB_CENT_GP_COMMAND_RATE_DP0, pb_com_pb_cent_gp_cmd_rate_dp0);
    write_scom_for_chiplet(PROC_CHIPLET, PB_CENT_GP_COMMAND_RATE_DP1, pb_com_pb_cent_gp_cmd_rate_dp1);
    write_scom_for_chiplet(PROC_CHIPLET, PB_CENT_RGP_COMMAND_RATE_DP0, pb_com_pb_cent_rgp_cmd_rate_dp0);
    write_scom_for_chiplet(PROC_CHIPLET, PB_CENT_RGP_COMMAND_RATE_DP1, pb_com_pb_cent_rgp_cmd_rate_dp1);
    write_scom_for_chiplet(PROC_CHIPLET, PB_CENT_SP_COMMAND_RATE_DP0, pb_com_pb_cent_sp_cmd_rate_dp0);
    write_scom_for_chiplet(PROC_CHIPLET, PB_CENT_SP_COMMAND_RATE_DP1, pb_com_pb_cent_sp_cmd_rate_dp1);
    write_scom_for_chiplet(PROC_CHIPLET, PB_EAST_MODE_CFG, pb_com_pb_east_mode);
}

void p9_fbc_ioe_tl_scom(void)
{
	uint64_t pb_ioe_scom_pb_fp01_cfg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_FRAMER_PARSER_01_CFG);
    uint64_t pb_ioe_scom_pb_fp23_cfg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_FRAMER_PARSER_23_CFG);
    uint64_t pb_ioe_scom_pb_fp45_cfg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_FRAMER_PARSER_45_CFG);
    uint64_t pb_ioe_scom_pb_elink_data_01_cfg_reg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_DATA_BUFF_01_CFG);
    uint64_t pb_ioe_scom_pb_elink_data_23_cfg_reg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_DATA_BUFF_23_CFG);
    uint64_t pb_ioe_scom_pb_elink_data_45_cfg_reg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_DATA_BUFF_45_CFG);
    uint64_t pb_ioe_scom_pb_misc_cfg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_MISC_CFG);
    uint64_t pb_ioe_scom_pb_trace_cfg = read_scom_for_chiplet(PROC_CHIPLET, PB_ELE_LINK_TRACE_CFG);

	if(X0_ENABLED)
	{
		pb_ioe_scom_pb_fp01_cfg &= 0xfff004fffff007bf;
        pb_ioe_scom_pb_fp01_cfg |= 0x0002010000020000;
	}
	else
	{
		pb_ioe_scom_pb_fp01_cfg |= 0x84000000840;
	}

	// if(X0_ENABLED && DD2X_PARTS)
	// {
	// 	PB_IOE_SCOM_PB_FP01_CFG.insert<4, 8, 56, uint64_t>(0x15 - (l_def_DD2_LO_LIMIT_N / l_def_DD2_LO_LIMIT_D))
    //     PB_IOE_SCOM_PB_FP01_CFG.insert<36, 8, 56, uint64_t>(0x15 - (l_def_DD2_LO_LIMIT_N / l_def_DD2_LO_LIMIT_D))
	// }
	// else if ((X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R < DD1_LO_LIMIT_H)
    //       || (X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R == DD1_LO_LIMIT_H && DD1_LO_LIMIT_P))
	// {
	// 	PB_IOE_SCOM_PB_FP01_CFG.insert<4, 8, 56, uint64_t>(0x1A - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
    //     PB_IOE_SCOM_PB_FP01_CFG.insert<36, 8, 56, uint64_t>(0x1A - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
	// }
	// else if ((X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R == DD1_LO_LIMIT_H && !DD1_LO_LIMIT_P)
    //       || (X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R > DD1_LO_LIMIT_H))
    // {
    //     PB_IOE_SCOM_PB_FP01_CFG.insert<4, 8, 56, uint64_t>(0x19 - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
    //     PB_IOE_SCOM_PB_FP01_CFG.insert<36, 8, 56, uint64_t>(0x19 - (l_def_DD1_LO_LIMIT_N / l_def_DD1_LO_LIMIT_D))
    // }
	if(X2_ENABLED)
    {
        pb_ioe_scom_pb_fp45_cfg &= 0xfff004bffff007bf;
        pb_ioe_scom_pb_fp45_cfg |= 0x0002010000020000;
    }
    else
    {
        pb_ioe_scom_pb_fp45_cfg |= 0x84000000840;
    }
	// if (X2_ENABLED && DD2X_PARTS)
    // {
    //     pb_ioe_scom_pb_fp45_cfg.insert<4, 8, 56, uint64_t>(0x15 - (l_def_dd2_lo_limit_n /    ))
    //     pb_ioe_scom_pb_fp45_cfg.insert<36, 8, 56, uint64_t>(0x15 - (l_def_dd2_lo_limit_n / l_def_dd2_lo_limit_d))
    // }
    // else if ((X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R < DD1_LO_LIMIT_H)
    //       || (X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R == DD1_LO_LIMIT_H && DD1_LO_LIMIT_P))
    // {
    //     pb_ioe_scom_pb_fp45_cfg.insert<4, 8, 56, uint64_t>(0x1A - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D))
    //     pb_ioe_scom_pb_fp45_cfg.insert<36, 8, 56, uint64_t>(0x1A - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D))
    // }
    // else if ((X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R == DD1_LO_LIMIT_H && !DD1_LO_LIMIT_P)
    //       || (X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R > DD1_LO_LIMIT_H))
    // {
    //     pb_ioe_scom_pb_fp45_cfg.insert<4, 8, 56, uint64_t>(0x19 - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D))
    //     pb_ioe_scom_pb_fp45_cfg.insert<36, 8, 56, uint64_t>(0x19 - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D))
    // }

    if (X0_ENABLED && OPTICS_IS_A_BUS)
    {
        pb_ioe_scom_pb_elink_data_01_cfg_reg &= 0xFFFFFF07FFFFFFFF;
        pb_ioe_scom_pb_elink_data_01_cfg_reg |= 0x0000001000000000;
    }
    else if(X0_ENABLED)
    {
        pb_ioe_scom_pb_elink_data_01_cfg_reg |= 0x0000001F00000000;
    }

    if (X0_ENABLED)
    {
        pb_ioe_scom_pb_elink_data_01_cfg_reg &= 0x80FFFFFF80FFFFFF;
        pb_ioe_scom_pb_elink_data_01_cfg_reg |= 0x4000000040000000;

        pb_ioe_scom_pb_elink_data_01_cfg_reg &= 0xFF8080FFFF8080FF;
        pb_ioe_scom_pb_elink_data_01_cfg_reg |= 0x003C3C00003C3C00;
        
        pb_ioe_scom_pb_trace_cfg &= 0x0000FFFFFFFFFFFF;
        pb_ioe_scom_pb_trace_cfg |= 0x4141000000000000;
    }
    else if (X1_ENABLED)
    {
        pb_ioe_scom_pb_trace_cfg &= 0xFFFF0000FFFFFFFF;
        pb_ioe_scom_pb_trace_cfg |= 0x0000414100000000;
    }
    else if (X2_ENABLED)
    {
        pb_ioe_scom_pb_trace_cfg &= 0xFFFFFFFF0000FFFF;
        pb_ioe_scom_pb_trace_cfg |= 0x0000000041410000;
    }

    // if (X1_ENABLED && DD2X_PARTS)
    // {
    //     PB_IOE_SCOM_PB_FP23_CFG.insert<4, 8, 56, uint64_t>(0x15 - (DD2_LO_LIMIT_N / DD2_LO_LIMIT_D));
    //     PB_IOE_SCOM_PB_FP23_CFG.insert<36, 8, 56, uint64_t>(0x15 - (DD2_LO_LIMIT_N / DD2_LO_LIMIT_D));
    // }
    // else if ((X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R < DD1_LO_LIMIT_H)
    //       || (X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R == DD1_LO_LIMIT_H && DD1_LO_LIMIT_P))
    // {
    //     PB_IOE_SCOM_PB_FP23_CFG.insert<4, 8, 56, uint64_t>(0x1A - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D));
    //     PB_IOE_SCOM_PB_FP23_CFG.insert<36, 8, 56, uint64_t>(0x1A - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D));
    // }
    // else if ((X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R == DD1_LO_LIMIT_H && !DD1_LO_LIMIT_P)
    //       || (X0_ENABLED && DD1_PARTS && DD1_LO_LIMIT_R > DD1_LO_LIMIT_H))
    // {
    //     PB_IOE_SCOM_PB_FP23_CFG.insert<4, 8, 56, uint64_t>(0x19 - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D));
    //     PB_IOE_SCOM_PB_FP23_CFG.insert<36, 8, 56, uint64_t>(0x19 - (DD1_LO_LIMIT_N / DD1_LO_LIMIT_D));
    // }


    if(X1_ENABLED)
    {
        pb_ioe_scom_pb_fp23_cfg &= 0xfff004bffff007bf;
        pb_ioe_scom_pb_fp23_cfg |= 0x0002010000020000;
        if (OPTICS_IS_A_BUS)
        {
            pb_ioe_scom_pb_elink_data_23_cfg_reg &= 0xFE0FFFFFFFFFFFFF;
            pb_ioe_scom_pb_elink_data_23_cfg_reg |= 0x0100000000000000;
        }
        else
        {
            pb_ioe_scom_pb_elink_data_23_cfg_reg &= 0xFE0FFFFFFFFFFFFF;
            pb_ioe_scom_pb_elink_data_23_cfg_reg |= 0x01F0000000000000;
        }
        pb_ioe_scom_pb_elink_data_23_cfg_reg &= 0x808080FF808080FF;
        pb_ioe_scom_pb_elink_data_23_cfg_reg |= 0x403C3C00403C3C00;
    }
    else
    {
        pb_ioe_scom_pb_fp23_cfg |= 0x0000084000000840;
    }

    if(X2_ENABLED)
    {
        if (OPTICS_IS_A_BUS)
        {
            pb_ioe_scom_pb_elink_data_45_cfg_reg &= 0xFFFFFF07FFFFFFFF;
            pb_ioe_scom_pb_elink_data_45_cfg_reg |= 0x0000008000000000;
        }
        else
        {
            pb_ioe_scom_pb_elink_data_45_cfg_reg |= 0x000000F800000000;
        }
        pb_ioe_scom_pb_elink_data_45_cfg_reg &= 0x808080FF808080FF;
        pb_ioe_scom_pb_elink_data_45_cfg_reg |= 0x403C3C00403C3C00;
    }

    if(X0_IS_PAIRED)
    {
        pb_ioe_scom_pb_misc_cfg |= 0x8000000000000000;
    }
    else
    {
        pb_ioe_scom_pb_misc_cfg &= 0x7FFFFFFFFFFFFFFF;
    }
    if(X1_IS_PAIRED)
    {
        pb_ioe_scom_pb_misc_cfg |= 0x4000000000000000;
    }
    else
    {
        pb_ioe_scom_pb_misc_cfg &= 0xBFFFFFFFFFFFFFFF;
    }
    if(X2_IS_PAIRED)
    {
        pb_ioe_scom_pb_misc_cfg |= 0x2000000000000000;
    }
    else
    {
        pb_ioe_scom_pb_misc_cfg &= 0xDFFFFFFFFFFFFFFF;
    }

    // REGISTERS write
    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_FRAMER_PARSER_01_CFG, pb_ioe_scom_pb_fp01_cfg);
    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_FRAMER_PARSER_23_CFG, pb_ioe_scom_pb_fp23_cfg);
    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_FRAMER_PARSER_45_CFG, pb_ioe_scom_pb_fp45_cfg);
    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_DATA_BUFF_01_CFG, pb_ioe_scom_pb_elink_data_01_cfg_reg);
    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_DATA_BUFF_23_CFG, pb_ioe_scom_pb_elink_data_23_cfg_reg);
    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_PB_DATA_BUFF_45_CFG, pb_ioe_scom_pb_elink_data_45_cfg_reg);
    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_MISC_CFG, pb_ioe_scom_pb_misc_cfg);
    write_scom_for_chiplet(PROC_CHIPLET, PB_ELE_LINK_TRACE_CFG, pb_ioe_scom_pb_trace_cfg);
}

void main(void)
{
	console_init();
<<<<<<< HEAD

	vpd_pnor_main();

	prepare_dimm_data();

	report_istep(13,1);	// no-op
	istep_13_2();
	istep_13_3();
	istep_13_4();
	report_istep(13,5);	// no-op
	istep_13_6();
	report_istep(13,7);	// no-op
	istep_13_8();
	istep_13_9();
	istep_13_10();
	istep_13_11();
	report_istep(13,12);	// optional, not yet implemented
	istep_13_13();
	istep_14_2();

	/* Test if SCOM still works. Maybe should check also indirect access? */
	printk(BIOS_DEBUG, "0xF000F = %llx\n", read_scom(0xf000f));

	/*
	 * Halt to give a chance to inspect FIRs, otherwise checkstops from
	 * ramstage may cover up the failure in romstage.
	 */
	if (read_scom(0xf000f) != 0x223d104900008040)
		die("SCOM stopped working, check FIRs, halting now\n");

=======
	istep89();
>>>>>>> src/mainboard/raptor-cs/talos-2/romstage.c: scom_init initial
	run_ramstage();
}
