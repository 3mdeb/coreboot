/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
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
		IOM0.DDRPHY_DP16_RD_VREF_CAL_ERROR_P0_{0-4},      // 0x8000007A0701103F, +0x0400_0000_0000
		IOM0.DDRPHY_DP16_WR_ERROR0_P0_{0-4},              // 0x8000001B0701103F, +0x0400_0000_0000
		IOM0.DDRPHY_DP16_RD_STATUS0_P0_{0-4},             // 0x800000140701103F, +0x0400_0000_0000
		IOM0.DDRPHY_DP16_RD_LVL_STATUS2_P0_{0-4},         // 0x800000100701103F, +0x0400_0000_0000
		IOM0.DDRPHY_DP16_RD_LVL_STATUS0_P0_{0-4},         // 0x8000000E0701103F, +0x0400_0000_0000
		IOM0.DDRPHY_DP16_WR_VREF_ERROR0_P0_{0-4},         // 0x800000AE0701103F, +0x0400_0000_0000
		IOM0.DDRPHY_DP16_WR_VREF_ERROR1_P0_{0-4},         // 0x800000AF0701103F, +0x0400_0000_0000
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

/* Based on ATTR_MSS_MRW_RESET_DELAY_BEFORE_CAL, by default do it. */
static void dp16_reset_delay_values(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	/* Make a const copy to help the compiler with optimizations. */
	const int ranks[2] = {mca->dimm[0].mranks, mca->dimm[1].mranks};
	int dp;

	/*
	 * It iterates over enabled rank pairs. See 13.8 for where these "pairs"
	 * (which may have up to 4 elements) were set.
	 *
	 * Because mem_data is initialized to 0, we can take shortcuts here and test
	 * for number of ranks directly.
	 */
	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_DQS_GATE_DELAY_RP0_P0_{0-4} = 0 */
		if (ranks[0] >= 1)
			dp_mca_and_or(id, dp, mca_i, 0x800000130701103F, 0, 0);
		/* IOM0.DDRPHY_DP16_DQS_GATE_DELAY_RP1_P0_{0-4} = 0 */
		if (ranks[0] > 1)
			dp_mca_and_or(id, dp, mca_i, 0x800001130701103F, 0, 0);
		/* IOM0.DDRPHY_DP16_DQS_GATE_DELAY_RP2_P0_{0-4} = 0 */
		if (ranks[1] >= 1)
			dp_mca_and_or(id, dp, mca_i, 0x800002130701103F, 0, 0);
		/* IOM0.DDRPHY_DP16_DQS_GATE_DELAY_RP3_P0_{0-4} = 0 */
		if (ranks[1] > 1)
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

	report_istep(13,11);

	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		if (!mem_data.mcs[mcs_i].functional)
			continue;

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (!mca->functional)
				continue;

			for (dimm = 0; dimm < DIMMS_PER_MCA; dimm++) {
				if (!mca->dimm[dimm].present)
					continue;

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
			dp16_reset_delay_values(mcs_i, mca_i);
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
				if ((rp == 0 && mca->dimm[0].mranks < 1) ||
				    (rp == 1 && mca->dimm[0].mranks < 2) ||
				    (rp == 2 && mca->dimm[1].mranks < 1) ||
				    (rp == 3 && mca->dimm[1].mranks < 2))
					continue;

				// TBD
			}
		}
	}

	printk(BIOS_EMERG, "ending istep 13.11\n");
}
