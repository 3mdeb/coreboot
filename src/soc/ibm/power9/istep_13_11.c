/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <cpu/power/vpd_data.h>
#include <console/console.h>
#include <timer.h>

static void setup_and_execute_zqcal(int mcs_i, int mca_i, int d)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	int mirrored = mca->dimm[d].spd[136] & 1; /* Maybe add this to mca_data_t? */
	mrs_cmd_t cmd = ddr4_get_zqcal_cmd(DDR4_ZQCAL_LONG);
	enum rank_selection ranks;

	if (d == 0) {
		if (mca->dimm[d].mranks == 2)
			ranks = DIMM0_ALL_RANKS;
		else
			ranks = DIMM0_RANK0;
	}
	else {
		if (mca->dimm[d].mranks == 2)
			ranks = DIMM1_ALL_RANKS;
		else
			ranks = DIMM1_RANK0;
	}

	/*
	 * JEDEC: "All banks must be precharged and tRP met before ZQCL or ZQCS
	 * commands are issued by the controller" - not sure if this is ensured.
	 * A refresh during the calibration probably would impact the results. Also,
	 * "No other activities should be performed on the DRAM channel by the
	 * controller for the duration of tZQinit, tZQoper, or tZQCS" - this means
	 * we have to insert a delay after every ZQCL, not only after the last one.
	 * As a possible improvement, perhaps we could reorder this step a bit and
	 * send ZQCL on all ports "simultaneously" (without delays) and add a delay
	 * just between different DIMMs/ranks, but those delays cannot be done by
	 * CCS and we don't have a timer with enough precision to make it worth the
	 * effort.
	 */
	ccs_add_mrs(id, cmd, ranks, mirrored, tZQinit);
	ccs_execute(id, mca_i);
}

static void clear_initial_cal_errors(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	for (dp = 0; dp < 5; dp++) {
		/* Whole lot of zeroing
		IOM0.DDRPHY_DP16_RD_VREF_CAL_ERROR_P0_{0-4},
		IOM0.DDRPHY_DP16_WR_ERROR0_P0_{0-4},
		IOM0.DDRPHY_DP16_RD_STATUS0_P0_{0-4},
		IOM0.DDRPHY_DP16_RD_LVL_STATUS2_P0_{0-4},
		IOM0.DDRPHY_DP16_RD_LVL_STATUS0_P0_{0-4},
		IOM0.DDRPHY_DP16_WR_VREF_ERROR0_P0_{0-4},
		IOM0.DDRPHY_DP16_WR_VREF_ERROR1_P0_{0-4},
			[all] 0
		*/
		dp_mca_and_or(id, dp, mca_i, 0x8000007A0701103F, 0, 0);	/* RD_VREF_CAL_ERROR */
		dp_mca_and_or(id, dp, mca_i, 0x8000001B0701103F, 0, 0);	/* WR_ERROR0 */
		dp_mca_and_or(id, dp, mca_i, 0x800000140701103F, 0, 0);	/* RD_STATUS0 */
		dp_mca_and_or(id, dp, mca_i, 0x800000100701103F, 0, 0);	/* RD_LVL_STATUS2 */
		dp_mca_and_or(id, dp, mca_i, 0x8000000E0701103F, 0, 0);	/* RD_LVL_STATUS0 */
		dp_mca_and_or(id, dp, mca_i, 0x800000AE0701103F, 0, 0);	/* WR_VREF_ERROR0 */
		dp_mca_and_or(id, dp, mca_i, 0x800000AF0701103F, 0, 0);	/* WR_VREF_ERROR1 */
	}

	/* IOM0.DDRPHY_APB_CONFIG0_P0 =
		[49]  RESET_ERR_RPT = 1, then 0
	*/
	mca_and_or(id, mca_i, 0x8000D0000701103F, ~0, PPC_BIT(49));
	mca_and_or(id, mca_i, 0x8000D0000701103F, ~PPC_BIT(49), 0);

	/* IOM0.DDRPHY_APB_ERROR_STATUS0_P0 =
		[all] 0
	*/
	mca_and_or(id, mca_i, 0x8000D0010701103F, 0, 0);

	/* IOM0.DDRPHY_RC_ERROR_STATUS0_P0 =
		[all] 0
	*/
	mca_and_or(id, mca_i, 0x8000C8050701103F, 0, 0);

	/* IOM0.DDRPHY_SEQ_ERROR_STATUS0_P0 =
		[all] 0
	*/
	mca_and_or(id, mca_i, 0x8000C4080701103F, 0, 0);

	/* IOM0.DDRPHY_WC_ERROR_STATUS0_P0 =
		[all] 0
	*/
	mca_and_or(id, mca_i, 0x8000CC030701103F, 0, 0);

	/* IOM0.DDRPHY_PC_ERROR_STATUS0_P0 =
		[all] 0
	*/
	mca_and_or(id, mca_i, 0x8000C0120701103F, 0, 0);

	/* IOM0.DDRPHY_PC_INIT_CAL_ERROR_P0 =
		[all] 0
	*/
	mca_and_or(id, mca_i, 0x8000C0180701103F, 0, 0);

	/* IOM0.IOM_PHY0_DDRPHY_FIR_REG =
		[56]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_2 = 0
	*/
	mca_and_or(id, mca_i, 0x07011000, ~PPC_BIT(56), 0);
}

/* TODO: make non-void, return status and use as a test for repetition */
static void dump_cal_errors(int mcs_i, int mca_i)
{
#if CONFIG(DEBUG_RAM_SETUP)
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	for (dp = 0; dp < 5; dp++) {
		printk(BIOS_ERR, "DP %d\n", dp);
		printk(BIOS_ERR, "\t%#16.16llx - RD_VREF_CAL_ERROR\n",
		       dp_mca_read(id, dp, mca_i, 0x8000007A0701103F));
		printk(BIOS_ERR, "\t%#16.16llx - WR_ERROR0\n",
		       dp_mca_read(id, dp, mca_i, 0x8000001B0701103F));
		printk(BIOS_ERR, "\t%#16.16llx - RD_STATUS0\n",
		       dp_mca_read(id, dp, mca_i, 0x800000140701103F));
		printk(BIOS_ERR, "\t%#16.16llx - RD_LVL_STATUS2\n",
		       dp_mca_read(id, dp, mca_i, 0x800000100701103F));
		printk(BIOS_ERR, "\t%#16.16llx - RD_LVL_STATUS0\n",
		       dp_mca_read(id, dp, mca_i, 0x8000000E0701103F));
		printk(BIOS_ERR, "\t%#16.16llx - WR_VREF_ERROR0\n",
		       dp_mca_read(id, dp, mca_i, 0x800000AE0701103F));
		printk(BIOS_ERR, "\t%#16.16llx - WR_VREF_ERROR1\n",
		       dp_mca_read(id, dp, mca_i, 0x800000AF0701103F));
	}

	printk(BIOS_ERR, "%#16.16llx - APB_ERROR_STATUS0\n",
	       mca_read(id, mca_i, 0x8000D0010701103F));

	printk(BIOS_ERR, "%#16.16llx - RC_ERROR_STATUS0\n",
	       mca_read(id, mca_i, 0x8000C8050701103F));

	printk(BIOS_ERR, "%#16.16llx - SEQ_ERROR_STATUS0\n",
	       mca_read(id, mca_i, 0x8000C4080701103F));

	printk(BIOS_ERR, "%#16.16llx - WC_ERROR_STATUS0\n",
	       mca_read(id, mca_i, 0x8000CC030701103F));

	printk(BIOS_ERR, "%#16.16llx - PC_ERROR_STATUS0\n",
	       mca_read(id, mca_i, 0x8000C0120701103F));

	printk(BIOS_ERR, "%#16.16llx - PC_INIT_CAL_ERROR\n",
	       mca_read(id, mca_i, 0x8000C0180701103F));

	/* 0x8000 on success */
	printk(BIOS_ERR, "%#16.16llx - DDRPHY_PC_INIT_CAL_STATUS\n",
	       mca_read(id, mca_i, 0x8000C0190701103F));

	printk(BIOS_ERR, "%#16.16llx - IOM_PHY0_DDRPHY_FIR_REG\n",
	       mca_read(id, mca_i, 0x07011000));
#endif
}

/* Based on ATTR_MSS_MRW_RESET_DELAY_BEFORE_CAL, by default do it. */
static void dp16_reset_delay_values(int mcs_i, int mca_i, enum rank_selection ranks_present)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	/*
	 * It iterates over enabled rank pairs. See 13.8 for where these "pairs"
	 * (which may have up to 4 elements) were set.
	 */
	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_DQS_GATE_DELAY_RP0_P0_{0-4} = 0 */
		if (ranks_present & DIMM0_RANK0)
			dp_mca_and_or(id, dp, mca_i, 0x800000130701103F, 0, 0);
		/* IOM0.DDRPHY_DP16_DQS_GATE_DELAY_RP1_P0_{0-4} = 0 */
		if (ranks_present & DIMM0_RANK1)
			dp_mca_and_or(id, dp, mca_i, 0x800001130701103F, 0, 0);
		/* IOM0.DDRPHY_DP16_DQS_GATE_DELAY_RP2_P0_{0-4} = 0 */
		if (ranks_present & DIMM1_RANK0)
			dp_mca_and_or(id, dp, mca_i, 0x800002130701103F, 0, 0);
		/* IOM0.DDRPHY_DP16_DQS_GATE_DELAY_RP3_P0_{0-4} = 0 */
		if (ranks_present & DIMM1_RANK1)
			dp_mca_and_or(id, dp, mca_i, 0x800003130701103F, 0, 0);
	}
}

static void dqs_align_turn_on_refresh(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];

	/* IOM0.DDRPHY_SEQ_MEM_TIMING_PARAM0_P0
	// > May need to add freq/tRFI attr dependency later but for now use this value
	// > Provided by Ryan King
	    [60-63] TRFC_CYCLES = 9             // tRFC = 2^9 = 512 memcycles
	*/
	mca_and_or(id, mca_i, 0x8000C4120701103F, ~PPC_BITMASK(60, 63),
	           PPC_SHIFT(9, 63));

	/* IOM0.DDRPHY_PC_INIT_CAL_CONFIG1_P0
	    // > Hard coded settings provided by Ryan King for this workaround
	    [48-51] REFRESH_COUNT =     0xf
	    // TODO: see "Read clock align - pre-workaround" below. Why not 1 until
	    // calibration finishes? Does it pull in refresh commands?
	    [52-53] REFRESH_CONTROL =   3       // refresh commands may interrupt calibration routines
	    [54]    REFRESH_ALL_RANKS = 1
	    [55]    CMD_SNOOP_DIS =     0
	    [57-63] REFRESH_INTERVAL =  0x13    // Worst case: 6.08us for 1866 (max tCK). Must be not more than 7.8us
	*/
	mca_and_or(id, mca_i, 0x8000C0170701103F,
	           ~(PPC_BITMASK(48, 55) | PPC_BITMASK(57, 63)),
	           PPC_SHIFT(0xF, 51) | PPC_SHIFT(3, 53) | PPC_BIT(54) |
	           PPC_SHIFT(0x13, 63));
}

static void wr_level_pre(int mcs_i, int mca_i, int rp,
                         enum rank_selection ranks_present)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	int d = rp / 2;
	int vpd_idx = (mca->dimm[d].mranks - 1) * 2 + (!!mca->dimm[d ^ 1].present);
	int mirrored = mca->dimm[d].spd[136] & 1;
	mrs_cmd_t mrs;
	enum rank_selection rank = NO_RANKS;
	int i;

	/*
	 * JEDEC specification requires disabling RTT_WR during WR_LEVEL, and
	 * enabling equivalent terminations.
	 */
	if (ATTR_MSS_VPD_MT_DRAM_RTT_WR[vpd_idx] != 0) {
		/*
		 * This block is done after MRS commands in Hostboot, but we need rank
		 * selection and this is the easiest way to get it. We also do not call
		 * ccs_execute() until the end of this function.
		 */
		switch (rp) {
		case 0:
			rank = DIMM0_RANK0;
			/* IOM0.DDRPHY_SEQ_ODT_WR_CONFIG0_P0 =
			  [48] = 1
			*/
			mca_and_or(id, mca_i, 0x8000C40A0701103F, ~0, PPC_BIT(48));
			break;
		case 1:
			rank = DIMM0_RANK1;
			/* IOM0.DDRPHY_SEQ_ODT_WR_CONFIG0_P0 =
			  [57] = 1
			*/
			mca_and_or(id, mca_i, 0x8000C40A0701103F, ~0, PPC_BIT(57));
			break;
		case 2:
			rank = DIMM1_RANK0;
			/* IOM0.DDRPHY_SEQ_ODT_WR_CONFIG1_P0 =
			  [50] = 1
			*/
			mca_and_or(id, mca_i, 0x8000C40B0701103F, ~0, PPC_BIT(50));
			break;
		case 3:
			rank = DIMM1_RANK1;
			/* IOM0.DDRPHY_SEQ_ODT_WR_CONFIG1_P0 =
			  [59] = 1
			*/
			mca_and_or(id, mca_i, 0x8000C40B0701103F, ~0, PPC_BIT(59));
			break;
		}

		/* MR2 =               // redo the rest of the bits
		  [A11-A9]  0
		*/
		mrs = ddr4_get_mr2(DDR4_MR2_WR_CRC_DISABLE,
                           vpd_to_rtt_wr(0),
                           DDR4_MR2_ASR_MANUAL_EXTENDED_RANGE,
                           mem_data.cwl);
		ccs_add_mrs(id, mrs, rank, mirrored, tMRD);

		/* MR1 =               // redo the rest of the bits
		  // Write properly encoded RTT_WR value as RTT_NOM
		  [A8-A10]  240/ATTR_MSS_VPD_MT_DRAM_RTT_WR
		*/
		mrs = ddr4_get_mr1(DDR4_MR1_QOFF_ENABLE,
                           mca->dimm[d].width == WIDTH_x8 ? DDR4_MR1_TQDS_ENABLE :
                                                            DDR4_MR1_TQDS_DISABLE,
                           vpd_to_rtt_nom(ATTR_MSS_VPD_MT_DRAM_RTT_WR[vpd_idx]),
                           DDR4_MR1_WRLVL_DISABLE,
                           DDR4_MR1_ODIMP_RZQ_7,
                           DDR4_MR1_AL_DISABLE,
                           DDR4_MR1_DLL_ENABLE);
		/*
		 * Next command for this rank should be enabling write leveling, done by
		 * PHY hardware, so use tMRD.
		 *
		 * There are possible MRS commands to be send to other ranks, maybe we
		 * can subtract those. On the other hand, with microsecond precision for
		 * delays in ccs_execute(), this probably doesn't matter anyway.
		 */
		ccs_add_mrs(id, mrs, rank, mirrored, tMRD);

		// mss::workarounds::seq::odt_config();		// Not needed on DD2

	}

	/* Different workaround, executed even if RTT_WR == 0 */
	/* workarounds::wr_lvl::configure_non_calibrating_ranks()
	for each rank on MCA except current primary rank:
	  MR1 =               // redo the rest of the bits
		  [A7]  = 1       // Write Leveling Enable
		  [A12] = 1       // Outputs disabled (DQ, DQS)
	*/
	for (i = 0; i < 4; i++) {
		rank = 1 << i;
		if (i == rp || !(ranks_present & rank))
			continue;

		/*
		 * VPD index stays the same (DIMM mixing rules), but I'm not sure about
		 * mirroring. Better safe than sorry, assume mirrored and non-mirrored
		 * DIMMs can be mixed.
		 */
		mirrored = mca->dimm[i/2].spd[136] & 1;

		mrs = ddr4_get_mr1(DDR4_MR1_QOFF_DISABLE,
                           mca->dimm[d].width == WIDTH_x8 ? DDR4_MR1_TQDS_ENABLE :
                                                            DDR4_MR1_TQDS_DISABLE,
                           vpd_to_rtt_nom(ATTR_MSS_VPD_MT_DRAM_RTT_NOM[vpd_idx]),
                           DDR4_MR1_WRLVL_ENABLE,
                           DDR4_MR1_ODIMP_RZQ_7,
                           DDR4_MR1_AL_DISABLE,
                           DDR4_MR1_DLL_ENABLE);
		/*
		 * Delays apply to commands sent to the same rank, but we are changing
		 * ranks. Can we get away with 0 delay? Is it worth it? Remember that
		 * the same delay is currently used between sides of RCD.
		 */
		ccs_add_mrs(id, mrs, rank, mirrored, tMRD);
	}

	/* TODO: maybe drop it, next ccs_phy_hw_step() would call it anyway. */
	//ccs_execute(id, mca_i);
}

static uint64_t wr_level_time(void)
{
	/*
	 * "Note: the following equation is taken from the PHY workbook - leaving
	 * the naked numbers in for parity to the workbook
	 *
	 * This step runs for approximately (80 + TWLO_TWLOE) x NUM_VALID_SAMPLES x
	 * (384/(BIG_STEP + 1) + (2 x (BIG_STEP + 1))/(SMALL_STEP + 1)) + 20 memory
	 * clock cycles per rank."
	 *
	 * TWLO_TWLOE for every defined speed bin is 9.5 + 2 = 11.5 ns, this needs
	 * to be converted to clock cycles, it is the only non-constant component of
	 * the equation.
	 *
	 * WR_LVL_BIG_STEP = 7
	 * WR_LVL_SMALL_STEP = 0
	 * WR_LVL_NUM_VALID_SAMPLES = 5
	 */
	const int BIG_STEP = 7;
	const int SMALL_STEP = 0;
	const int NUM_VALID_SAMPLES = 5;
	const int TWLO_TWLOE = ps_to_nck(11500);

	return (80 + TWLO_TWLOE) * NUM_VALID_SAMPLES * (384 / (BIG_STEP + 1) +
	       (2 * (BIG_STEP + 1)) / (SMALL_STEP + 1)) + 20;
}

/* Undo the pre-workaround, basically */
static void wr_level_post(int mcs_i, int mca_i, int rp,
                          enum rank_selection ranks_present)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	int d = rp / 2;
	int vpd_idx = (mca->dimm[d].mranks - 1) * 2 + (!!mca->dimm[d ^ 1].present);
	int mirrored = mca->dimm[d].spd[136] & 1;
	mrs_cmd_t mrs;
	enum rank_selection rank = NO_RANKS;
	int i;

	/*
	 * JEDEC specification requires disabling RTT_WR during WR_LEVEL, and
	 * enabling equivalent terminations.
	 */
	if (ATTR_MSS_VPD_MT_DRAM_RTT_WR[vpd_idx] != 0) {
		/*
		 * This block is done after MRS commands in Hostboot, but we need rank
		 * selection and this is the easiest way to get it. We also do not call
		 * ccs_execute() until the end of this function.
		 */
		#define F(x) ((((x) >> 4) & 0xc) | (((x) >> 2) & 0x3))
		/* Originally done in seq_reset() in 13.8 */
		/* IOM0.DDRPHY_SEQ_ODT_RD_CONFIG1_P0 =
			  F(X) = (((X >> 4) & 0xc) | ((X >> 2) & 0x3))    // Bits 0,1,4,5 of X, see also MC01.PORT0.SRQ.MBA_FARB2Q
			  [all]   0
			  [48-51] ODT_RD_VALUES0 =
				count_dimm(MCA) == 2: F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][1][0])
				count_dimm(MCA) != 2: F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][0][2])
			  [56-59] ODT_RD_VALUES1 =
				count_dimm(MCA) == 2: F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][1][1])
				count_dimm(MCA) != 2: F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][0][3])
		*/
		/* 2 DIMMs -> odd vpd_idx */
		uint64_t val = 0;
		if (vpd_idx % 2)
			val = PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][1][0]), 51) |
				  PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][1][1]), 59);

		mca_and_or(id, mca_i, 0x8000C40F0701103F, 0, val);


		/* IOM0.DDRPHY_SEQ_ODT_WR_CONFIG0_P0 =
			  F(X) = (((X >> 4) & 0xc) | ((X >> 2) & 0x3))    // Bits 0,1,4,5 of X, see also MC01.PORT0.SRQ.MBA_FARB2Q
			  [all]   0
			  [48-51] ODT_WR_VALUES0 = F(ATTR_MSS_VPD_MT_ODT_WR[index(MCA)][0][0])
			  [56-59] ODT_WR_VALUES1 = F(ATTR_MSS_VPD_MT_ODT_WR[index(MCA)][0][1])
		*/
		mca_and_or(id, mca_i, 0x8000C40A0701103F, 0,
				   PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][0][0]), 51) |
				   PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][0][1]), 59));
		#undef F

		/* MR2 =               // redo the rest of the bits
		  [A11-A9]  0
		*/
		mrs = ddr4_get_mr2(DDR4_MR2_WR_CRC_DISABLE,
                           vpd_to_rtt_wr(ATTR_MSS_VPD_MT_DRAM_RTT_WR[vpd_idx]),
                           DDR4_MR2_ASR_MANUAL_EXTENDED_RANGE,
                           mem_data.cwl);
		ccs_add_mrs(id, mrs, rank, mirrored, tMRD);

		/* MR1 =               // redo the rest of the bits
		  // Write properly encoded RTT_WR value as RTT_NOM
		  [A8-A10]  240/ATTR_MSS_VPD_MT_DRAM_RTT_WR
		*/
		mrs = ddr4_get_mr1(DDR4_MR1_QOFF_ENABLE,
                           mca->dimm[d].width == WIDTH_x8 ? DDR4_MR1_TQDS_ENABLE :
                                                            DDR4_MR1_TQDS_DISABLE,
                           vpd_to_rtt_nom(ATTR_MSS_VPD_MT_DRAM_RTT_NOM[vpd_idx]),
                           DDR4_MR1_WRLVL_DISABLE,
                           DDR4_MR1_ODIMP_RZQ_7,
                           DDR4_MR1_AL_DISABLE,
                           DDR4_MR1_DLL_ENABLE);
		/*
		 * Next command for this rank should be Initial Pattern Write, done by
		 * PHY hardware, so use tMRD.
		 */
		ccs_add_mrs(id, mrs, rank, mirrored, tMRD);

		// mss::workarounds::seq::odt_config();		// Not needed on DD2

	}

	/* Different workaround, executed even if RTT_WR == 0 */
	/* workarounds::wr_lvl::configure_non_calibrating_ranks()
	for each rank on MCA except current primary rank:
	  MR1 =               // redo the rest of the bits
		  [A7]  = 1       // Write Leveling Enable
		  [A12] = 1       // Outputs disabled (DQ, DQS)
	*/
	for (i = 0; i < 4; i++) {
		rank = 1 << i;
		if (i == rp || !(ranks_present & rank))
			continue;

		/*
		 * VPD index stays the same (DIMM mixing rules), but I'm not sure about
		 * mirroring. Better safe than sorry, assume mirrored and non-mirrored
		 * DIMMs can be mixed.
		 */
		mirrored = mca->dimm[i/2].spd[136] & 1;

		mrs = ddr4_get_mr1(DDR4_MR1_QOFF_ENABLE,
                           mca->dimm[d].width == WIDTH_x8 ? DDR4_MR1_TQDS_ENABLE :
                                                            DDR4_MR1_TQDS_DISABLE,
                           vpd_to_rtt_nom(ATTR_MSS_VPD_MT_DRAM_RTT_NOM[vpd_idx]),
                           DDR4_MR1_WRLVL_DISABLE,
                           DDR4_MR1_ODIMP_RZQ_7,
                           DDR4_MR1_AL_DISABLE,
                           DDR4_MR1_DLL_ENABLE);
		/*
		 * Delays apply to commands sent to the same rank, but we are changing
		 * ranks. Can we get away with 0 delay? Is it worth it? Remember that
		 * the same delay is currently used between sides of RCD.
		 */
		ccs_add_mrs(id, mrs, rank, mirrored, tMRD);
	}

	/* TODO: maybe drop it, next ccs_phy_hw_step() would call it anyway. */
	//ccs_execute(id, mca_i);
}

static uint64_t initial_pat_wr_time(void)
{
	/*
	 * "Not sure how long this should take, so we're gonna use 1 to make sure we
	 * get at least one polling loop"
	 *
	 * Hostboot polls every 10 us, but in coreboot this value results in minimal
	 * delay of 2 us (one microsecond for delay_nck() and another for wait_us()
	 * in ccs_execute()). Tests show that it is not enough.
	 *
	 * What has to be done to write pattern to MPR in general:
	 * - write to MR3 to enable MPR access (tMOD)
	 * - write to MPRs (tWR_MPR for back-to-back writes, there are 4 MPRs;
	 *   tWR_MPR is tMOD + AL + PL, but AL and PL is 0 here)
	 * - write to MR3 to disable MPR access (tMOD or tMRD, depending on what is
	 *   the next command).
	 *
	 * This gives 6 * tMOD, but because there is RCD with sides A and B this is
	 * 12 * tMOD = 288 nCK. However, we have to add to calculations refresh
	 * commands, as set in dqs_align_turn_on_refresh() - 15 commands, each takes
	 * 512 nCK. This is kind of consistent for 2666 MT/s DIMM with 5 us I've
	 * seen in tests.
	 *
	 * There is no limit about how many refresh commands can be issued (as long
	 * as tRFC isn't violated), but only 8 of them are "pulling in" further
	 * refreshes, meaning that DRAM will survive 9*tREFI without a refresh
	 * (8 pulled in and 1 regular interval) - this is useful for longer
	 * calibration steps. Another 9*tREFI can be postponed - REF commands are
	 * sent after a longer pause, but this (probably) isn't relevant here.
	 *
	 * There may be more refreshes sent in the middle of the most of steps due
	 * to REFRESH_CONTROL setting.
	 *
	 * These additional cycles should be added to all calibration steps. I don't
	 * think they are included in Hostboot, then again I don't know what exactly
	 * is added in "equations taken from the PHY workbook". This may be the
	 * reason why Hostboot multiplies every timeout by 4 AND assumes worst case
	 * wherever possible AND polls so rarely.
	 *
	 * From the lack of better ideas, return 10 us.
	 */
	return ns_to_nck(10 * 1000);
}

static uint64_t dqs_align_time(void)
{
	/*
	 * "This step runs for approximately 6 x 600 x 4 DRAM clocks per rank pair."
	 *
	 * TODO: check if this is enough, now step fails for a different reason.
	 */
	return 6 * 600 * 4;
}

static void rdclk_align_pre(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];

	/*
	 * TODO: we just set it before starting calibration steps. As we don't have
	 * any precious data in RAM yet, maybe we can use 0 there and just change it
	 * to 3 in the post-workaround?
	 */

	/* Turn off refresh, we don't want it to interfere here
	IOM0.DDRPHY_PC_INIT_CAL_CONFIG1_P0
		[52-53] REFRESH_CONTROL =   0       // refresh commands are only sent at start of initial calibration
	*/
	mca_and_or(id, mca_i, 0x8000C0170701103F, ~PPC_BITMASK(52, 53), 0);
}

static uint64_t rdclk_align_time(void)
{
	/*
	 * "This step runs for approximately 24 x ((1024/COARSE_CAL_STEP_SIZE +
	 * 4 x COARSE_CAL_STEP_SIZE) x 4 + 32) DRAM clocks per rank pair"
	 *
	 * COARSE_CAL_STEP_SIZE = 4
	 */
	const int COARSE_CAL_STEP_SIZE = 4;
	return 24 * ((1024/COARSE_CAL_STEP_SIZE + 4*COARSE_CAL_STEP_SIZE) * 4 + 32);
}

static void rdclk_align_post(int mcs_i, int mca_i, int rp)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;
	uint64_t val;
	const uint64_t mul = 0x0000010000000000;

	/*
	 * "In DD2.*, We adjust the red waterfall to account for low VDN settings.
	 * We move the waterfall forward by one"
	IOM0.DDRPHY_DP16_DQS_RD_PHASE_SELECT_RANK_PAIR{0-3}_P0_{0-3}      // 0x80000<rp>090701103F, +0x0400_0000_0000
	    [48-49] DQSCLK_SELECT0 = (++DQSCLK_SELECT0 % 4)
	    [52-53] DQSCLK_SELECT1 = (++DQSCLK_SELECT1 % 4)
	    [56-57] DQSCLK_SELECT2 = (++DQSCLK_SELECT2 % 4)
	    [60-61] DQSCLK_SELECT3 = (++DQSCLK_SELECT3 % 4)
	IOM0.DDRPHY_DP16_DQS_RD_PHASE_SELECT_RANK_PAIR{0-3}_P0_4          // 0x80001{0-3}090701103F
	    [48-49] DQSCLK_SELECT0 = (++DQSCLK_SELECT0 % 4)
	    [52-53] DQSCLK_SELECT1 = (++DQSCLK_SELECT1 % 4)
	    // Can't change non-existing quads
	*/
	for (dp = 0; dp < 4; dp++) {
		val = dp_mca_read(id, dp, mca_i, 0x800000090701103F + rp * mul);
		val += PPC_BIT(49) | PPC_BIT(53) | PPC_BIT(57) | PPC_BIT(61);
		val &= PPC_BITMASK(48, 49) | PPC_BITMASK(52, 53) | PPC_BITMASK(56, 57) |
		       PPC_BITMASK(60, 61);
		/* TODO: this can be done with just one read */
		dp_mca_and_or(id, dp, mca_i, 0x800000090701103F + rp * mul,
		              ~(PPC_BITMASK(48, 49) | PPC_BITMASK(52, 53) |
		                PPC_BITMASK(56, 57) | PPC_BITMASK(60, 61)),
		              val);
	}

	val = dp_mca_read(id, 4, mca_i, 0x800000090701103F + rp * mul);
	val += PPC_BIT(49) | PPC_BIT(53);
	val &= PPC_BITMASK(48, 49) | PPC_BITMASK(52, 53);
	dp_mca_and_or(id, dp, mca_i, 0x800000090701103F + rp * mul,
				  ~(PPC_BITMASK(48, 49) | PPC_BITMASK(52, 53)),
				  val);

	/* Turn on refresh */
	dqs_align_turn_on_refresh(mcs_i, mca_i);
}

/*
 * 13.11 mss_draminit_training: Dram training
 *
 * a) p9_mss_draminit_training.C (mcbist) -- Nimbus
 * b) p9c_mss_draminit_training.C (mba) -- Cumulus
 *    - Prior to running this procedure will apply known DQ bad bits to prevent
 *      them from participating in training. This information is extracted from
 *      the bad DQ attribute and applied to Hardware
 *      - Marks the calibration fail array
 *    - External ZQ Calibration
 *    - Execute initial dram calibration (7 step - handled by HW)
 *    - This procedure will update the bad DQ attribute for each dimm based on
 *      its findings
 */
void istep_13_11(void)
{
	printk(BIOS_EMERG, "starting istep 13.11\n");
	int mcs_i, mca_i, dimm, rp;
	enum rank_selection ranks_present;

	report_istep(13,11);

	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		if (!mem_data.mcs[mcs_i].functional)
			continue;

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (!mca->functional)
				continue;

			ranks_present = NO_RANKS;
			for (dimm = 0; dimm < DIMMS_PER_MCA; dimm++) {
				if (!mca->dimm[dimm].present)
					continue;

				if (mca->dimm[dimm].mranks == 2)
					ranks_present |= DIMM0_ALL_RANKS << (2 * dimm);
				else
					ranks_present |= DIMM0_RANK0 << (2 * dimm);

				setup_and_execute_zqcal(mcs_i, mca_i, dimm);
			}

			/* IOM0.DDRPHY_PC_INIT_CAL_CONFIG0_P0 = 0 */
			mca_and_or(mcs_ids[mcs_i], mca_i, 0x8000C0160701103F, 0, 0);

			/*
			 * > Disable port fails as it doesn't appear the MC handles initial
			 * > cal timeouts correctly (cal_length.) BRS, see conversation with
			 * > Brad Michael
			MC01.PORT0.SRQ.MBA_FARB0Q =                 // 0x07010913
				  [57]  MBA_FARB0Q_CFG_PORT_FAIL_DISABLE = 1
			*/
			mca_and_or(mcs_ids[mcs_i], mca_i, 0x07010913, ~0, PPC_BIT(57));

			/*
			 * > The following registers must be configured to the correct
			 * > operating environment:
			 * > These are reset in phy_scominit
			 * > Section 5.2.5.10 SEQ ODT Write Configuration {0-3} on page 422
			 * > Section 5.2.6.1 WC Configuration 0 Register on page 434
			 * > Section 5.2.6.2 WC Configuration 1 Register on page 436
			 * > Section 5.2.6.3 WC Configuration 2 Register on page 438
			 *
			 * It would be nice to have the documentation mentioned above or at
			 * least know what it is about...
			 */

			clear_initial_cal_errors(mcs_i, mca_i);
			dp16_reset_delay_values(mcs_i, mca_i, ranks_present);
			dqs_align_turn_on_refresh(mcs_i, mca_i);

			/*
			 * List of calibration steps for RDIMM, in execution order:
			 * - ZQ calibration - calibrates DRAM output driver and on-die termination
			 *   values (already done)
			 * - Write leveling - compensates for skew caused by a fly-by topology
			 * - Initial pattern write - not exactly a calibration, but prepares patterns
			 *   for next steps
			 * - DQS align
			 * - RDCLK align
			 * - Read centering
			 * - Write Vref latching - not exactly a calibration, but required for next
			 *   steps
			 * - Write centering
			 * - Coarse write/read
			 * - Custom read and/or write centering - performed in istep 13.12
			 * Some of these steps have pre- or post-workarounds, or both.
			 *
			 * All of those steps (except ZQ calibration) are executed for each rank pair
			 * before going to the next pair. Some of them require that there is no other
			 * activity on the controller so parallelization may not be possible.
			 *
			 * Quick reminder from set_rank_pairs() in 13.8 (RDIMM only):
			 * - RP0 primary - DIMM 0 rank 0
			 * - RP1 primary - DIMM 0 rank 1
			 * - RP2 primary - DIMM 1 rank 0
			 * - RP3 primary - DIMM 1 rank 1
			 */
			for (rp = 0; rp < 4; rp++) {
				if (!(ranks_present & (1 << rp)))
					continue;

				dump_cal_errors(mcs_i, mca_i);

				/* Write leveling */
				wr_level_pre(mcs_i, mca_i, rp, ranks_present);
				ccs_phy_hw_step(mcs_ids[mcs_i], mca_i, rp, CAL_WR_LEVEL,
				                wr_level_time());
				/*
				 * First pass returns errors for CLK16 and CLK22 for first DP16
				 * and CLK18 and CLK20 for second DP16. Second pass is able to
				 * level those properly.
				 */
				/* TODO: add a helper function for all steps */
				if (mca_read(mcs_ids[mcs_i], mca_i, 0x8000C0180701103F) != 0) {
					dump_cal_errors(mcs_i, mca_i);
					printk(BIOS_DEBUG, "First attempt returned error, "
					       "repeating write leveling\n");
					/*
					 * All registers except FIR reset to 0 when next calibration
					 * step is started, but if we try to reset FIR when other
					 * registers are non-0, error bit in FIR immediately gets
					 * set again.
					 */
					clear_initial_cal_errors(mcs_i, mca_i);
					ccs_phy_hw_step(mcs_ids[mcs_i], mca_i, rp, CAL_WR_LEVEL,
									wr_level_time());
					if (mca_read(mcs_ids[mcs_i], mca_i, 0x8000C0180701103F) != 0)
						die("Write leveling failed twice, aborting\n");
				}
				wr_level_post(mcs_i, mca_i, rp, ranks_present);

				dump_cal_errors(mcs_i, mca_i);


				/* Initial Pattern Write */
				/*
				 * Patterns were specified in IOM0.DDRPHY_SEQ_RD_WR_DATA{0,1}_P0
				 * in 13.8.
				 */
				ccs_phy_hw_step(mcs_ids[mcs_i], mca_i, rp, CAL_INITIAL_PAT_WR,
				                initial_pat_wr_time());

				dump_cal_errors(mcs_i, mca_i);


				/* DQS alignment */
				/*
				 * FIXME: during this step CCS goes crazy: first command gets
				 * written in the place of the last one, minus GOTO_CMD field,
				 * which is zeroed. This results in infinite loop and timeout.
				 * DDRPHY_PC_INIT_CAL_STATUS is 0x0000000000000688 - meaning
				 * there was an overflow of refresh pending counter, but whether
				 * it is a cause or a result of CCS error is beyond my current
				 * knowledge.
				 */
				ccs_phy_hw_step(mcs_ids[mcs_i], mca_i, rp, CAL_DQS_ALIGN,
				                dqs_align_time());
				if (mca_read(mcs_ids[mcs_i], mca_i, 0x8000C0180701103F) != 0) {
					dump_cal_errors(mcs_i, mca_i);
					printk(BIOS_DEBUG, "First attempt returned error, "
					       "repeating DQS alignment\n");

					clear_initial_cal_errors(mcs_i, mca_i);
					ccs_phy_hw_step(mcs_ids[mcs_i], mca_i, rp, CAL_DQS_ALIGN,
					                dqs_align_time());
					if (mca_read(mcs_ids[mcs_i], mca_i, 0x8000C0180701103F) != 0)
						die("DQS alignment failed twice, aborting\n");
				}
				/* Post-workaround does not apply to DD2.* */

				dump_cal_errors(mcs_i, mca_i);

				printk(BIOS_EMERG, "DQS alignment completed\n");

				/*
				 * When there are no workarounds, in theory we can chain steps:
				ccs_phy_hw_step(mcs_ids[mcs_i], mca_i, rp, CAL_INITIAL_PAT_WR | CAL_DQS_ALIGN,
				                initial_pat_wr_time() + dqs_align_time());
				 *
				 * However, focus on making them work first...
				 */

				/* Alignment of the internal SysClk to the Read clock */
				rdclk_align_pre(mcs_i, mca_i);
				ccs_phy_hw_step(mcs_ids[mcs_i], mca_i, rp, CAL_RDCLK_ALIGN,
				                rdclk_align_time());
				rdclk_align_post(mcs_i, mca_i, rp);
				// TBD
			}
		}
	}

	printk(BIOS_EMERG, "ending istep 13.11\n");
}
