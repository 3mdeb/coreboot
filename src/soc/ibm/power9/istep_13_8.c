/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <console/console.h>

#define ATTR_PG			0xE000000000000000ull
#define FREQ_PB_MHZ		1866

/*
 * This function was generated from initfiles. Some of the registers used here
 * are not documented, except for occasional name of a constant written to it.
 * They also access registers at addresses for chiplet ID = 5 (Nest west), even
 * though the specified target is MCA. It is not clear if MCA offset has to be
 * added to SCOM address for those registers or not. Undocumented registers are
 * marked with (?) in the comments.
 */
static void p9n_mca_scom(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	/*
	 * Mixing rules:
	 * - rank configurations are the same for both DIMMs
	 * - fields for unpopulated DIMMs are initialized to all 0
	 *
	 * With those two assumptions values can be logically ORed to produce a
	 * common value without conditionals.
	 */
	int n_dimms = (mca->dimm[0].present && mca->dimm[1].present) ? 2 : 1;
	int mranks = mca->dimm[0].mranks | mca->dimm[1].mranks;
	int log_ranks = mca->dimm[0].log_ranks | mca->dimm[1].log_ranks;
	bool is_8H = (log_ranks / mranks) == 8;
	chiplet_id_t nest = id == MC01_CHIPLET_ID ? N3_CHIPLET_ID : N1_CHIPLET_ID;

	/* P9N2_MCS_PORT02_MCPERF0 (?)
	    [22-27] = 0x20                              // AMO_LIMIT
	*/
	scom_and_or_for_chiplet(nest, 0x05010823, ~PPC_BITMASK(22,27), PPC_SHIFT(0x20, 27));

	/* P9N2_MCS_PORT02_MCPERF2 (?)
	    [0-2]   = 1                                 // PF_DROP_VALUE0
	    [3-5]   = 3                                 // PF_DROP_VALUE1
	    [6-8]   = 5                                 // PF_DROP_VALUE2
	    [9-11]  = 7                                 // PF_DROP_VALUE3
	    [13-15] =                                   // REFRESH_BLOCK_CONFIG
		if has only one DIMM in MCA:
		  0b000 : if master ranks = 1
		  0b001 : if master ranks = 2
		  0b100 : if master ranks = 4
		// Per allowable DIMM mixing rules, we cannot mix different number of ranks on any single port
		if has both DIMMs in MCA:
		  0b010 : if master ranks = 1
		  0b011 : if master ranks = 2
		  0b100 : if master ranks = 4           // 4 mranks is the same for one and two DIMMs in MCA
	    [16] =                                      // ENABLE_REFRESH_BLOCK_SQ
	    [17] =                                      // ENABLE_REFRESH_BLOCK_NSQ, always the same value as [16]
		1 : if (1 < (DIMM0 + DIMM1 logical ranks) <= 8 && not (one DIMM, 4 mranks, 2H 3DS)
		0 : otherwise
	    [18]    = 0                                 // ENABLE_REFRESH_BLOCK_DISP
	    [28-31] = 0b0100                            // SQ_LFSR_CNTL
	    [50-54] = 0b11100                           // NUM_RMW_BUF
	    [61] = ATTR_ENABLE_MEM_EARLY_DATA_SCOM      // EN_ALT_ECR_ERR, 0?
	*/
	uint64_t ref_blk_cfg = mranks == 4 ? 0x4 :
	                       mranks == 2 ? (n_dimms == 1 ? 0x1 : 0x3) :
	                       n_dimms == 1 ? 0x0 : 0x2;
	uint64_t en_ref_blk = (log_ranks <= 1 || log_ranks > 8) ? 0 :
	                      (n_dimms == 1 && mranks == 4 && log_ranks == 8) ? 0 : 3;

	scom_and_or_for_chiplet(nest, 0x05010824,
	                        /* and */
	                        ~(PPC_BITMASK(0,11) | PPC_BITMASK(13,18) | PPC_BITMASK(28,31)
	                          | PPC_BITMASK(28,31) | PPC_BITMASK(50,54) | PPC_BIT(61)),
	                        /* or */
	                        PPC_SHIFT(1, 2) | PPC_SHIFT(3, 5) | PPC_SHIFT(5, 8)
	                        | PPC_SHIFT(7, 11) /* PF_DROP_VALUEs */
	                        | PPC_SHIFT(ref_blk_cfg, 15) | PPC_SHIFT(en_ref_blk, 17)
	                        | PPC_SHIFT(0x4, 31) | PPC_SHIFT(0x1C, 54));

	/* P9N2_MCS_PORT02_MCAMOC (?)
	    [1]     = 0                                 // FORCE_PF_DROP0
	    [4-28]  = 0x19fffff                         // WRTO_AMO_COLLISION_RULES
	    [29-31] = 1                                 // AMO_SIZE_SELECT, 128B_RW_64B_DATA
	*/
	scom_and_or_for_chiplet(nest, 0x05010825, ~(PPC_BIT(1) | PPC_BITMASK(4,31)),
	                        PPC_SHIFT(0x19FFFFF, 28) | PPC_SHIFT(1, 31));

	/* P9N2_MCS_PORT02_MCEPSQ (?)
	    [0-7]   = 1                                 // JITTER_EPSILON
	    // ATTR_PROC_EPS_READ_CYCLES_T* are calculated in 8.6
	    [8-15]  = (ATTR_PROC_EPS_READ_CYCLES_T0 + 6) / 4        // LOCAL_NODE_EPSILON
	    [16-23] = (ATTR_PROC_EPS_READ_CYCLES_T1 + 6) / 4        // NEAR_NODAL_EPSILON
	    [24-31] = (ATTR_PROC_EPS_READ_CYCLES_T1 + 6) / 4        // GROUP_EPSILON
	    [32-39] = (ATTR_PROC_EPS_READ_CYCLES_T2 + 6) / 4        // REMOTE_NODAL_EPSILON
	    [40-47] = (ATTR_PROC_EPS_READ_CYCLES_T2 + 6) / 4        // VECTOR_GROUP_EPSILON
	 */
	scom_and_or_for_chiplet(nest, 0x05010826, ~PPC_BITMASK(0,47),
	                        PPC_SHIFT(1, 7) /* FIXME: fill the rest with non-hardcoded values*/
	                        | PPC_SHIFT(3, 15) | PPC_SHIFT(3, 23) | PPC_SHIFT(3, 31)
	                        | PPC_SHIFT(18, 39) | PPC_SHIFT(18, 47));
//~ static const uint32_t EPSILON_R_T0_LE[] = {    7,    7,    8,    8,   10,   22 };  // T0, T1
//~ static const uint32_t EPSILON_R_T2_LE[] = {   67,   69,   71,   74,   79,  103 };  // T2

	/* P9N2_MCS_PORT02_MCBUSYQ (?)
	    [0]     = 1                                 // ENABLE_BUSY_COUNTERS
	    [1-3]   = 1                                 // BUSY_COUNTER_WINDOW_SELECT, 1024 cycles
	    [4-13]  = 38                                // BUSY_COUNTER_THRESHOLD0
	    [14-23] = 51                                // BUSY_COUNTER_THRESHOLD1
	    [24-33] = 64                                // BUSY_COUNTER_THRESHOLD2
	*/
	scom_and_or_for_chiplet(nest, 0x05010827, ~PPC_BITMASK(0,33),
	                        PPC_BIT(0) | PPC_SHIFT(1, 3) | PPC_SHIFT(38, 13)
	                        | PPC_SHIFT(51, 23) | PPC_SHIFT(64, 33));

	/* P9N2_MCS_PORT02_MCPERF3 (?)
	    [31] = 1                                    // ENABLE_CL0
	    [41] = 1                                    // ENABLE_AMO_MSI_RMW_ONLY
	    [43] = !ATTR_ENABLE_MEM_EARLY_DATA_SCOM     // ENABLE_CP_M_MDI0_LOCAL_ONLY, !0 = 1?
	    [44] = 1                                    // DISABLE_WRTO_IG
	    [45] = 1                                    // AMO_LIMIT_SEL
	*/
	scom_and_or_for_chiplet(nest, 0x0501082B, ~PPC_BIT(43),
	                        PPC_BIT(31) | PPC_BIT(41) | PPC_BIT(44) | PPC_BIT(45));

	/* MC01.PORT0.SRQ.MBA_DSM0Q =
	    // These are set per port so all latencies should be calculated from both DIMMs (if present)
	    [0-5]   MBA_DSM0Q_CFG_RODT_START_DLY =  ATTR_EFF_DRAM_CL - ATTR_EFF_DRAM_CWL
	    [6-11]  MBA_DSM0Q_CFG_RODT_END_DLY =    ATTR_EFF_DRAM_CL - ATTR_EFF_DRAM_CWL + 5
	    [12-17] MBA_DSM0Q_CFG_WODT_START_DLY =  0
	    [18-23] MBA_DSM0Q_CFG_WODT_END_DLY =    5
	    [24-29] MBA_DSM0Q_CFG_WRDONE_DLY =      24
	    [30-35] MBA_DSM0Q_CFG_WRDATA_DLY =      ATTR_EFF_DRAM_CWL + ATTR_MSS_EFF_DPHY_WLO - 8
	    // Assume RDIMM, non-NVDIMM only
	    [36-41] MBA_DSM0Q_CFG_RDTAG_DLY =
		MSS_FREQ_EQ_1866:                   ATTR_EFF_DRAM_CL[l_def_PORT_INDEX] + 7
		MSS_FREQ_EQ_2133:                   ATTR_EFF_DRAM_CL[l_def_PORT_INDEX] + 7
		MSS_FREQ_EQ_2400:                   ATTR_EFF_DRAM_CL[l_def_PORT_INDEX] + 8
		MSS_FREQ_EQ_2666:                   ATTR_EFF_DRAM_CL[l_def_PORT_INDEX] + 9
	*/
	/* ATTR_MSS_EFF_DPHY_WLO = 1 (from VPD) */
	uint64_t rdtag_dly = mem_data.speed == 2666 ? 9 :
	                     mem_data.speed == 2400 ? 8 : 7;
	mca_and_or(id, mca_i, 0x0701090A, ~PPC_BITMASK(0,41),
	           PPC_SHIFT(mca->cl - mem_data.cwl, 5) |
	           PPC_SHIFT(mca->cl - mem_data.cwl + 5, 11) |
	           PPC_SHIFT(5, 23) | PPC_SHIFT(24, 29) |
	           PPC_SHIFT(mem_data.cwl + 1 - 8, 35) |
	           PPC_SHIFT(mca->cl + rdtag_dly, 41));

	/* MC01.PORT0.SRQ.MBA_TMR0Q =
	    [0-3]   MBA_TMR0Q_RRDM_DLY =
		MSS_FREQ_EQ_1866:             8
		MSS_FREQ_EQ_2133:             9
		MSS_FREQ_EQ_2400:             10
		MSS_FREQ_EQ_2666:             11
	    [4-7]   MBA_TMR0Q_RRSMSR_DLY =  4
	    [8-11]  MBA_TMR0Q_RRSMDR_DLY =  4
	    [12-15] MBA_TMR0Q_RROP_DLY =    ATTR_EFF_DRAM_TCCD_L
	    [16-19] MBA_TMR0Q_WWDM_DLY =
		MSS_FREQ_EQ_1866:             8
		MSS_FREQ_EQ_2133:             9
		MSS_FREQ_EQ_2400:             10
		MSS_FREQ_EQ_2666:             11
	    [20-23] MBA_TMR0Q_WWSMSR_DLY =  4
	    [24-27] MBA_TMR0Q_WWSMDR_DLY =  4
	    [28-31] MBA_TMR0Q_WWOP_DLY =    ATTR_EFF_DRAM_TCCD_L
	    [32-36] MBA_TMR0Q_RWDM_DLY =        // same as below
	    [37-41] MBA_TMR0Q_RWSMSR_DLY =      // same as below
	    [42-46] MBA_TMR0Q_RWSMDR_DLY =
		MSS_FREQ_EQ_1866:             ATTR_EFF_DRAM_CL - ATTR_EFF_DRAM_CWL + 8
		MSS_FREQ_EQ_2133:             ATTR_EFF_DRAM_CL - ATTR_EFF_DRAM_CWL + 9
		MSS_FREQ_EQ_2400:             ATTR_EFF_DRAM_CL - ATTR_EFF_DRAM_CWL + 10
		MSS_FREQ_EQ_2666:             ATTR_EFF_DRAM_CL - ATTR_EFF_DRAM_CWL + 11
	    [47-50] MBA_TMR0Q_WRDM_DLY =
		MSS_FREQ_EQ_1866:             ATTR_EFF_DRAM_CWL - ATTR_EFF_DRAM_CL + 8
		MSS_FREQ_EQ_2133:             ATTR_EFF_DRAM_CWL - ATTR_EFF_DRAM_CL + 9
		MSS_FREQ_EQ_2400:             ATTR_EFF_DRAM_CWL - ATTR_EFF_DRAM_CL + 10
		MSS_FREQ_EQ_2666:             ATTR_EFF_DRAM_CWL - ATTR_EFF_DRAM_CL + 11
	    [51-56] MBA_TMR0Q_WRSMSR_DLY =      // same as below
	    [57-62] MBA_TMR0Q_WRSMDR_DLY =    ATTR_EFF_DRAM_CWL + ATTR_EFF_DRAM_TWTR_S + 4
	*/
	uint64_t var_dly = mem_data.speed == 2666 ? 11 :
	                   mem_data.speed == 2400 ? 10 :
	                   mem_data.speed == 2133 ? 9 : 8;
	mca_and_or(id, mca_i, 0x0701090B, PPC_BIT(63),
	           PPC_SHIFT(var_dly, 3) | PPC_SHIFT(4, 7) | PPC_SHIFT(4, 11) |
	           PPC_SHIFT(mca->nccd_l, 15) | PPC_SHIFT(var_dly, 19) |
	           PPC_SHIFT(4, 23) | PPC_SHIFT(4, 27) | PPC_SHIFT(mca->nccd_l, 31) |
	           PPC_SHIFT(mca->cl - mem_data.cwl + var_dly, 36) |
	           PPC_SHIFT(mca->cl - mem_data.cwl + var_dly, 41) |
	           PPC_SHIFT(mca->cl - mem_data.cwl + var_dly, 46) |
	           PPC_SHIFT(mem_data.cwl - mca->cl + var_dly, 50) |
	           PPC_SHIFT(mem_data.cwl + mca->nwtr_s + 4, 56) |
	           PPC_SHIFT(mem_data.cwl + mca->nwtr_s + 4, 62));

	/* MC01.PORT0.SRQ.MBA_TMR1Q =
	    [0-3]   MBA_TMR1Q_RRSBG_DLY =   ATTR_EFF_DRAM_TCCD_L
	    [4-9]   MBA_TMR1Q_WRSBG_DLY =   ATTR_EFF_DRAM_CWL + ATTR_EFF_DRAM_TWTR_L + 4
	    [10-15] MBA_TMR1Q_CFG_TFAW =    ATTR_EFF_DRAM_TFAW
	    [16-20] MBA_TMR1Q_CFG_TRCD =    ATTR_EFF_DRAM_TRCD
	    [21-25] MBA_TMR1Q_CFG_TRP =     ATTR_EFF_DRAM_TRP
	    [26-31] MBA_TMR1Q_CFG_TRAS =    ATTR_EFF_DRAM_TRAS
	    [41-47] MBA_TMR1Q_CFG_WR2PRE =  ATTR_EFF_DRAM_CWL + ATTR_EFF_DRAM_TWR + 4
	    [48-51] MBA_TMR1Q_CFG_RD2PRE =  ATTR_EFF_DRAM_TRTP
	    [52-55] MBA_TMR1Q_TRRD =        ATTR_EFF_DRAM_TRRD_S
	    [56-59] MBA_TMR1Q_TRRD_SBG =    ATTR_EFF_DRAM_TRRD_L
	    [60-63] MBA_TMR1Q_CFG_ACT_TO_DIFF_RANK_DLY =    // var_dly from above
		MSS_FREQ_EQ_1866:           8
		MSS_FREQ_EQ_2133:           9
		MSS_FREQ_EQ_2400:           10
		MSS_FREQ_EQ_2666:           11
	*/
	mca_and_or(id, mca_i, 0x0701090C, 0,
	           PPC_SHIFT(mca->nccd_l, 3) |
	           PPC_SHIFT(mem_data.cwl + mca->nwtr_l + 4, 9) |
	           PPC_SHIFT(mca->nfaw, 15) |
	           PPC_SHIFT(mca->nrcd, 20) |
	           PPC_SHIFT(mca->nrp, 25) |
	           PPC_SHIFT(mca->nras, 31) |
	           PPC_SHIFT(mem_data.cwl + mca->nwr + 4, 47) |
	           PPC_SHIFT(mem_data.nrtp, 51) |
	           PPC_SHIFT(mca->nrrd_s, 55) |
	           PPC_SHIFT(mca->nrrd_l, 59) |
	           PPC_SHIFT(var_dly, 63));

	/* MC01.PORT0.SRQ.MBA_WRQ0Q =
	    [5]     MBA_WRQ0Q_CFG_WRQ_FIFO_MODE =               0   // ATTR_MSS_REORDER_QUEUE_SETTING, 0 = reorder
	    [6]     MBA_WRQ0Q_CFG_DISABLE_WR_PG_MODE =          1
	    [55-58] MBA_WRQ0Q_CFG_WRQ_ACT_NUM_WRITES_PENDING =  8
	*/
	mca_and_or(id, mca_i, 0x0701090D, ~(PPC_BIT(5) | PPC_BIT(6) | PPC_BITMASK(55, 58)),
	           PPC_BIT(6) | PPC_SHIFT(8, 58));

	/* MC01.PORT0.SRQ.MBA_RRQ0Q =
	    [6]     MBA_RRQ0Q_CFG_RRQ_FIFO_MODE =               0   // ATTR_MSS_REORDER_QUEUE_SETTING
	    [57-60] MBA_RRQ0Q_CFG_RRQ_ACT_NUM_READS_PENDING =   8
	*/
	mca_and_or(id, mca_i, 0x0701090E, ~(PPC_BIT(6) | PPC_BITMASK(57, 60)),
	           PPC_SHIFT(8, 60));

	/* MC01.PORT0.SRQ.MBA_FARB0Q =
	    if (l_TGT3_ATTR_MSS_MRW_DRAM_2N_MODE == 0x02 || (l_TGT3_ATTR_MSS_MRW_DRAM_2N_MODE == 0x00 && l_TGT2_ATTR_MSS_VPD_MR_MC_2N_MODE_AUTOSET == 0x02))
	      [17] MBA_FARB0Q_CFG_2N_ADDR =         1     // Default is auto for mode, 1N from VPD, so [17] = 0
	    [38] MBA_FARB0Q_CFG_PARITY_AFTER_CMD =  1
	    [61-63] MBA_FARB0Q_CFG_OPT_RD_SIZE =    3
	*/
	mca_and_or(id, mca_i, 0x07010913, ~(PPC_BIT(17) | PPC_BIT(38) | PPC_BITMASK(61, 63)),
	           PPC_BIT(38) | PPC_SHIFT(3, 63));

	/* MC01.PORT0.SRQ.MBA_FARB1Q =
	    [0-2]   MBA_FARB1Q_CFG_SLOT0_S0_CID = 0
	    [3-5]   MBA_FARB1Q_CFG_SLOT0_S1_CID = 4
	    [6-8]   MBA_FARB1Q_CFG_SLOT0_S2_CID = 2
	    [9-11]  MBA_FARB1Q_CFG_SLOT0_S3_CID = 6
	    if (DIMM0 is 8H 3DS)
	      [12-14] MBA_FARB1Q_CFG_SLOT0_S4_CID =   1
	      [15-17] MBA_FARB1Q_CFG_SLOT0_S5_CID =   5
	      [18-20] MBA_FARB1Q_CFG_SLOT0_S6_CID =   3
	      [21-23] MBA_FARB1Q_CFG_SLOT0_S7_CID =   7
	    else
	      [12-14] MBA_FARB1Q_CFG_SLOT0_S4_CID =   0
	      [15-17] MBA_FARB1Q_CFG_SLOT0_S5_CID =   4
	      [18-20] MBA_FARB1Q_CFG_SLOT0_S6_CID =   2
	      [21-23] MBA_FARB1Q_CFG_SLOT0_S7_CID =   6
	    if (DIMM0 has 4 master ranks)
	      [12-14] MBA_FARB1Q_CFG_SLOT0_S4_CID =   4     // TODO: test if all slots with 4R DIMMs works with that
	    [24-26] MBA_FARB1Q_CFG_SLOT1_S0_CID = 0
	    [27-29] MBA_FARB1Q_CFG_SLOT1_S1_CID = 4
	    [30-32] MBA_FARB1Q_CFG_SLOT1_S2_CID = 2
	    [33-35] MBA_FARB1Q_CFG_SLOT1_S3_CID = 6
	    if (DIMM1 is 8H 3DS)
	      [36-38] MBA_FARB1Q_CFG_SLOT1_S4_CID =   1
	      [39-41] MBA_FARB1Q_CFG_SLOT1_S5_CID =   5
	      [42-44] MBA_FARB1Q_CFG_SLOT1_S6_CID =   3
	      [45-47] MBA_FARB1Q_CFG_SLOT1_S7_CID =   7
	    else
	      [36-38] MBA_FARB1Q_CFG_SLOT1_S4_CID =   0
	      [39-41] MBA_FARB1Q_CFG_SLOT1_S5_CID =   4
	      [42-44] MBA_FARB1Q_CFG_SLOT1_S6_CID =   2
	      [45-47] MBA_FARB1Q_CFG_SLOT1_S7_CID =   6
	    if (DIMM1 has 4 master ranks)
	      [36-38] MBA_FARB1Q_CFG_SLOT1_S4_CID =   4     // TODO: test if all slots with 4R DIMMs works with that
	*/
	/* Due to allowable DIMM mixing rules, ranks of both DIMMs are the same */
	uint64_t cids_even = (0 << 9) | (4 << 6) | (2 << 3) | (6 << 0);
	uint64_t cids_odd =  (1 << 9) | (5 << 6) | (3 << 3) | (7 << 0);
	uint64_t cids_4_7 = is_8H ? cids_odd : cids_even;
	/* Not sure if this is even supported, there is no MT VPD data for this case */
	if (mranks == 4)
		cids_4_7 = (cids_4_7 & ~(7ull << 9)) | (4 << 9);

	mca_and_or(id, mca_i, 0x07010914, ~PPC_BITMASK(0, 47),
	           PPC_SHIFT(cids_even, 11) | PPC_SHIFT(cids_4_7, 23) |
	           PPC_SHIFT(cids_even, 35) | PPC_SHIFT(cids_4_7, 47));

	/*
	 * ATTR_MSS_VPD_MT_ODT_RD has form uint8_t x[mca][dimm][rank], but there
	 * are different ATTR_MSS_VPD_MT_ODT_RD tables for different occasions:
	 * - 1 DIMM populated, 1 mrank
	 * - 2 DIMMs populated, 1 mrank
	 * - 1 DIMM populated, 2 mranks
	 * - 2 DIMMs populated, 2 mranks
	 *
	 * There is no configuration for 4 mranks. The same goes for *_WR tables.
	 * This heavily depends on correct DIMM mixing and population rules.
	 *
	 * Not sure what would be the best way of implementing it. We can either
	 * rely on VPD data (which may change its layout at some point), copy it
	 * all here in more ready-to-use format (no dependency on layout, but may
	 * miss updates from OpenPOWER) or find out if there is a way to calculate
	 * these values from a different set of DIMM configurations. Some of the
	 * values are the same for all configurations, keeping them is a waste of
	 * precious space.
	 *
	 * FIXME: manual copy for now, using currently installed option. Change it!
	 */
	static const uint8_t ATTR_MSS_VPD_MT_ODT_RD[2][2][4] = {0};
	static const uint8_t ATTR_MSS_VPD_MT_ODT_WR[2][2][4] = {
		{ {0x80, 0, 0, 0}, {0} },
		{ {0x80, 0, 0, 0}, {0} }
	};

	/* MC01.PORT0.SRQ.MBA_FARB2Q =
	    F(X) = (((X >> 4) & 0xc) | ((X >> 2) & 0x3))    // Bits 0,1,4,5 of uint8_t X, big endian numbering
	    [0-3]   MBA_FARB2Q_CFG_RANK0_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][0][0])
	    [4-7]   MBA_FARB2Q_CFG_RANK1_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][0][1])
	    [8-11]  MBA_FARB2Q_CFG_RANK2_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][0][2])
	    [12-15] MBA_FARB2Q_CFG_RANK3_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][0][3])
	    [16-19] MBA_FARB2Q_CFG_RANK4_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][1][0])
	    [20-23] MBA_FARB2Q_CFG_RANK5_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][1][1])
	    [24-27] MBA_FARB2Q_CFG_RANK6_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][1][2])
	    [28-31] MBA_FARB2Q_CFG_RANK7_RD_ODT = F(ATTR_MSS_VPD_MT_ODT_RD[l_def_PORT_INDEX][1][3])
	    [32-35] MBA_FARB2Q_CFG_RANK0_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][0][0])
	    [36-39] MBA_FARB2Q_CFG_RANK1_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][0][1])
	    [40-43] MBA_FARB2Q_CFG_RANK2_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][0][2])
	    [44-47] MBA_FARB2Q_CFG_RANK3_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][0][3])
	    [48-51] MBA_FARB2Q_CFG_RANK4_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][1][0])
	    [52-55] MBA_FARB2Q_CFG_RANK5_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][1][1])
	    [56-59] MBA_FARB2Q_CFG_RANK6_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][1][2])
	    [60-63] MBA_FARB2Q_CFG_RANK7_WR_ODT = F(ATTR_MSS_VPD_MT_ODT_WR[l_def_PORT_INDEX][1][3])
	*/
	#define F(X)	((((X) >> 4) & 0xc) | (((X) >> 2) & 0x3))
	mca_and_or(id, mca_i, 0x07010915, 0,
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[mca_i][0][0]), 3) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[mca_i][0][1]), 7) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[mca_i][0][2]), 11) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[mca_i][0][3]), 15) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[mca_i][1][0]), 19) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[mca_i][1][1]), 23) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[mca_i][1][2]), 27) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[mca_i][1][3]), 31) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[mca_i][0][0]), 35) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[mca_i][0][1]), 39) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[mca_i][0][2]), 43) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[mca_i][0][3]), 47) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[mca_i][1][0]), 51) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[mca_i][1][1]), 55) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[mca_i][1][2]), 59) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[mca_i][1][3]), 63));
	#undef F

	/* MC01.PORT0.SRQ.PC.MBAREF0Q =
	    [5-7]   MBAREF0Q_CFG_REFRESH_PRIORITY_THRESHOLD = 3
	    [8-18]  MBAREF0Q_CFG_REFRESH_INTERVAL =           ATTR_EFF_DRAM_TREFI / (8 * (DIMM0 + DIMM1 logical ranks))
	    [30-39] MBAREF0Q_CFG_TRFC =                       ATTR_EFF_DRAM_TRFC
	    [40-49] MBAREF0Q_CFG_REFR_TSV_STACK =             ATTR_EFF_DRAM_TRFC_DLR
	    [50-60] MBAREF0Q_CFG_REFR_CHECK_INTERVAL =        ((ATTR_EFF_DRAM_TREFI / 8) * 6) / 5
	*/
	mca_and_or(id, mca_i, 0x07010932, ~(PPC_BITMASK(5, 18) | PPC_BITMASK(30, 60)),
	           PPC_SHIFT(3, 7) | PPC_SHIFT(mem_data.nrefi / (8 * 2 * log_ranks), 18) |
	           PPC_SHIFT(mca->nrfc, 39) | PPC_SHIFT(mca->nrfc_dlr, 49) |
	           PPC_SHIFT(((mem_data.nrefi / 8) * 6) / 5, 60));

	/* MC01.PORT0.SRQ.PC.MBARPC0Q =
	    [6-10]  MBARPC0Q_CFG_PUP_AVAIL =
	      MSS_FREQ_EQ_1866: 6
	      MSS_FREQ_EQ_2133: 7
	      MSS_FREQ_EQ_2400: 8
	      MSS_FREQ_EQ_2666: 9
	    [11-15] MBARPC0Q_CFG_PDN_PUP =
	      MSS_FREQ_EQ_1866: 5
	      MSS_FREQ_EQ_2133: 6
	      MSS_FREQ_EQ_2400: 6
	      MSS_FREQ_EQ_2666: 7
	    [16-20] MBARPC0Q_CFG_PUP_PDN =
	      MSS_FREQ_EQ_1866: 5
	      MSS_FREQ_EQ_2133: 6
	      MSS_FREQ_EQ_2400: 6
	      MSS_FREQ_EQ_2666: 7
	    [21] MBARPC0Q_RESERVED_21 =         // MCP_PORT0_SRQ_PC_MBARPC0Q_CFG_QUAD_RANK_ENC
	      (l_def_MASTER_RANKS_DIMM0 == 4): 1
	      (l_def_MASTER_RANKS_DIMM0 != 4): 0
	*/
	/* Perhaps these can be done by ns_to_nck(), but Hostboot used a forest of ifs */
	uint64_t pup_avail = mem_data.speed == 1866 ? 6 :
	                     mem_data.speed == 2133 ? 7 :
	                     mem_data.speed == 2400 ? 8 : 9;
	uint64_t p_up_dn =   mem_data.speed == 1866 ? 5 :
	                     mem_data.speed == 2666 ? 7 : 6;
	mca_and_or(id, mca_i, 0x07010934, ~PPC_BITMASK(6, 21),
	           PPC_SHIFT(pup_avail, 10) | PPC_SHIFT(p_up_dn, 15) |
	           PPC_SHIFT(p_up_dn, 20) | (mranks == 4 ? PPC_BIT(21) : 0));

	/* MC01.PORT0.SRQ.PC.MBASTR0Q =
	    [12-16] MBASTR0Q_CFG_TCKESR = 5
	    [17-21] MBASTR0Q_CFG_TCKSRE =
	      MSS_FREQ_EQ_1866: 10
	      MSS_FREQ_EQ_2133: 11
	      MSS_FREQ_EQ_2400: 12
	      MSS_FREQ_EQ_2666: 14
	    [22-26] MBASTR0Q_CFG_TCKSRX =
	      MSS_FREQ_EQ_1866: 10
	      MSS_FREQ_EQ_2133: 11
	      MSS_FREQ_EQ_2400: 12
	      MSS_FREQ_EQ_2666: 14
	    [27-37] MBASTR0Q_CFG_TXSDLL =
	      MSS_FREQ_EQ_1866: 597
	      MSS_FREQ_EQ_2133: 768
	      MSS_FREQ_EQ_2400: 768
	      MSS_FREQ_EQ_2666: 939
	    [46-56] MBASTR0Q_CFG_SAFE_REFRESH_INTERVAL = ATTR_EFF_DRAM_TREFI / (8 * (DIMM0 + DIMM1 logical ranks))
	*/
	uint64_t tcksr_ex = mem_data.speed == 1866 ? 10 :
	                    mem_data.speed == 2133 ? 11 :
	                    mem_data.speed == 2400 ? 12 : 14;
	uint64_t txsdll = mem_data.speed == 1866 ? 597 :
	                  mem_data.speed == 2666 ? 939 : 768;
	mca_and_or(id, mca_i, 0x07010935, ~(PPC_BITMASK(12, 37) | PPC_BITMASK(46, 56)),
	           PPC_SHIFT(5, 16) | PPC_SHIFT(tcksr_ex, 21) |
	           PPC_SHIFT(tcksr_ex, 26) | PPC_SHIFT(txsdll, 37) |
	           PPC_SHIFT(mem_data.nrefi / (8 * 2 * log_ranks), 56));

	/* MC01.PORT0.ECC64.SCOM.RECR =
	    [16-18] MBSECCQ_VAL_TO_DATA_DELAY =
	      l_TGT4_ATTR_MC_SYNC_MODE == 1:  5
	      l_def_mn_freq_ratio < 915:      3
	      l_def_mn_freq_ratio < 1150:     4
	      l_def_mn_freq_ratio < 1300:     5
	      l_def_mn_freq_ratio >= 1300:    6
	    [19]    MBSECCQ_DELAY_VALID_1X =  0
	    [20-21] MBSECCQ_NEST_VAL_TO_DATA_DELAY =
	      l_TGT4_ATTR_MC_SYNC_MODE == 1:  1
	      l_def_mn_freq_ratio < 1040:     1
	      l_def_mn_freq_ratio < 1150:     0
	      l_def_mn_freq_ratio < 1215:     1
	      l_def_mn_freq_ratio < 1300:     0
	      l_def_mn_freq_ratio < 1400:     1
	      l_def_mn_freq_ratio >= 1400:    0
	    [22]    MBSECCQ_DELAY_NONBYPASS =
	      l_TGT4_ATTR_MC_SYNC_MODE == 1:  0
	      l_def_mn_freq_ratio < 1215:     0
	      l_def_mn_freq_ratio >= 1215:    1
	    [40]    MBSECCQ_RESERVED_36_43 =        // MCP_PORT0_ECC64_ECC_SCOM_MBSECCQ_BYPASS_TENURE_3
	      l_TGT4_ATTR_MC_SYNC_MODE == 1:  0
	      l_TGT4_ATTR_MC_SYNC_MODE == 0:  1
	*/
	/* Assume asynchronous mode */
	/*
	 * From Hostboot:
	 *     l_def_mn_freq_ratio = 1000 * ATTR_MSS_FREQ / ATTR_FREQ_PB_MHZ;
	 * ATTR_MSS_FREQ is in MT/s (sigh), ATTR_FREQ_PB_MHZ is 1866 MHz (from talos.xml).
	 */
	uint64_t mn_freq_ratio = 1000 * mem_data.speed / FREQ_PB_MHZ;
	uint64_t val_to_data = mn_freq_ratio < 915 ? 3 :
	                       mn_freq_ratio < 1150 ? 4 :
	                       mn_freq_ratio < 1300 ? 5 : 6;
	uint64_t nest_val_to_data = mn_freq_ratio < 1040 ? 1 :
	                            mn_freq_ratio < 1150 ? 0 :
	                            mn_freq_ratio < 1215 ? 1 :
	                            mn_freq_ratio < 1300 ? 0 :
	                            mn_freq_ratio < 1400 ? 1 : 0;
	mca_and_or(id, mca_i, 0x07010A0A, ~(PPC_BITMASK(16, 22) | PPC_BIT(22)),
	           PPC_SHIFT(val_to_data, 18) | PPC_SHIFT(nest_val_to_data, 21) |
	           (mn_freq_ratio < 1215 ? 0 : PPC_BIT(22)) | PPC_BIT(40));

	/* MC01.PORT0.ECC64.SCOM.DBGR =
	    [9]     DBGR_ECC_WAT_ACTION_SELECT =  0
	    [10-11] DBGR_ECC_WAT_SOURCE =         0
	*/
	mca_and_or(id, mca_i, 0x07010A0B, ~PPC_BITMASK(9, 11), 0);

	/* MC01.PORT0.WRITE.WRTCFG =
	    [9] = 1     // MCP_PORT0_WRITE_NEW_WRITE_64B_MODE   this is marked as RO const 0 for bits 8-63 in docs!
	*/
	mca_and_or(id, mca_i, 0x07010A38, ~0ull, PPC_BIT(9));
}

static void thermal_throttle_scominit(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	/* Set power control register */
	/* MC01.PORT0.SRQ.PC.MBARPC0Q =
	    [3-5]   MBARPC0Q_CFG_MIN_MAX_DOMAINS =                          0
	    [22]    MBARPC0Q_CFG_MIN_DOMAIN_REDUCTION_ENABLE =
	      if ATTR_MSS_MRW_POWER_CONTROL_REQUESTED == PD_AND_STR_OFF:    0     // default
	      else:                                                         1
	    [23-32] MBARPC0Q_CFG_MIN_DOMAIN_REDUCTION_TIME =                959
	*/
	mca_and_or(id, mca_i, 0x07010934, ~(PPC_BITMASK(3, 5) | PPC_BITMASK(22, 32)),
	           PPC_SHIFT(959, 32));

	/* Set STR register */
	/* MC01.PORT0.SRQ.PC.MBASTR0Q =
	    [0]     MBASTR0Q_CFG_STR_ENABLE =
	      ATTR_MSS_MRW_POWER_CONTROL_REQUESTED == PD_AND_STR:           1
	      ATTR_MSS_MRW_POWER_CONTROL_REQUESTED == PD_AND_STR_CLK_STOP:  1
	      ATTR_MSS_MRW_POWER_CONTROL_REQUESTED == POWER_DOWN:           0
	      ATTR_MSS_MRW_POWER_CONTROL_REQUESTED == PD_AND_STR_OFF:       0     // default
	    [2-11]  MBASTR0Q_CFG_ENTER_STR_TIME =                           1023
	*/
	mca_and_or(id, mca_i, 0x07010935, ~(PPC_BIT(0) | PPC_BITMASK(2, 11)),
	           PPC_SHIFT(1023, 11));

	/* Set N/M throttling control register */
	/* TODO: these attributes are calculated in 7.4, implement this */
	/* MC01.PORT0.SRQ.MBA_FARB3Q =
	    [0-14]  MBA_FARB3Q_CFG_NM_N_PER_SLOT = ATTR_MSS_RUNTIME_MEM_THROTTLED_N_COMMANDS_PER_SLOT[mss::index(MCA)]
	    [15-30] MBA_FARB3Q_CFG_NM_N_PER_PORT = ATTR_MSS_RUNTIME_MEM_THROTTLED_N_COMMANDS_PER_PORT[mss::index(MCA)]
	    [31-44] MBA_FARB3Q_CFG_NM_M =          ATTR_MSS_MRW_MEM_M_DRAM_CLOCKS     // default 0x200
	    [45-47] MBA_FARB3Q_CFG_NM_RAS_WEIGHT = 0
	    [48-50] MBA_FARB3Q_CFG_NM_CAS_WEIGHT = 1
	    // Set to disable permanently due to hardware design bug (HW403028) that won't be changed
	    [53]    MBA_FARB3Q_CFG_NM_CHANGE_AFTER_SYNC = 0
	*/
	printk(BIOS_EMERG, "Please FIXME: ATTR_MSS_RUNTIME_MEM_THROTTLED_N_COMMANDS_PER_SLOT\n");
	/* Values dumped after Hostboot's calculations, may be different for other DIMMs */
	uint64_t nm_n_per_slot = 0x80;
	uint64_t nm_n_per_port = 0x80;
	mca_and_or(id, mca_i, 0x07010916, ~(PPC_BITMASK(0, 50) | PPC_BIT(53)),
	           PPC_SHIFT(nm_n_per_slot, 14) | PPC_SHIFT(nm_n_per_port, 30) |
	           PPC_SHIFT(0x200, 44) | PPC_SHIFT(1, 50));

	/* Set safemode throttles */
	/* MC01.PORT0.SRQ.MBA_FARB4Q =
	    [27-41] MBA_FARB4Q_EMERGENCY_N = ATTR_MSS_RUNTIME_MEM_THROTTLED_N_COMMANDS_PER_PORT[mss::index(MCA)]  // BUG? var name says per_slot...
	    [42-55] MBA_FARB4Q_EMERGENCY_M = ATTR_MSS_MRW_MEM_M_DRAM_CLOCKS
	*/
	mca_and_or(id, mca_i, 0x07010917, ~PPC_BITMASK(27, 55),
	           PPC_SHIFT(nm_n_per_port, 41) |
	           PPC_SHIFT(0x200, 55));
}

/*
 * Values set in this function are mostly for magic MCA, other (functional) MCAs
 * are set later. If all of these registers are later written with proper values
 * for functional MCAs, maybe this can be called just for magic, non-functional
 * ones to save time, but for now do it in a way the Hostboot does it.
 */
static void p9n_ddrphy_scom(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;
	/*
	 * Hostboot sets this to proper value in phy_scominit(), but I don't see
	 * why. Speed is the same for whole MCBIST anyway.
	 */
	uint64_t strength = mem_data.speed == 1866 ? 1 :
			    mem_data.speed == 2133 ? 2 :
			    mem_data.speed == 2400 ? 4 : 8;

	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_DLL_VREG_CONTROL0_P0_{0,1,2,3,4} =
		  [48-50] RXREG_VREG_COMPCON_DC = 3
		  [52-59] = 0x74:
			  [53-55] RXREG_VREG_DRVCON_DC =  0x7
			  [56-58] RXREG_VREG_REF_SEL_DC = 0x2
		  [62-63] = 0:
			  [62] DLL_DRVREN_MODE =      POWER8 mode (thermometer style, enabling all drivers up to the one that is used)
			  [63] DLL_CAL_CKTS_ACTIVE =  After VREG calibration, some analog circuits are powered down
		*/
		dp_mca_and_or(id, dp, mca_i, 0x8000002A0701103F,
		              ~(PPC_BITMASK(48, 50) | PPC_BITMASK(52, 59) | PPC_BITMASK(62, 63)),
		              PPC_SHIFT(3, 50) | PPC_SHIFT(0x74, 59));

		/* IOM0.DDRPHY_DP16_DLL_VREG_CONTROL1_P0_{0,1,2,3,4} =
		  [48-50] RXREG_VREG_COMPCON_DC = 3
		  [52-59] = 0x74:
			  [53-55] RXREG_VREG_DRVCON_DC =  0x7
			  [56-58] RXREG_VREG_REF_SEL_DC = 0x2
		  [62-63] = 0:
			  [62] DLL_DRVREN_MODE =      POWER8 mode (thermometer style, enabling all drivers up to the one that is used)
			  [63] DLL_CAL_CKTS_ACTIVE =  After VREG calibration, some analog circuits are powered down
		*/
		dp_mca_and_or(id, dp, mca_i, 0x8000002B0701103F,
		              ~(PPC_BITMASK(48, 50) | PPC_BITMASK(52, 59) | PPC_BITMASK(62, 63)),
		              PPC_SHIFT(3, 50) | PPC_SHIFT(0x74, 59));

		/* IOM0.DDRPHY_DP16_WRCLK_PR_P0_{0,1,2,3,4} =
		  // For zero delay simulations, or simulations where the delay of the SysClk tree and the WrClk tree are equal,
		  // set this field to 60h
		  [49-55] TSYS_WRCLK = 0x60
		*/
		dp_mca_and_or(id, dp, mca_i, 0x800000740701103F,
		              ~PPC_BITMASK(49, 55), PPC_SHIFT(0x60, 55));

		/* IOM0.DDRPHY_DP16_IO_TX_CONFIG0_P0_{0,1,2,3,4} =
		  [48-51] STRENGTH =                    0x4 // 2400 MT/s
		  [52]    DD2_RESET_READ_FIX_DISABLE =  0   // Enable the DD2 function to remove the register reset on read feature
							    // on status registers
		*/
		dp_mca_and_or(id, dp, mca_i, 0x800000750701103F,
		              ~PPC_BITMASK(48, 52), PPC_SHIFT(strength, 51));

		/* IOM0.DDRPHY_DP16_DLL_CONFIG1_P0_{0,1,2,3,4} =
		  [48-63] = 0x0006:
			  [48-51] HS_DLLMUX_SEL_0_0_3 = 0
			  [53-56] HS_DLLMUX_SEL_1_0_3 = 0
			  [61]    S0INSDLYTAP =         1 // For proper functional operation, this bit must be 0b
			  [62]    S1INSDLYTAP =         1 // For proper functional operation, this bit must be 0b
		*/
		dp_mca_and_or(id, dp, mca_i, 0x800000770701103F,
		              ~(PPC_BITMASK(48, 63)), PPC_BIT(61) | PPC_BIT(62));

		/* IOM0.DDRPHY_DP16_IO_TX_FET_SLICE_P0_{0,1,2,3,4} =
		  [48-63] = 0x7f7f:
			  [59-55] EN_SLICE_N_WR = 0x7f
			  [57-63] EN_SLICE_P_WR = 0x7f
		*/
		dp_mca_and_or(id, dp, mca_i, 0x800000780701103F,
		              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x7F7F, 63));
	}

	for (dp = 0; dp < 4; dp++) {
		/* IOM0.DDRPHY_ADR_BIT_ENABLE_P0_ADR{0,1,2,3} =
		  [48-63] = 0xffff
		*/
		dp_mca_and_or(id, dp, mca_i, 0x800040000701103F,
		              ~PPC_BITMASK(48, 63), PPC_SHIFT(0xFFFF, 63));
	}

	/* IOM0.DDRPHY_ADR_DIFFPAIR_ENABLE_P0_ADR1 =
	  [48-63] = 0x5000:
		  [49] DI_ADR2_ADR3: 1 = Lanes 2 and 3 are a differential clock pair
		  [51] DI_ADR6_ADR7: 1 = Lanes 6 and 7 are a differential clock pair
	*/
	dp_mca_and_or(id, dp, mca_i, 0x800044010701103F,
	              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x5000, 63));

	/* IOM0.DDRPHY_ADR_DELAY1_P0_ADR1 =
	  [48-63] = 0x4040:
		  [49-55] ADR_DELAY2 = 0x40
		  [57-63] ADR_DELAY3 = 0x40
	*/
	dp_mca_and_or(id, dp, mca_i, 0x800044050701103F,
	              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x4040, 63));

	/* IOM0.DDRPHY_ADR_DELAY3_P0_ADR1 =
	  [48-63] = 0x4040:
		  [49-55] ADR_DELAY6 = 0x40
		  [57-63] ADR_DELAY7 = 0x40
	*/
	dp_mca_and_or(id, dp, mca_i, 0x800044070701103F,
	              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x4040, 63));

	for (dp = 0; dp < 2; dp ++) {
		/* IOM0.DDRPHY_ADR_DLL_VREG_CONFIG_1_P0_ADR32S{0,1} =
		  [48-63] = 0x0008:
			  [48-51] HS_DLLMUX_SEL_0_3 = 0
			  [59-62] STRENGTH =          4 // 2400 MT/s
		*/
		dp_mca_and_or(id, dp, mca_i, 0x800080310701103F,
		              ~PPC_BITMASK(48, 63), PPC_SHIFT(strength, 62));

		/* IOM0.DDRPHY_ADR_MCCLK_WRCLK_PR_STATIC_OFFSET_P0_ADR32S{0,1} =
		  [48-63] = 0x6000
		      // For zero delay simulations, or simulations where the delay of the
		      // SysClk tree and the WrClk tree are equal, set this field to 60h
		      [49-55] TSYS_WRCLK = 0x60
		*/
		dp_mca_and_or(id, dp, mca_i, 0x800080330701103F,
		              ~PPC_BITMASK(48, 63), PPC_SHIFT(0x6000, 63));

		/* IOM0.DDRPHY_ADR_DLL_VREG_CONTROL_P0_ADR32S{0,1} =
		  [48-50] RXREG_VREG_COMPCON_DC =         3
		  [52-59] = 0x74:
			  [53-55] RXREG_VREG_DRVCON_DC =  0x7
			  [56-58] RXREG_VREG_REF_SEL_DC = 0x2
		  [63] DLL_CAL_CKTS_ACTIVE =  0   // After VREG calibration, some analog circuits are powered down
		*/
		dp_mca_and_or(id, dp, mca_i, 0x8000803d0701103F,
		              ~PPC_BITMASK(48, 63), PPC_SHIFT(3, 50) | PPC_SHIFT(0x74, 59));
	}

	/* IOM0.DDRPHY_PC_CONFIG0_P0 =             // 0x8000c00c0701103f
	  [48-63] = 0x0202:
		  [48-51] PDA_ENABLE_OVERRIDE =     0
		  [52]    2TCK_PREAMBLE_ENABLE =    0
		  [53]    PBA_ENABLE =              0
		  [54]    DDR4_CMD_SIG_REDUCTION =  1
		  [55]    SYSCLK_2X_MEMINTCLKO =    0
		  [56]    RANK_OVERRIDE =           0
		  [57-59] RANK_OVERRIDE_VALUE =     0
		  [60]    LOW_LATENCY =             0
		  [61]    DDR4_IPW_LOOP_DIS =       0
		  [62]    DDR4_VLEVEL_BANK_GROUP =  1
		  [63]    VPROTH_PSEL_MODE =        0
	*/
	mca_and_or(id, mca_i, 0x8000C00C0701103F, ~PPC_BITMASK(48, 63),
	           PPC_SHIFT(0x0202, 63));
}

static void p9n_mcbist_scom(int mcs_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	/* MC01.MCBIST.MBA_SCOMFIR.WATCFG0AQ =
	    [0-47]  WATCFG0AQ_CFG_WAT_EVENT_SEL =  0x400000000000
	*/
	scom_and_or_for_chiplet(id, 0x07012380, ~PPC_BITMASK(0, 47),
	                        PPC_SHIFT(0x400000000000, 47));

	/* MC01.MCBIST.MBA_SCOMFIR.WATCFG0BQ =
	    [0-43]  WATCFG0BQ_CFG_WAT_MSKA =  0x3fbfff
	    [44-60] WATCFG0BQ_CFG_WAT_CNTL =  0x10000
	*/
	scom_and_or_for_chiplet(id, 0x07012381, ~PPC_BITMASK(0, 60),
	                        PPC_SHIFT(0x3fbfff, 43) | PPC_SHIFT(0x10000, 60));

	/* MC01.MCBIST.MBA_SCOMFIR.WATCFG0DQ =
	    [0-43]  WATCFG0DQ_CFG_WAT_PATA =  0x80200004000
	*/
	scom_and_or_for_chiplet(id, 0x07012383, ~PPC_BITMASK(0, 43),
	                        PPC_SHIFT(0x80200004000, 43));

	/* MC01.MCBIST.MBA_SCOMFIR.WATCFG3AQ =
	    [0-47]  WATCFG3AQ_CFG_WAT_EVENT_SEL = 0x800000000000
	*/
	scom_and_or_for_chiplet(id, 0x0701238F, ~PPC_BITMASK(0, 47),
	                        PPC_SHIFT(0x800000000000, 47));

	/* MC01.MCBIST.MBA_SCOMFIR.WATCFG3BQ =
	    [0-43]  WATCFG3BQ_CFG_WAT_MSKA =  0xfffffffffff
	    [44-60] WATCFG3BQ_CFG_WAT_CNTL =  0x10400
	*/
	scom_and_or_for_chiplet(id, 0x07012390, ~PPC_BITMASK(0, 60),
	                        PPC_SHIFT(0xfffffffffff, 43) | PPC_SHIFT(0x10400, 60));

	/* MC01.MCBIST.MBA_SCOMFIR.MCBCFGQ =
	    [36]    MCBCFGQ_CFG_LOG_COUNTS_IN_TRACE = 0
	*/
	scom_and_or_for_chiplet(id, 0x070123E0, ~PPC_BIT(36), 0);

	/* MC01.MCBIST.MBA_SCOMFIR.DBGCFG0Q =
	    [0]     DBGCFG0Q_CFG_DBG_ENABLE =         1
	    [23-33] DBGCFG0Q_CFG_DBG_PICK_MCBIST01 =  0x780
	*/
	scom_and_or_for_chiplet(id, 0x070123E8, ~PPC_BITMASK(23, 33),
	                        PPC_BIT(0) | PPC_SHIFT(0x780, 33));

	/* MC01.MCBIST.MBA_SCOMFIR.DBGCFG1Q =
	    [0]     DBGCFG1Q_CFG_WAT_ENABLE = 1
	*/
	scom_and_or_for_chiplet(id, 0x070123E9, ~0ull, PPC_BIT(0));

	/* MC01.MCBIST.MBA_SCOMFIR.DBGCFG2Q =
	    [0-19]  DBGCFG2Q_CFG_WAT_LOC_EVENT0_SEL = 0x10000
	    [20-39] DBGCFG2Q_CFG_WAT_LOC_EVENT1_SEL = 0x08000
	*/
	scom_and_or_for_chiplet(id, 0x070123EA, ~PPC_BITMASK(0, 39),
	                        PPC_SHIFT(0x10000, 19) | PPC_SHIFT(0x08000, 39));

	/* MC01.MCBIST.MBA_SCOMFIR.DBGCFG3Q =
	    [20-22] DBGCFG3Q_CFG_WAT_GLOB_EVENT0_SEL =      0x4
	    [23-25] DBGCFG3Q_CFG_WAT_GLOB_EVENT1_SEL =      0x4
	    [37-40] DBGCFG3Q_CFG_WAT_ACT_SET_SPATTN_PULSE = 0x4
	*/
	scom_and_or_for_chiplet(id, 0x070123EB, ~(PPC_BITMASK(20, 25) | PPC_BITMASK(37, 40)),
	                        PPC_SHIFT(0x4, 22) | PPC_SHIFT(0x4, 25) | PPC_SHIFT(0x4, 40));
}

static void set_rank_pairs(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	/*
	 * Assumptions:
	 * - non-LR DIMMs (platform wiki),
	 * - no ATTR_EFF_RANK_GROUP_OVERRIDE,
	 * - mixing rules followed - the same rank configuration for both DIMMs.
	 *
	 * Because rank pairs are defined for each MCA, we can only have up to two
	 * 2R DIMMs. For such configurations, RP0 primary is rank 0 on DIMM 0,
	 * RP1 primary - rank 1 DIMM 0, RP2 primary - rank 0 DIMM 1,
	 * RP3 primary - rank 1 DIMM 1. There are no secondary (this is true for
	 * RDIMM only), tertiary or quaternary rank pairs.
	 */

	static const uint16_t F[] = {0, 0xf000, 0xf0f0, 0xfff0, 0xffff};

	/* TODO: can we mix mirrored and non-mirrored 2R DIMMs in one port? */

	/* IOM0.DDRPHY_PC_RANK_PAIR0_P0 =
	    // rank_countX is the number of master ranks on DIMM X.
	    [48-63] = 0x1537 & F[rank_count0]:      // F = {0, 0xf000, 0xf0f0, 0xfff0, 0xffff}
		[48-50] RANK_PAIR0_PRI = 0
		[51]    RANK_PAIR0_PRI_V = 1: if (rank_count0 >= 1)
		[52-54] RANK_PAIR0_SEC = 2
		[55]    RANK_PAIR0_SEC_V = 1: if (rank_count0 >= 3)
		[56-58] RANK_PAIR1_PRI = 1
		[59]    RANK_PAIR1_PRI_V = 1: if (rank_count0 >= 2)
		[60-62] RANK_PAIR1_SEC = 3
		[63]    RANK_PAIR1_SEC_V = 1: if (rank_count0 == 4)
	*/
	mca_and_or(id, mca_i, 0x8000C0020701103F, ~PPC_BITMASK(48, 63),
	           PPC_SHIFT(0x1537 & F[mca->dimm[0].mranks], 63));

	/* IOM0.DDRPHY_PC_RANK_PAIR1_P0 =
	    [48-63] = 0x1537 & F[rank_count1]:     // F = {0, 0xf000, 0xf0f0, 0xfff0, 0xffff}
		[48-50] RANK_PAIR2_PRI = 0
		[51]    RANK_PAIR2_PRI_V = 1: if (rank_count1 >= 1)
		[52-54] RANK_PAIR2_SEC = 2
		[55]    RANK_PAIR2_SEC_V = 1: if (rank_count1 >= 3)
		[56-58] RANK_PAIR3_PRI = 1
		[59]    RANK_PAIR3_PRI_V = 1: if (rank_count1 >= 2)
		[60-62] RANK_PAIR3_SEC = 3
		[63]    RANK_PAIR3_SEC_V = 1: if (rank_count1 == 4)
	*/
	mca_and_or(id, mca_i, 0x8000C0030701103F, ~PPC_BITMASK(48, 63),
	           PPC_SHIFT(0x1537 & F[mca->dimm[1].mranks], 63));

	/* IOM0.DDRPHY_PC_RANK_PAIR2_P0 =
	    [48-63] = 0
	*/
	mca_and_or(id, mca_i, 0x8000C0300701103F, ~PPC_BITMASK(48, 63), 0);

	/* IOM0.DDRPHY_PC_RANK_PAIR3_P0 =
	    [48-63] = 0
	*/
	mca_and_or(id, mca_i, 0x8000C0310701103F, ~PPC_BITMASK(48, 63), 0);

	/* IOM0.DDRPHY_PC_CSID_CFG_P0 =
	    [0-63]  0xf000:
		[48]  CS0_INIT_CAL_VALUE = 1
		[49]  CS1_INIT_CAL_VALUE = 1
		[50]  CS2_INIT_CAL_VALUE = 1
		[51]  CS3_INIT_CAL_VALUE = 1
	*/
	mca_and_or(id, mca_i, 0x8000C0330701103F, ~PPC_BITMASK(48, 63),
	           PPC_SHIFT(0xF000, 63));

	/* IOM0.DDRPHY_PC_MIRROR_CONFIG_P0 =
	    [all] = 0
	    // A rank is mirrored if all are true:
	    //  - the rank is valid (RANK_PAIRn_XXX_V   ==  1)
	    //  - the rank is odd   (RANK_PAIRn_XXX % 2 ==  1)
	    //  - the mirror mode attribute is set for the rank's DIMM (SPD[136])
	    //  - We are not in quad encoded mode (so master ranks <= 2)
	    [48]    ADDR_MIRROR_RP0_PRI
		...
	    [55]    ADDR_MIRROR_RP3_SEC
	    [58]    ADDR_MIRROR_A3_A4 = 1
	    [59]    ADDR_MIRROR_A5_A6 = 1
	    [60]    ADDR_MIRROR_A7_A8 = 1
	    [61]    ADDR_MIRROR_A11_A13 = 1
	    [62]    ADDR_MIRROR_BA0_BA1 = 1
	    [63]    ADDR_MIRROR_BG0_BG1 = 1
	*/
	/*
	 * Assumptions:
	 * - primary and secondary have the same evenness,
	 * - RP1 and RP3 have odd ranks,
	 * - both DIMMs have SPD[136] set or both have it unset, no mixing allowed,
	 * - when rank is not valid, it doesn't matter if it is mirrored,
	 * - no quad encoded mode - no data for it in MT VPD anyway.
	 *
	 * With all of the above, ADDR_MIRROR_RP{1,3}_{PRI,SEC} = SPD[136].
	 */
	uint64_t mirr = mca->dimm[0].present ? mca->dimm[0].spd[136] :
	                                       mca->dimm[1].spd[136];
	mca_and_or(id, mca_i, 0x8000C0110701103F, ~PPC_BITMASK(48, 63),
	           PPC_SHIFT(mirr, 50) | PPC_SHIFT(mirr, 51) | /* RP1 */
	           PPC_SHIFT(mirr, 54) | PPC_SHIFT(mirr, 55) | /* RP3 */
	           PPC_BITMASK(58, 63));

	/* IOM0.DDRPHY_PC_RANK_GROUP_EXT_P0 =  // 0x8000C0350701103F
	    [all] = 0
	    // Same rules as above
	    [48]    ADDR_MIRROR_RP0_TER
		...
	    [55]    ADDR_MIRROR_RP3_QUA
	*/
	/* These are not valid anyway, so don't bother setting anything. */
}

static void reset_data_bit_enable(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	for (dp = 0; dp < 4; dp++) {
		/* IOM0.DDRPHY_DP16_DQ_BIT_ENABLE0_P0_{0,1,2,3} =
		    [all] = 0
		    [48-63] DATA_BIT_ENABLE_0_15 = 0xffff
		*/
		dp_mca_and_or(id, dp, mca_i, 0x800000000701103F, 0, 0xFFFF);
	}

	/* IOM0.DDRPHY_DP16_DQ_BIT_ENABLE0_P0_4 =
	    [all] = 0
	    [48-63] DATA_BIT_ENABLE_0_15 = 0xff00
	*/
	dp_mca_and_or(id, 4, mca_i, 0x800000000701103F, 0, 0xFF00);

	/* IOM0.DDRPHY_DP16_DFT_PDA_CONTROL_P0_{0,1,2,3,4} =
	    // This reg is named MCA_DDRPHY_DP16_DATA_BIT_ENABLE1_P0_n in the code.
	    // Probably the address changed for DD2 but the documentation did not.
	    [all] = 0
	*/
	for (dp = 0; dp < 5; dp++) {
		dp_mca_and_or(id, dp, mca_i, 0x800000010701103F, 0, 0);
	}
}

/* 5 DP16, 8 MCA */
/*
 * These tables specify which clock/strobes pins (16-23) of DP16 are used to
 * capture outgoing/incoming data on which data pins (0-16). Those will
 * eventually arrive to DIMM as DQS and DQ, respectively. The mapping must be
 * the same for write and read, but for some reason HW has two separate sets of
 * registers.
 */
/* TODO: after we know how MCAs are numbered we can drop half of x8 table */
static const uint16_t x4_clk[5] = {0x8640, 0x8640, 0x8640, 0x8640, 0x8400};
static const uint16_t x8_clk[8][5] = {
	{0x0CC0, 0xC0C0, 0x0CC0, 0x0F00, 0x0C00}, /* Port 0 */
	{0xC0C0, 0x0F00, 0x0CC0, 0xC300, 0x0C00}, /* Port 1 */
	{0xC300, 0xC0C0, 0xC0C0, 0x0F00, 0x0C00}, /* Port 2 */
	{0x0F00, 0x0F00, 0xC300, 0xC300, 0xC000}, /* Port 3 */
	{0x0CC0, 0xC0C0, 0x0F00, 0x0F00, 0xC000}, /* Port 4 */
	{0xC300, 0x0CC0, 0x0CC0, 0xC300, 0xC000}, /* Port 5 */
	{0x0CC0, 0x0CC0, 0x0CC0, 0xC0C0, 0x0C00}, /* Port 6 */
	{0x0CC0, 0xC0C0, 0x0F00, 0xC300, 0xC000}, /* Port 7 */
};

static void reset_clock_enable(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	/* Assume the same rank configuration for both DIMMs */
	int dp;
	int width = mca->dimm[0].present ? mca->dimm[0].width :
	                                   mca->dimm[1].width;
	int mranks[2] = {mca->dimm[0].mranks, mca->dimm[1].mranks};
	 /* Index for x8_clk depends on how MCAs are numbered... */
	const uint16_t *clk = width == WIDTH_x4 ? x4_clk :
	                                          x8_clk[mcs_i * MCA_PER_MCS + mca_i];

	/* IOM0.DDRPHY_DP16_WRCLK_EN_RP0_P0_{0,1,2,3,4}
	    [all] = 0
	    [48-63] QUADn_CLKxx
	*/
	/* IOM0.DDRPHY_DP16_READ_CLOCK_RANK_PAIR0_P0_{0,1,2,3,4}
	    [all] = 0
	    [48-63] QUADn_CLKxx
	*/
	for (dp = 0; dp < 5; dp++) {
		/* Note that these correspond to valid rank pairs */
		if (mranks[0] > 0) {
			dp_mca_and_or(id, dp, mca_i, 0x800000050701103F, 0, clk[dp]);
			dp_mca_and_or(id, dp, mca_i, 0x800000040701103F, 0, clk[dp]);
		}

		if (mranks[0] > 1) {
			dp_mca_and_or(id, dp, mca_i, 0x800001050701103F, 0, clk[dp]);
			dp_mca_and_or(id, dp, mca_i, 0x800001040701103F, 0, clk[dp]);
		}

		if (mranks[1] > 0) {
			dp_mca_and_or(id, dp, mca_i, 0x800002050701103F, 0, clk[dp]);
			dp_mca_and_or(id, dp, mca_i, 0x800002040701103F, 0, clk[dp]);
		}

		if (mranks[1] > 1) {
			dp_mca_and_or(id, dp, mca_i, 0x800003050701103F, 0, clk[dp]);
			dp_mca_and_or(id, dp, mca_i, 0x800003040701103F, 0, clk[dp]);
		}
	}
}

/* This comes from VPD */
static const uint32_t ATTR_MSS_VPD_MT_VREF_MC_RD[4] = {
	0x00011cc7,	// 1R in DIMM0 and no DIMM1
	0x00013d6d,	// 1R in both DIMMs
	0x00011c48,	// 2R in DIMM0 and no DIMM1
	0x00014f20,	// 2R in both DIMMs
};

static void reset_rd_vref(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

	int dp;
	int vpd_idx = mca->dimm[0].present ? (mca->dimm[0].mranks == 2 ? 2 : 0) :
	                                     (mca->dimm[1].mranks == 2 ? 2 : 0);
	if (mca->dimm[0].present && mca->dimm[1].present)
		vpd_idx++;

	/*       RD_VREF_DVDD * (100000 - ATTR_MSS_VPD_MT_VREF_MC_RD) / RD_VREF_DAC_STEP
	vref_bf =     12      * (100000 - ATTR_MSS_VPD_MT_VREF_MC_RD) / 6500
	IOM0.DDRPHY_DP16_RD_VREF_DAC_{0-7}_P0_{0-3},
	IOM0.DDRPHY_DP16_RD_VREF_DAC_{0-3}_P0_4 =     // only half of last DP16 is used
	      [49-55] BIT0_VREF_DAC = vref_bf
	      [57-63] BIT1_VREF_DAC = vref_bf
	*/
	for (dp = 0; dp < 5; dp++) {
		uint64_t vref_bf = 12 * (100000 - ATTR_MSS_VPD_MT_VREF_MC_RD[vpd_idx]) / 6500;

		/* SCOM addresses are not regular for DAC, so no inner loop. */
		dp_mca_and_or(id, dp, mca_i, 0x800000160701103F,  // DAC_0
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, 55) | PPC_SHIFT(vref_bf, 63));

		dp_mca_and_or(id, dp, mca_i, 0x8000001F0701103F,  // DAC_1
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, 55) | PPC_SHIFT(vref_bf, 63));

		dp_mca_and_or(id, dp, mca_i, 0x800000C00701103F,  // DAC_2
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, 55) | PPC_SHIFT(vref_bf, 63));

		dp_mca_and_or(id, dp, mca_i, 0x800000C10701103F,  // DAC_3
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, 55) | PPC_SHIFT(vref_bf, 63));

		if (dp == 4) break;

		dp_mca_and_or(id, dp, mca_i, 0x800000C20701103F,  // DAC_4
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, 55) | PPC_SHIFT(vref_bf, 63));

		dp_mca_and_or(id, dp, mca_i, 0x800000C30701103F,  // DAC_5
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, 55) | PPC_SHIFT(vref_bf, 63));

		dp_mca_and_or(id, dp, mca_i, 0x800000C40701103F,  // DAC_6
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, 55) | PPC_SHIFT(vref_bf, 63));

		dp_mca_and_or(id, dp, mca_i, 0x800000C50701103F,  // DAC_7
		              ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		              PPC_SHIFT(vref_bf, 55) | PPC_SHIFT(vref_bf, 63));
	}

	/* IOM0.DDRPHY_DP16_RD_VREF_CAL_EN_P0_{0-4}
	      [48-63] VREF_CAL_EN = 0xffff          // enable = 0xffff, disable = 0x0000
	*/
	for (dp = 0; dp < 5; dp++) {
		/* Is it safe to set this before VREF_DAC? If yes, may use one loop for both */
		dp_mca_and_or(id, dp, mca_i, 0x800000760701103F, 0, PPC_BITMASK(48, 63));
	}
}

static void pc_reset(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	/* These are from VPD */
	uint64_t ATTR_MSS_EFF_DPHY_WLO = mem_data.speed == 1866 ? 1 : 2;
	uint64_t ATTR_MSS_EFF_DPHY_RLO = mem_data.speed == 1866 ? 4 :
	                                 mem_data.speed == 2133 ? 5 :
	                                 mem_data.speed == 2400 ? 6 : 7;

	/* IOM0.DDRPHY_PC_CONFIG0_P0 has been reset in p9n_ddrphy_scom() */

	/* IOM0.DDRPHY_PC_CONFIG1_P0 =
	      [48-51] WRITE_LATENCY_OFFSET =  ATTR_MSS_EFF_DPHY_WLO
	      [52-55] READ_LATENCY_OFFSET =   ATTR_MSS_EFF_DPHY_RLO
			+1: if 2N mode (ATTR_MSS_VPD_MR_MC_2N_MODE_AUTOSET, ATTR_MSS_MRW_DRAM_2N_MODE)  // Gear-down mode in JEDEC
	      // Assume no LRDIMM
	      [59-61] MEMORY_TYPE =           0x5     // 0x7 for LRDIMM
	      [62]    DDR4_LATENCY_SW =       1
	*/
	mca_and_or(id, mca_i, 0x8000C00D0701103F,
	           ~(PPC_BITMASK(48, 55) | PPC_BITMASK(59, 62)),
	           PPC_SHIFT(ATTR_MSS_EFF_DPHY_WLO, 51) | PPC_SHIFT(ATTR_MSS_EFF_DPHY_RLO, 55) |
	           PPC_SHIFT(0x5, 61) | PPC_BIT(62));

	/* IOM0.DDRPHY_PC_ERROR_STATUS0_P0 =
	      [all]   0
	*/
	mca_and_or(id, mca_i, 0x8000C0120701103F, 0, 0);

	/* IOM0.DDRPHY_PC_INIT_CAL_ERROR_P0 =
	      [all]   0
	*/
	mca_and_or(id, mca_i, 0x8000C0180701103F, 0, 0);
}

static void wc_reset(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

	/* IOM0.DDRPHY_WC_CONFIG0_P0 =
	      [all]   0
	      // BUG? Mismatch between comment (-,-), code (+,+) and docs (-,+) for operations inside 'max'
	      [48-55] TWLO_TWLOE =        12 + max((twldqsen - tmod), (twlo + twlow))
				   + longest DQS delay in clocks (rounded up) + longest DQ delay in clocks (rounded up)
	      [56]    WL_ONE_DQS_PULSE =  1
	      [57-62] FW_WR_RD =          0x20      // "# dd0 = 17 clocks, now 32 from SWyatt"
	      [63]    CUSTOM_INIT_WRITE = 1         // set to a 1 to get proper values for RD VREF
	*/
	/*
	 * tMOD = max(24 nCK, 15 ns) = 24 nCK for all supported speed bins
	 * tWLDQSEN = 25 nCK
	 * tWLO = 0 - 9.5 ns, Hostboot uses ATTR_MSS_EFF_DPHY_WLO
	 * tWLOE = 0 - 2 ns, Hostboot uses 2 ns
	 * Longest DQ and DQS delays are both equal 1 nCK.
	 */
	uint64_t tWLO = mem_data.speed == 1866 ? 1 : 2;
	uint64_t tWLOE = ns_to_nck(2);
	/*
	 * Use the version from the code, it may be longer than necessary but it
	 * works. Note that MAX() always expands to 25 + 24 = 49, which means that
	 * we can just write 'tWLO_tWLOE = 63'. Leaving full version below, it will
	 * be easier to fix.
	 */
	uint64_t tWLO_tWLOE = 12 + MAX((25 + 24), (tWLO + tWLOE)) + 1 + 1;
	mca_and_or(id, mca_i, 0x8000CC000701103F, 0,
	           PPC_SHIFT(tWLO_tWLOE, 55) | PPC_BIT(56) |
	           PPC_SHIFT(0x20, 62) | PPC_BIT(63));

	/* IOM0.DDRPHY_WC_CONFIG1_P0 =
	      [all]   0
	      [48-51] BIG_STEP =          7
	      [52-54] SMALL_STEP =        0
	      [55-60] WR_PRE_DLY =        0x2a (42)
	*/
	mca_and_or(id, mca_i, 0x8000CC010701103F, 0,
	           PPC_SHIFT(7, 51) | PPC_SHIFT(0x2A, 60));

	/* IOM0.DDRPHY_WC_CONFIG2_P0 =
	      [all]   0
	      [48-51] NUM_VALID_SAMPLES = 5
	      [52-57] FW_RD_WR =          max(tWTR_S + 11, AL + tRTP + 3)
	      [58-61] IPW_WR_WR =         5     // results in 24 clock cycles
	*/
	/* There is no Additive Latency. */
	mca_and_or(id, mca_i, 0x8000CC020701103F, 0,
	           PPC_SHIFT(5, 51) | PPC_SHIFT(5, 61) |
	           PPC_SHIFT(MAX(mca->nwtr_s + 11, mem_data.nrtp + 3), 57));

	/* IOM0.DDRPHY_WC_CONFIG3_P0 =
	      [all]   0
	      [55-60] MRS_CMD_DQ_OFF =    0x3f
	*/
	mca_and_or(id, mca_i, 0x8000CC050701103F, 0, PPC_SHIFT(0x3F, 60));

	/* IOM0.DDRPHY_WC_RTT_WR_SWAP_ENABLE_P0    // 0x8000CC060701103F
	      [48]    WL_ENABLE_RTT_SWAP =            0
	      [49]    WR_CTR_ENABLE_RTT_SWAP =        0
	      [50-59] WR_CTR_VREF_COUNTER_RESET_VAL = 150ns in clock cycles  // JESD79-4C Table 67
	*/
	mca_and_or(id, mca_i, 0x8000CC060701103F, ~PPC_BITMASK(48, 59),
	           PPC_SHIFT(ns_to_nck(150), 59));
}

static void rc_reset(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

	/* IOM0.DDRPHY_RC_CONFIG0_P0
	      [all]   0
	      [48-51] GLOBAL_PHY_OFFSET =
			  MSS_FREQ_EQ_1866: 12
			  MSS_FREQ_EQ_2133: 12
			  MSS_FREQ_EQ_2400: 13
			  MSS_FREQ_EQ_2666: 13
	      [62]    PERFORM_RDCLK_ALIGN = 1
	*/
	uint64_t global_phy_offset = mem_data.speed < 2400 ? 12 : 13;
	mca_and_or(id, mca_i, 0x8000C8000701103F, 0,
	           PPC_SHIFT(global_phy_offset, 51) | PPC_BIT(62));

	/* IOM0.DDRPHY_RC_CONFIG1_P0
	      [all]   0
	*/
	mca_and_or(id, mca_i, 0x8000C8010701103F, 0, 0);

	/* IOM0.DDRPHY_RC_CONFIG2_P0
	      [all]   0
	      [48-52] CONSEC_PASS = 8
	      [57-58] 3                   // not documented, BURST_WINDOW?
	*/
	mca_and_or(id, mca_i, 0x8000C8020701103F, 0,
	           PPC_SHIFT(8, 52) | PPC_SHIFT(3, 58));

	/* IOM0.DDRPHY_RC_CONFIG3_P0
	      [all]   0
	      [51-54] COARSE_CAL_STEP_SIZE = 4  // 5/128
	*/
	mca_and_or(id, mca_i, 0x8000C8070701103F, 0, PPC_SHIFT(4, 54));

	/* IOM0.DDRPHY_RC_RDVREF_CONFIG0_P0 =
	      [all]   0
	      [48-63] WAIT_TIME =
			  0xffff          // as slow as possible, or use calculation from vref_guess_time(), or:
			  MSS_FREQ_EQ_1866: 0x0804
			  MSS_FREQ_EQ_2133: 0x092a
			  MSS_FREQ_EQ_2400: 0x0a50
			  MSS_FREQ_EQ_2666: 0x0b74    // use this value for all freqs maybe?
	*/
	uint64_t wait_time = mem_data.speed == 1866 ? 0x0804 :
	                     mem_data.speed == 2133 ? 0x092A :
	                     mem_data.speed == 2400 ? 0x0A50 : 0x0B74;
	mca_and_or(id, mca_i, 0x8000C8090701103F, 0, PPC_SHIFT(wait_time, 63));

	/* IOM0.DDRPHY_RC_RDVREF_CONFIG1_P0 =
	      [all]   0
	      [48-55] CMD_PRECEDE_TIME =  (AL + CL + 15)
	      [56-59] MPR_LOCATION =      4     // "From R. King."
	*/
	mca_and_or(id, mca_i, 0x8000C80A0701103F, 0,
	           PPC_SHIFT(mca->cl + 15, 55) | PPC_SHIFT(4, 59));
}

static inline int log2_up(uint32_t x)
{
	int lz;
	asm("cntlzd %0, %1" : "=r"(lz) : "r"((x << 1) - 1));
	return 63 - lz;
}

/*
 * Warning: this is not a 1:1 copy from VPD.
 *
 * VPD uses uint8_t [2][2][4] table, indexed as [MCA][DIMM][RANK]. It tries to
 * be generic, but for RDIMMs only 2 ranks are supported. This format also
 * allows for different settings across MCAs, but in Talos they are identical.
 *
 * Tables below are uint8_t [4][2][2], indexed as [rank config.][DIMM][RANK].
 *
 * There are 4 rank configurations, see comments in ATTR_MSS_VPD_MT_VREF_MC_RD.
 */
static const uint8_t ATTR_MSS_VPD_MT_ODT_RD[4][2][2] = {
	{ {0x00, 0x00}, {0x00, 0x00} },	// 1R in DIMM0 and no DIMM1
	{ {0x08, 0x00}, {0x80, 0x00} },	// 1R in both DIMMs
	{ {0x00, 0x00}, {0x00, 0x00} },	// 2R in DIMM0 and no DIMM1
	{ {0x40, 0x80}, {0x04, 0x08} },	// 2R in both DIMMs
};

static const uint8_t ATTR_MSS_VPD_MT_ODT_WR[4][2][2] = {
	{ {0x80, 0x00}, {0x00, 0x00} },	// 1R in DIMM0 and no DIMM1
	{ {0x88, 0x00}, {0x88, 0x00} },	// 1R in both DIMMs
	{ {0x40, 0x80}, {0x00, 0x00} },	// 2R in DIMM0 and no DIMM1
	{ {0xC0, 0xC0}, {0x0C, 0x0C} },	// 2R in both DIMMs
};

static void seq_reset(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	int vpd_idx = mca->dimm[0].present ? (mca->dimm[0].mranks == 2 ? 2 : 0) :
	                                     (mca->dimm[1].mranks == 2 ? 2 : 0);
	if (mca->dimm[0].present && mca->dimm[1].present)
		vpd_idx++;


	/* IOM0.DDRPHY_SEQ_CONFIG0_P0 =
	      [all]   0
	      [49]    TWO_CYCLE_ADDR_EN =
			  2N mode:                1
			  else:                   0
	      [54]    DELAYED_PAR = 1
	      [62]    PAR_A17_MASK =
			  16Gb x4 configuration:  0
			  else:                   1
	*/
	uint64_t par_a17_mask = PPC_BIT(62);
	if ((mca->dimm[0].width == WIDTH_x4 && mca->dimm[0].density == DENSITY_16Gb) ||
	    (mca->dimm[1].width == WIDTH_x4 && mca->dimm[1].density == DENSITY_16Gb))
		par_a17_mask = 0;

	mca_and_or(id, mca_i, 0x8000C4020701103F, 0,
	           PPC_BIT(54) | par_a17_mask);

	/* All log2 values in timing registers are rounded up. */
	/* IOM0.DDRPHY_SEQ_MEM_TIMING_PARAM0_P0 =
	      [all]   0
	      [48-51] TMOD_CYCLES = 5           // log2(max(tMRD, tMOD)) = log2(24), JEDEC tables 169 and 170 and section 13.5
	      [52-55] TRCD_CYCLES = log2(tRCD)
	      [56-59] TRP_CYCLES =  log2(tRP)
	      [60-63] TRFC_CYCLES = log2(tRFC)
	*/
	mca_and_or(id, mca_i, 0x8000C4120701103F, 0,
	           PPC_SHIFT(5, 51) | PPC_SHIFT(log2_up(mca->nrcd), 55) |
	           PPC_SHIFT(log2_up(mca->nrp), 59) | PPC_SHIFT(log2_up(mca->nrfc), 63));

	/* IOM0.DDRPHY_SEQ_MEM_TIMING_PARAM1_P0 =
	      [all]   0
	      [48-51] TZQINIT_CYCLES =  10      // log2(1024), JEDEC tables 169 and 170
	      [52-55] TZQCS_CYCLES =    7       // log2(128), JEDEC tables 169 and 170
	      [56-59] TWLDQSEN_CYCLES = 5       // log2(25) rounded up, JEDEC tables 169 and 170
	      [60-63] TWRMRD_CYCLES =   6       // log2(40) rounded up, JEDEC tables 169 and 170
	*/
	mca_and_or(id, mca_i, 0x8000C4130701103F, 0,
	           PPC_SHIFT(10, 51) | PPC_SHIFT(7, 55) |
	           PPC_SHIFT(5, 59) | PPC_SHIFT(6, 63));

	/* IOM0.DDRPHY_SEQ_MEM_TIMING_PARAM2_P0 =
	      [all]   0
	      [48-51] TODTLON_OFF_CYCLES =  log2(CWL + AL + PL - 2)
	      [52-63] =                     0x777     // "Reset value of SEQ_TIMING2 is lucky 7's"
	*/
	/* AL and PL are disabled (0) */
	mca_and_or(id, mca_i, 0x8000C4140701103F, 0,
	           PPC_SHIFT(log2_up(mem_data.cwl - 2), 51) | PPC_SHIFT(0x777, 63));

	/* IOM0.DDRPHY_SEQ_RD_WR_DATA0_P0 =
	      [all]   0
	      [48-63] RD_RW_DATA_REG0 = 0xaa00
	*/
	mca_and_or(id, mca_i, 0x8000C4000701103F, 0, PPC_SHIFT(0xAA00, 63));

	/* IOM0.DDRPHY_SEQ_RD_WR_DATA1_P0 =
	      [all]   0
	      [48-63] RD_RW_DATA_REG1 = 0x00aa
	*/
	mca_and_or(id, mca_i, 0x8000C4010701103F, 0, PPC_SHIFT(0x00AA, 63));

	/*
	 * For all registers below, assume RDIMM (max 2 ranks).
	 *
	 * Remember that VPD data layout is different, code will be slightly
	 * different than the comments.
	 */
#define F(x)	(((x >> 4) & 0xC) | ((x >> 2) & 0x3))

	/* IOM0.DDRPHY_SEQ_ODT_RD_CONFIG0_P0 =
	      F(X) = (((X >> 4) & 0xc) | ((X >> 2) & 0x3))    // Bits 0,1,4,5 of X, see also MC01.PORT0.SRQ.MBA_FARB2Q
	      [all]   0
	      [48-51] ODT_RD_VALUES0 = F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][0][0])
	      [56-59] ODT_RD_VALUES1 = F(ATTR_MSS_VPD_MT_ODT_RD[index(MCA)][0][1])
	*/
	mca_and_or(id, mca_i, 0x8000C40E0701103F, 0,
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][0][0]), 51) |
	           PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][0][1]), 59));

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

	/* IOM0.DDRPHY_SEQ_ODT_WR_CONFIG1_P0 =     // 0x8000C40B0701103F
	      F(X) = (((X >> 4) & 0xc) | ((X >> 2) & 0x3))    // Bits 0,1,4,5 of X, see also MC01.PORT0.SRQ.MBA_FARB2Q
	      [all]   0
	      [48-51] ODT_WR_VALUES2 =
			count_dimm(MCA) == 2: F(ATTR_MSS_VPD_MT_ODT_WR[index(MCA)][1][0])
			count_dimm(MCA) != 2: F(ATTR_MSS_VPD_MT_ODT_WR[index(MCA)][0][2])
	      [56-59] ODT_WR_VALUES3 =
			count_dimm(MCA) == 2: F(ATTR_MSS_VPD_MT_ODT_WR[index(MCA)][1][1])
			count_dimm(MCA) != 2: F(ATTR_MSS_VPD_MT_ODT_WR[index(MCA)][0][3])
	*/
	val = 0;
	if (vpd_idx % 2)
		val = PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][1][0]), 51) |
		      PPC_SHIFT(F(ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][1][1]), 59);
#undef F
}

static void reset_ac_boost_cntl(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	/* IOM0.DDRPHY_DP16_ACBOOST_CTL_BYTE{0,1}_P0_{0,1,2,3,4} =
	    // For all of the AC Boost attributes, they're laid out in the uint32_t as such:
	    // Bit 0-2   = DP16 Block 0 (DQ Bits 0-7)       BYTE0_P0_0
	    // Bit 3-5   = DP16 Block 0 (DQ Bits 8-15)      BYTE1_P0_0
	    // Bit 6-8   = DP16 Block 1 (DQ Bits 0-7)       BYTE0_P0_1
	    // Bit 9-11  = DP16 Block 1 (DQ Bits 8-15)      BYTE1_P0_1
	    // Bit 12-14 = DP16 Block 2 (DQ Bits 0-7)       BYTE0_P0_2
	    // Bit 15-17 = DP16 Block 2 (DQ Bits 8-15)      BYTE1_P0_2
	    // Bit 18-20 = DP16 Block 3 (DQ Bits 0-7)       BYTE0_P0_3
	    // Bit 21-23 = DP16 Block 3 (DQ Bits 8-15)      BYTE1_P0_3
	    // Bit 24-26 = DP16 Block 4 (DQ Bits 0-7)       BYTE0_P0_4
	    // Bit 27-29 = DP16 Block 4 (DQ Bits 8-15)      BYTE1_P0_4
	    [all]   0?    // function does read prev values from SCOM but then overwrites all non-const-0 fields. Why bother?
	    [48-50] S{0,1}ACENSLICENDRV_DC = appropriate bits from ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_DOWN
	    [51-53] S{0,1}ACENSLICENDRV_DC = appropriate bits from ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_UP
	    [54-56] S{0,1}ACENSLICENDRV_DC = appropriate bits from ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_RD_UP
	*/
	/*
	 * Both ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_* have a value of 0x24924924
	 * for all rank configurations (two copies for two MCA indices to be exact),
	 * meaning that all 3b fields are 0b001. ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_RD_UP
	 * equals 0. Last DP16 doesn't require special handling, all DQ bits are
	 * configured.
	 *
	 * Write these fields explicitly instead of shifting and masking for better
	 * readability.
	 */
	for (dp = 0; dp < 5; dp++) {
		dp_mca_and_or(id, dp, mca_i, 0x800000220701103F, ~PPC_BITMASK(48, 56),
		              PPC_SHIFT(1, 50) | PPC_SHIFT(1, 53));
		dp_mca_and_or(id, dp, mca_i, 0x800000230701103F, ~PPC_BITMASK(48, 56),
		              PPC_SHIFT(1, 50) | PPC_SHIFT(1, 53));
	}
}

static void reset_ctle_cntl(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	/* IOM0.DDRPHY_DP16_CTLE_CTL_BYTE{0,1}_P0_{0,1,2,3,4} =        // 0x8000002{0,1}0701103F, +0x0400_0000_0000
	    // For the capacitance CTLE attributes, they're laid out in the uint64_t as such. The resitance
	    // attributes are the same, but 3 bits long. Notice that DP Block X Nibble 0 is DQ0:3,
	    // Nibble 1 is DQ4:7, Nibble 2 is DQ8:11 and 3 is DQ12:15.
	    // Bit 0-1   = DP16 Block 0 Nibble 0     Bit 16-17 = DP16 Block 2 Nibble 0     Bit 32-33 = DP16 Block 4 Nibble 0
	    // Bit 2-3   = DP16 Block 0 Nibble 1     Bit 18-19 = DP16 Block 2 Nibble 1     Bit 34-35 = DP16 Block 4 Nibble 1
	    // Bit 4-5   = DP16 Block 0 Nibble 2     Bit 20-21 = DP16 Block 2 Nibble 2     Bit 36-37 = DP16 Block 4 Nibble 2
	    // Bit 6-7   = DP16 Block 0 Nibble 3     Bit 22-23 = DP16 Block 2 Nibble 3     Bit 38-39 = DP16 Block 4 Nibble 3
	    // Bit 8-9   = DP16 Block 1 Nibble 0     Bit 24-25 = DP16 Block 3 Nibble 0
	    // Bit 10-11 = DP16 Block 1 Nibble 1     Bit 26-27 = DP16 Block 3 Nibble 1
	    // Bit 12-13 = DP16 Block 1 Nibble 2     Bit 28-29 = DP16 Block 3 Nibble 2
	    // Bit 14-15 = DP16 Block 1 Nibble 3     Bit 30-31 = DP16 Block 3 Nibble 3
	    [48-49] NIB_{0,2}_DQSEL_CAP = appropriate bits from ATTR_MSS_VPD_MT_MC_DQ_CTLE_CAP
	    [53-55] NIB_{0,2}_DQSEL_RES = appropriate bits from ATTR_MSS_VPD_MT_MC_DQ_CTLE_RES
	    [56-57] NIB_{1,3}_DQSEL_CAP = appropriate bits from ATTR_MSS_VPD_MT_MC_DQ_CTLE_CAP
	    [61-63] NIB_{1,3}_DQSEL_RES = appropriate bits from ATTR_MSS_VPD_MT_MC_DQ_CTLE_RES
	*/
	/*
	 * For all rank configurations and both MCAs, ATTR_MSS_VPD_MT_MC_DQ_CTLE_CAP
	 * is 0x5555555555000000 (so every 2b field is 0b01) and *_RES equals
	 * 0xb6db6db6db6db6d0 (every 3b field is 0b101 = 5).
	 */
	for (dp = 0; dp < 5; dp++) {
		dp_mca_and_or(id, dp, mca_i, 0x800000200701103F,
		              ~(PPC_BITMASK(48, 49) | PPC_BITMASK(53, 57) | PPC_BITMASK(61, 63)),
		              PPC_SHIFT(1, 49) | PPC_SHIFT(5, 55) |
		              PPC_SHIFT(1, 57) | PPC_SHIFT(5, 63));
		dp_mca_and_or(id, dp, mca_i, 0x800000210701103F,
		              ~(PPC_BITMASK(48, 49) | PPC_BITMASK(53, 57) | PPC_BITMASK(61, 63)),
		              PPC_SHIFT(1, 49) | PPC_SHIFT(5, 55) |
		              PPC_SHIFT(1, 57) | PPC_SHIFT(5, 63));
	}
}

/*
 * 43 tables for 43 signals. These probably are platform specific so in the
 * final version we should read this from VPD partition. Hardcoding it will make
 * one less possible fault point - compiler will warn about unused variables.
 *
 * Also, VPD layout may change. Right npw Talos uses first version of layout,
 * but there is a newer version with one additional field __in the middle__ of
 * the structure.
 */
static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN0[][MCA_PER_MCS] = {
	{ 0x1b, 0x1d },	// J0 - PROC 0 MCS 0, 1 DIMM, 1866 MT/s
	{ 0x24, 0x15 },	// J1 - PROC 0 MCS 1, 1 DIMM, 1866 MT/s
	{ 0x22, 0x18 },	// J2 - PROC 1 MCS 0, 1 DIMM, 1866 MT/s
	{ 0x1f, 0x14 },	// J3 - PROC 1 MCS 1, 1 DIMM, 1866 MT/s
	{ 0x15, 0x17 },	// J4 - PROC 0 MCS 0, 2 DIMMs, 1866 MT/s
	{ 0x1d, 0x10 },	// J5
	{ 0x1c, 0x12 },	// J6
	{ 0x1a, 0x0e },	// J7 - PROC 1 MCS 1, 2 DIMMs, 1866 MT/s
	{ 0x1b, 0x1d },	// J8 - PROC 0 MCS 0, 1 DIMM, 2133 MT/s
	{ 0x24, 0x15 },	// J9
	{ 0x22, 0x18 },	// JA
	{ 0x1f, 0x14 },	// JB - PROC 1 MCS 1, 1 DIMM, 2133 MT/s
	{ 0x15, 0x17 },	// JC - PROC 0 MCS 0, 2 DIMMs, 2133 MT/s
	{ 0x1d, 0x10 },	// JD
	{ 0x1c, 0x12 },	// JE
	{ 0x1a, 0x0e },	// JF - PROC 1 MCS 1, 2 DIMMs, 2133 MT/s
	{ 0x1b, 0x1d },	// JG - PROC 0 MCS 0, 1 DIMM, 2400 MT/s
	{ 0x24, 0x15 },	// JH
	{ 0x22, 0x18 },	// JI
	{ 0x1f, 0x14 },	// JJ - PROC 1 MCS 1, 1 DIMM, 2400 MT/s
	{ 0x18, 0x19 },	// JK - PROC 0 MCS 0, 2 DIMMs, 2400 MT/s
	{ 0x21, 0x12 },	// JL
	{ 0x1f, 0x14 },	// JM
	{ 0x1c, 0x11 },	// JN - PROC 1 MCS 1, 2 DIMMs, 2400 MT/s
	{ 0x1f, 0x20 },	// JO - PROC 0 MCS 0, 1 DIMM, 2666 MT/s
	{ 0x2a, 0x19 },	// JP
	{ 0x26, 0x1a },	// JQ
	{ 0x23, 0x18 },	// JR - PROC 1 MCS 1, 1 DIMM, 2666 MT/s
	// 2 DIMMs, 2666 MT/s is not supported. This is ensured by prepare_dimm_data()
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_WEN_A14[][MCA_PER_MCS] = {
	{ 0x14, 0x18 },	// J0
	{ 0x11, 0x0e },	// J1
	{ 0x1b, 0x13 },	// J2
	{ 0x10, 0x0b },	// J3
	{ 0x12, 0x15 },	// J4
	{ 0x0f, 0x0c },	// J5
	{ 0x18, 0x11 },	// J6
	{ 0x0f, 0x09 },	// J7
	{ 0x14, 0x18 },	// J8
	{ 0x11, 0x0e },	// J9
	{ 0x1b, 0x13 },	// JA
	{ 0x10, 0x0b },	// JB
	{ 0x12, 0x15 },	// JC
	{ 0x0f, 0x0c },	// JD
	{ 0x18, 0x11 },	// JE
	{ 0x0f, 0x09 },	// JF
	{ 0x14, 0x18 },	// JG
	{ 0x11, 0x0e },	// JH
	{ 0x1b, 0x13 },	// JI
	{ 0x10, 0x0b },	// JJ
	{ 0x14, 0x17 },	// JK
	{ 0x11, 0x0e },	// JL
	{ 0x1b, 0x13 },	// JM
	{ 0x11, 0x0b },	// JN
	{ 0x17, 0x19 },	// JO
	{ 0x14, 0x10 },	// JP
	{ 0x1e, 0x15 },	// JQ
	{ 0x12, 0x0c },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT1[][MCA_PER_MCS] = {
	{ 0x1e, 0x18 },	// J0
	{ 0x1f, 0x12 },	// J1
	{ 0x1f, 0x19 },	// J2
	{ 0x1e, 0x18 },	// J3
	{ 0x0f, 0x11 },	// J4
	{ 0x16, 0x08 },	// J5
	{ 0x0e, 0x16 },	// J6
	{ 0x10, 0x0e },	// J7
	{ 0x1e, 0x18 },	// J8
	{ 0x1f, 0x12 },	// J9
	{ 0x1f, 0x19 },	// JA
	{ 0x1e, 0x18 },	// JB
	{ 0x0f, 0x11 },	// JC
	{ 0x16, 0x08 },	// JD
	{ 0x0e, 0x16 },	// JE
	{ 0x10, 0x0e },	// JF
	{ 0x1e, 0x18 },	// JG
	{ 0x1f, 0x12 },	// JH
	{ 0x1f, 0x19 },	// JI
	{ 0x1e, 0x18 },	// JJ
	{ 0x10, 0x12 },	// JK
	{ 0x17, 0x08 },	// JL
	{ 0x0f, 0x18 },	// JM
	{ 0x11, 0x0f },	// JN
	{ 0x23, 0x1b },	// JO
	{ 0x25, 0x15 },	// JP
	{ 0x23, 0x1c },	// JQ
	{ 0x22, 0x1c },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C0[][MCA_PER_MCS] = {
	{ 0x11, 0x12 },	// J0
	{ 0x11, 0x08 },	// J1
	{ 0x13, 0x15 },	// J2
	{ 0x0e, 0x0d },	// J3
	{ 0x0f, 0x11 },	// J4
	{ 0x0f, 0x07 },	// J5
	{ 0x11, 0x12 },	// J6
	{ 0x0d, 0x0b },	// J7
	{ 0x11, 0x12 },	// J8
	{ 0x11, 0x08 },	// J9
	{ 0x13, 0x15 },	// JA
	{ 0x0e, 0x0d },	// JB
	{ 0x0f, 0x11 },	// JC
	{ 0x0f, 0x07 },	// JD
	{ 0x11, 0x12 },	// JE
	{ 0x0d, 0x0b },	// JF
	{ 0x11, 0x12 },	// JG
	{ 0x11, 0x08 },	// JH
	{ 0x13, 0x15 },	// JI
	{ 0x0e, 0x0d },	// JJ
	{ 0x11, 0x12 },	// JK
	{ 0x11, 0x07 },	// JL
	{ 0x13, 0x15 },	// JM
	{ 0x0f, 0x0d },	// JN
	{ 0x13, 0x13 },	// JO
	{ 0x14, 0x08 },	// JP
	{ 0x15, 0x17 },	// JQ
	{ 0x10, 0x0e },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA1[][MCA_PER_MCS] = {
	{ 0x14, 0x13 },	// J0
	{ 0x12, 0x0a },	// J1
	{ 0x19, 0x0f },	// J2
	{ 0x19, 0x0d },	// J3
	{ 0x12, 0x11 },	// J4
	{ 0x10, 0x09 },	// J5
	{ 0x16, 0x0d },	// J6
	{ 0x17, 0x0b },	// J7
	{ 0x14, 0x13 },	// J8
	{ 0x12, 0x0a },	// J9
	{ 0x19, 0x0f },	// JA
	{ 0x19, 0x0d },	// JB
	{ 0x12, 0x11 },	// JC
	{ 0x10, 0x09 },	// JD
	{ 0x16, 0x0d },	// JE
	{ 0x17, 0x0b },	// JF
	{ 0x14, 0x13 },	// JG
	{ 0x12, 0x0a },	// JH
	{ 0x19, 0x0f },	// JI
	{ 0x19, 0x0d },	// JJ
	{ 0x14, 0x12 },	// JK
	{ 0x12, 0x0a },	// JL
	{ 0x19, 0x0f },	// JM
	{ 0x19, 0x0d },	// JN
	{ 0x17, 0x14 },	// JO
	{ 0x15, 0x0b },	// JP
	{ 0x1b, 0x10 },	// JQ
	{ 0x1c, 0x0f },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A10[][MCA_PER_MCS] = {
	{ 0x14, 0x15 },	// J0
	{ 0x15, 0x10 },	// J1
	{ 0x19, 0x12 },	// J2
	{ 0x1a, 0x0e },	// J3
	{ 0x11, 0x13 },	// J4
	{ 0x13, 0x0e },	// J5
	{ 0x16, 0x0f },	// J6
	{ 0x17, 0x0c },	// J7
	{ 0x14, 0x15 },	// J8
	{ 0x15, 0x10 },	// J9
	{ 0x19, 0x12 },	// JA
	{ 0x1a, 0x0e },	// JB
	{ 0x11, 0x13 },	// JC
	{ 0x13, 0x0e },	// JD
	{ 0x16, 0x0f },	// JE
	{ 0x17, 0x0c },	// JF
	{ 0x14, 0x15 },	// JG
	{ 0x15, 0x10 },	// JH
	{ 0x19, 0x12 },	// JI
	{ 0x1a, 0x0e },	// JJ
	{ 0x14, 0x14 },	// JK
	{ 0x16, 0x10 },	// JL
	{ 0x19, 0x11 },	// JM
	{ 0x1a, 0x0e },	// JN
	{ 0x16, 0x16 },	// JO
	{ 0x19, 0x12 },	// JP
	{ 0x1c, 0x13 },	// JQ
	{ 0x1c, 0x10 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT1[][MCA_PER_MCS] = {
	{ 0x1e, 0x18 },	// J0
	{ 0x1f, 0x12 },	// J1
	{ 0x1f, 0x19 },	// J2
	{ 0x1e, 0x18 },	// J3
	{ 0x18, 0x14 },	// J4
	{ 0x19, 0x0d },	// J5
	{ 0x19, 0x13 },	// J6
	{ 0x18, 0x12 },	// J7
	{ 0x1e, 0x18 },	// J8
	{ 0x1f, 0x12 },	// J9
	{ 0x1f, 0x19 },	// JA
	{ 0x1e, 0x18 },	// JB
	{ 0x18, 0x14 },	// JC
	{ 0x19, 0x0d },	// JD
	{ 0x19, 0x13 },	// JE
	{ 0x18, 0x12 },	// JF
	{ 0x1e, 0x18 },	// JG
	{ 0x1f, 0x12 },	// JH
	{ 0x1f, 0x19 },	// JI
	{ 0x1e, 0x18 },	// JJ
	{ 0x1b, 0x14 },	// JK
	{ 0x1c, 0x0f },	// JL
	{ 0x1c, 0x16 },	// JM
	{ 0x1a, 0x15 },	// JN
	{ 0x23, 0x1b },	// JO
	{ 0x25, 0x15 },	// JP
	{ 0x23, 0x1c },	// JQ
	{ 0x22, 0x1c },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA0[][MCA_PER_MCS] = {
	{ 0x11, 0x13 },	// J0
	{ 0x14, 0x0d },	// J1
	{ 0x17, 0x13 },	// J2
	{ 0x1a, 0x0a },	// J3
	{ 0x0f, 0x11 },	// J4
	{ 0x11, 0x0b },	// J5
	{ 0x15, 0x11 },	// J6
	{ 0x17, 0x09 },	// J7
	{ 0x11, 0x13 },	// J8
	{ 0x14, 0x0d },	// J9
	{ 0x17, 0x13 },	// JA
	{ 0x1a, 0x0a },	// JB
	{ 0x0f, 0x11 },	// JC
	{ 0x11, 0x0b },	// JD
	{ 0x15, 0x11 },	// JE
	{ 0x17, 0x09 },	// JF
	{ 0x11, 0x13 },	// JG
	{ 0x14, 0x0d },	// JH
	{ 0x17, 0x13 },	// JI
	{ 0x1a, 0x0a },	// JJ
	{ 0x12, 0x13 },	// JK
	{ 0x14, 0x0d },	// JL
	{ 0x18, 0x13 },	// JM
	{ 0x1a, 0x0a },	// JN
	{ 0x14, 0x14 },	// JO
	{ 0x17, 0x0e },	// JP
	{ 0x1a, 0x15 },	// JQ
	{ 0x1c, 0x0b },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A00[][MCA_PER_MCS] = {
	{ 0x18, 0x16 },	// J0
	{ 0x17, 0x11 },	// J1
	{ 0x17, 0x12 },	// J2
	{ 0x18, 0x14 },	// J3
	{ 0x15, 0x14 },	// J4
	{ 0x15, 0x0f },	// J5
	{ 0x15, 0x10 },	// J6
	{ 0x16, 0x12 },	// J7
	{ 0x18, 0x16 },	// J8
	{ 0x17, 0x11 },	// J9
	{ 0x17, 0x12 },	// JA
	{ 0x18, 0x14 },	// JB
	{ 0x15, 0x14 },	// JC
	{ 0x15, 0x0f },	// JD
	{ 0x15, 0x10 },	// JE
	{ 0x16, 0x12 },	// JF
	{ 0x18, 0x16 },	// JG
	{ 0x17, 0x11 },	// JH
	{ 0x17, 0x12 },	// JI
	{ 0x18, 0x14 },	// JJ
	{ 0x18, 0x16 },	// JK
	{ 0x18, 0x11 },	// JL
	{ 0x18, 0x12 },	// JM
	{ 0x19, 0x14 },	// JN
	{ 0x1c, 0x17 },	// JO
	{ 0x1b, 0x13 },	// JP
	{ 0x1a, 0x13 },	// JQ
	{ 0x1b, 0x16 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT0[][MCA_PER_MCS] = {
	{ 0x1d, 0x1d },	// J0
	{ 0x20, 0x0f },	// J1
	{ 0x20, 0x1a },	// J2
	{ 0x20, 0x17 },	// J3
	{ 0x14, 0x13 },	// J4
	{ 0x17, 0x0f },	// J5
	{ 0x16, 0x16 },	// J6
	{ 0x19, 0x14 },	// J7
	{ 0x1d, 0x1d },	// J8
	{ 0x20, 0x0f },	// J9
	{ 0x20, 0x1a },	// JA
	{ 0x20, 0x17 },	// JB
	{ 0x14, 0x13 },	// JC
	{ 0x17, 0x0f },	// JD
	{ 0x16, 0x16 },	// JE
	{ 0x19, 0x14 },	// JF
	{ 0x1d, 0x1d },	// JG
	{ 0x20, 0x0f },	// JH
	{ 0x20, 0x1a },	// JI
	{ 0x20, 0x17 },	// JJ
	{ 0x16, 0x14 },	// JK
	{ 0x19, 0x10 },	// JL
	{ 0x17, 0x19 },	// JM
	{ 0x1c, 0x16 },	// JN
	{ 0x21, 0x20 },	// JO
	{ 0x25, 0x12 },	// JP
	{ 0x25, 0x1d },	// JQ
	{ 0x24, 0x1b },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT0[][MCA_PER_MCS] = {
	{ 0x1d, 0x1d },	// J0
	{ 0x20, 0x0f },	// J1
	{ 0x20, 0x1a },	// J2
	{ 0x20, 0x17 },	// J3
	{ 0x17, 0x17 },	// J4
	{ 0x19, 0x0a },	// J5
	{ 0x1a, 0x13 },	// J6
	{ 0x1a, 0x11 },	// J7
	{ 0x1d, 0x1d },	// J8
	{ 0x20, 0x0f },	// J9
	{ 0x20, 0x1a },	// JA
	{ 0x20, 0x17 },	// JB
	{ 0x17, 0x17 },	// JC
	{ 0x19, 0x0a },	// JD
	{ 0x1a, 0x13 },	// JE
	{ 0x1a, 0x11 },	// JF
	{ 0x1d, 0x1d },	// JG
	{ 0x20, 0x0f },	// JH
	{ 0x20, 0x1a },	// JI
	{ 0x20, 0x17 },	// JJ
	{ 0x1a, 0x19 },	// JK
	{ 0x1d, 0x0c },	// JL
	{ 0x1d, 0x16 },	// JM
	{ 0x1c, 0x14 },	// JN
	{ 0x21, 0x20 },	// JO
	{ 0x25, 0x12 },	// JP
	{ 0x25, 0x1d },	// JQ
	{ 0x24, 0x1b },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_CASN_A15[][MCA_PER_MCS] = {
	{ 0x17, 0x12 },	// J0
	{ 0x17, 0x15 },	// J1
	{ 0x1b, 0x12 },	// J2
	{ 0x1c, 0x13 },	// J3
	{ 0x14, 0x11 },	// J4
	{ 0x14, 0x12 },	// J5
	{ 0x18, 0x10 },	// J6
	{ 0x1a, 0x11 },	// J7
	{ 0x17, 0x12 },	// J8
	{ 0x17, 0x15 },	// J9
	{ 0x1b, 0x12 },	// JA
	{ 0x1c, 0x13 },	// JB
	{ 0x14, 0x11 },	// JC
	{ 0x14, 0x12 },	// JD
	{ 0x18, 0x10 },	// JE
	{ 0x1a, 0x11 },	// JF
	{ 0x17, 0x12 },	// JG
	{ 0x17, 0x15 },	// JH
	{ 0x1b, 0x12 },	// JI
	{ 0x1c, 0x13 },	// JJ
	{ 0x17, 0x12 },	// JK
	{ 0x17, 0x15 },	// JL
	{ 0x1b, 0x12 },	// JM
	{ 0x1c, 0x13 },	// JN
	{ 0x1a, 0x13 },	// JO
	{ 0x1a, 0x17 },	// JP
	{ 0x1e, 0x14 },	// JQ
	{ 0x1f, 0x15 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A13[][MCA_PER_MCS] = {
	{ 0x13, 0x13 },	// J0
	{ 0x18, 0x0a },	// J1
	{ 0x1b, 0x15 },	// J2
	{ 0x19, 0x0b },	// J3
	{ 0x11, 0x11 },	// J4
	{ 0x15, 0x08 },	// J5
	{ 0x18, 0x13 },	// J6
	{ 0x17, 0x0a },	// J7
	{ 0x13, 0x13 },	// J8
	{ 0x18, 0x0a },	// J9
	{ 0x1b, 0x15 },	// JA
	{ 0x19, 0x0b },	// JB
	{ 0x11, 0x11 },	// JC
	{ 0x15, 0x08 },	// JD
	{ 0x18, 0x13 },	// JE
	{ 0x17, 0x0a },	// JF
	{ 0x13, 0x13 },	// JG
	{ 0x18, 0x0a },	// JH
	{ 0x1b, 0x15 },	// JI
	{ 0x19, 0x0b },	// JJ
	{ 0x13, 0x12 },	// JK
	{ 0x18, 0x0a },	// JL
	{ 0x1b, 0x15 },	// JM
	{ 0x19, 0x0b },	// JN
	{ 0x16, 0x14 },	// JO
	{ 0x1b, 0x0b },	// JP
	{ 0x1e, 0x17 },	// JQ
	{ 0x1c, 0x0d },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN1[][MCA_PER_MCS] = {
	{ 0x1d, 0x19 },	// J0
	{ 0x18, 0x1b },	// J1
	{ 0x23, 0x17 },	// J2
	{ 0x1a, 0x19 },	// J3
	{ 0x16, 0x14 },	// J4
	{ 0x12, 0x15 },	// J5
	{ 0x1d, 0x11 },	// J6
	{ 0x15, 0x13 },	// J7
	{ 0x1d, 0x19 },	// J8
	{ 0x18, 0x1b },	// J9
	{ 0x23, 0x17 },	// JA
	{ 0x1a, 0x19 },	// JB
	{ 0x16, 0x14 },	// JC
	{ 0x12, 0x15 },	// JD
	{ 0x1d, 0x11 },	// JE
	{ 0x15, 0x13 },	// JF
	{ 0x1d, 0x19 },	// JG
	{ 0x18, 0x1b },	// JH
	{ 0x23, 0x17 },	// JI
	{ 0x1a, 0x19 },	// JJ
	{ 0x1a, 0x15 },	// JK
	{ 0x15, 0x18 },	// JL
	{ 0x20, 0x14 },	// JM
	{ 0x17, 0x17 },	// JN
	{ 0x21, 0x1c },	// JO
	{ 0x1d, 0x1f },	// JP
	{ 0x28, 0x1a },	// JQ
	{ 0x1e, 0x1e },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKN[][MCA_PER_MCS] = {
	{ 0x65, 0x61 },	// J0
	{ 0x64, 0x5a },	// J1
	{ 0x66, 0x5b },	// J2
	{ 0x62, 0x59 },	// J3
	{ 0x5e, 0x5a },	// J4
	{ 0x5d, 0x54 },	// J5
	{ 0x5e, 0x54 },	// J6
	{ 0x5b, 0x52 },	// J7
	{ 0x65, 0x61 },	// J8
	{ 0x64, 0x5a },	// J9
	{ 0x66, 0x5b },	// JA
	{ 0x62, 0x59 },	// JB
	{ 0x5e, 0x5a },	// JC
	{ 0x5d, 0x54 },	// JD
	{ 0x5e, 0x54 },	// JE
	{ 0x5b, 0x52 },	// JF
	{ 0x65, 0x61 },	// JG
	{ 0x64, 0x5a },	// JH
	{ 0x66, 0x5b },	// JI
	{ 0x62, 0x59 },	// JJ
	{ 0x63, 0x5d },	// JK
	{ 0x61, 0x57 },	// JL
	{ 0x63, 0x58 },	// JM
	{ 0x5f, 0x56 },	// JN
	{ 0x6c, 0x66 },	// JO
	{ 0x6a, 0x60 },	// JP
	{ 0x6c, 0x5f },	// JQ
	{ 0x66, 0x5f },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKP[][MCA_PER_MCS] = {
	{ 0x65, 0x61 },	// J0
	{ 0x64, 0x5a },	// J1
	{ 0x66, 0x5b },	// J2
	{ 0x62, 0x59 },	// J3
	{ 0x5e, 0x5a },	// J4
	{ 0x5d, 0x54 },	// J5
	{ 0x5e, 0x54 },	// J6
	{ 0x5b, 0x52 },	// J7
	{ 0x65, 0x61 },	// J8
	{ 0x64, 0x5a },	// J9
	{ 0x66, 0x5b },	// JA
	{ 0x62, 0x59 },	// JB
	{ 0x5e, 0x5a },	// JC
	{ 0x5d, 0x54 },	// JD
	{ 0x5e, 0x54 },	// JE
	{ 0x5b, 0x52 },	// JF
	{ 0x65, 0x61 },	// JG
	{ 0x64, 0x5a },	// JH
	{ 0x66, 0x5b },	// JI
	{ 0x62, 0x59 },	// JJ
	{ 0x63, 0x5d },	// JK
	{ 0x61, 0x57 },	// JL
	{ 0x63, 0x58 },	// JM
	{ 0x5f, 0x56 },	// JN
	{ 0x6c, 0x66 },	// JO
	{ 0x6a, 0x60 },	// JP
	{ 0x6c, 0x5f },	// JQ
	{ 0x66, 0x5f },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A17[][MCA_PER_MCS] = {
	{ 0x18, 0x12 },	// J0
	{ 0x17, 0x14 },	// J1
	{ 0x17, 0x14 },	// J2
	{ 0x1a, 0x13 },	// J3
	{ 0x15, 0x10 },	// J4
	{ 0x15, 0x11 },	// J5
	{ 0x15, 0x11 },	// J6
	{ 0x18, 0x11 },	// J7
	{ 0x18, 0x12 },	// J8
	{ 0x17, 0x14 },	// J9
	{ 0x17, 0x14 },	// JA
	{ 0x1a, 0x13 },	// JB
	{ 0x15, 0x10 },	// JC
	{ 0x15, 0x11 },	// JD
	{ 0x15, 0x11 },	// JE
	{ 0x18, 0x11 },	// JF
	{ 0x18, 0x12 },	// JG
	{ 0x17, 0x14 },	// JH
	{ 0x17, 0x14 },	// JI
	{ 0x1a, 0x13 },	// JJ
	{ 0x18, 0x12 },	// JK
	{ 0x18, 0x14 },	// JL
	{ 0x18, 0x14 },	// JM
	{ 0x1a, 0x13 },	// JN
	{ 0x1b, 0x13 },	// JO
	{ 0x1b, 0x16 },	// JP
	{ 0x1a, 0x15 },	// JQ
	{ 0x1d, 0x15 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C1[][MCA_PER_MCS] = {
	{ 0x15, 0x11 },	// J0
	{ 0x17, 0x13 },	// J1
	{ 0x17, 0x13 },	// J2
	{ 0x17, 0x10 },	// J3
	{ 0x13, 0x10 },	// J4
	{ 0x15, 0x11 },	// J5
	{ 0x15, 0x10 },	// J6
	{ 0x15, 0x0e },	// J7
	{ 0x15, 0x11 },	// J8
	{ 0x17, 0x13 },	// J9
	{ 0x17, 0x13 },	// JA
	{ 0x17, 0x10 },	// JB
	{ 0x13, 0x10 },	// JC
	{ 0x15, 0x11 },	// JD
	{ 0x15, 0x10 },	// JE
	{ 0x15, 0x0e },	// JF
	{ 0x15, 0x11 },	// JG
	{ 0x17, 0x13 },	// JH
	{ 0x17, 0x13 },	// JI
	{ 0x17, 0x10 },	// JJ
	{ 0x16, 0x11 },	// JK
	{ 0x18, 0x13 },	// JL
	{ 0x18, 0x13 },	// JM
	{ 0x17, 0x10 },	// JN
	{ 0x19, 0x12 },	// JO
	{ 0x1b, 0x15 },	// JP
	{ 0x1a, 0x14 },	// JQ
	{ 0x1a, 0x13 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKN[][MCA_PER_MCS] = {
	{ 0x65, 0x61 },	// J0
	{ 0x64, 0x5a },	// J1
	{ 0x66, 0x5b },	// J2
	{ 0x62, 0x59 },	// J3
	{ 0x57, 0x5a },	// J4
	{ 0x57, 0x53 },	// J5
	{ 0x5a, 0x55 },	// J6
	{ 0x5b, 0x55 },	// J7
	{ 0x65, 0x61 },	// J8
	{ 0x64, 0x5a },	// J9
	{ 0x66, 0x5b },	// JA
	{ 0x62, 0x59 },	// JB
	{ 0x57, 0x5a },	// JC
	{ 0x57, 0x53 },	// JD
	{ 0x5a, 0x55 },	// JE
	{ 0x5b, 0x55 },	// JF
	{ 0x65, 0x61 },	// JG
	{ 0x64, 0x5a },	// JH
	{ 0x66, 0x5b },	// JI
	{ 0x62, 0x59 },	// JJ
	{ 0x59, 0x5c },	// JK
	{ 0x58, 0x55 },	// JL
	{ 0x5c, 0x57 },	// JM
	{ 0x5d, 0x57 },	// JN
	{ 0x6c, 0x66 },	// JO
	{ 0x6a, 0x60 },	// JP
	{ 0x6c, 0x5f },	// JQ
	{ 0x66, 0x5f },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKP[][MCA_PER_MCS] = {
	{ 0x65, 0x61 },	// J0
	{ 0x64, 0x5a },	// J1
	{ 0x66, 0x5b },	// J2
	{ 0x62, 0x59 },	// J3
	{ 0x57, 0x5a },	// J4
	{ 0x57, 0x53 },	// J5
	{ 0x5a, 0x55 },	// J6
	{ 0x5b, 0x55 },	// J7
	{ 0x65, 0x61 },	// J8
	{ 0x64, 0x5a },	// J9
	{ 0x66, 0x5b },	// JA
	{ 0x62, 0x59 },	// JB
	{ 0x57, 0x5a },	// JC
	{ 0x57, 0x53 },	// JD
	{ 0x5a, 0x55 },	// JE
	{ 0x5b, 0x55 },	// JF
	{ 0x65, 0x61 },	// JG
	{ 0x64, 0x5a },	// JH
	{ 0x66, 0x5b },	// JI
	{ 0x62, 0x59 },	// JJ
	{ 0x59, 0x5c },	// JK
	{ 0x58, 0x55 },	// JL
	{ 0x5c, 0x57 },	// JM
	{ 0x5d, 0x57 },	// JN
	{ 0x6c, 0x66 },	// JO
	{ 0x6a, 0x60 },	// JP
	{ 0x6c, 0x5f },	// JQ
	{ 0x66, 0x5f },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C2[][MCA_PER_MCS] = {
	{ 0x17, 0x1a },	// J0
	{ 0x18, 0x13 },	// J1
	{ 0x19, 0x14 },	// J2
	{ 0x17, 0x13 },	// J3
	{ 0x15, 0x17 },	// J4
	{ 0x16, 0x11 },	// J5
	{ 0x16, 0x11 },	// J6
	{ 0x15, 0x11 },	// J7
	{ 0x17, 0x1a },	// J8
	{ 0x18, 0x13 },	// J9
	{ 0x19, 0x14 },	// JA
	{ 0x17, 0x13 },	// JB
	{ 0x15, 0x17 },	// JC
	{ 0x16, 0x11 },	// JD
	{ 0x16, 0x11 },	// JE
	{ 0x15, 0x11 },	// JF
	{ 0x17, 0x1a },	// JG
	{ 0x18, 0x13 },	// JH
	{ 0x19, 0x14 },	// JI
	{ 0x17, 0x13 },	// JJ
	{ 0x17, 0x19 },	// JK
	{ 0x19, 0x13 },	// JL
	{ 0x19, 0x14 },	// JM
	{ 0x18, 0x13 },	// JN
	{ 0x1a, 0x1b },	// JO
	{ 0x1c, 0x16 },	// JP
	{ 0x1c, 0x16 },	// JQ
	{ 0x1a, 0x15 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN1[][MCA_PER_MCS] = {
	{ 0x1d, 0x19 },	// J0
	{ 0x18, 0x1b },	// J1
	{ 0x23, 0x17 },	// J2
	{ 0x1a, 0x19 },	// J3
	{ 0x14, 0x13 },	// J4
	{ 0x14, 0x15 },	// J5
	{ 0x19, 0x17 },	// J6
	{ 0x13, 0x16 },	// J7
	{ 0x1d, 0x19 },	// J8
	{ 0x18, 0x1b },	// J9
	{ 0x23, 0x17 },	// JA
	{ 0x1a, 0x19 },	// JB
	{ 0x14, 0x13 },	// JC
	{ 0x14, 0x15 },	// JD
	{ 0x19, 0x17 },	// JE
	{ 0x13, 0x16 },	// JF
	{ 0x1d, 0x19 },	// JG
	{ 0x18, 0x1b },	// JH
	{ 0x23, 0x17 },	// JI
	{ 0x1a, 0x19 },	// JJ
	{ 0x16, 0x14 },	// JK
	{ 0x16, 0x17 },	// JL
	{ 0x1b, 0x19 },	// JM
	{ 0x15, 0x18 },	// JN
	{ 0x21, 0x1c },	// JO
	{ 0x1d, 0x1f },	// JP
	{ 0x28, 0x1a },	// JQ
	{ 0x1e, 0x1e },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A02[][MCA_PER_MCS] = {
	{ 0x19, 0x14 },	// J0
	{ 0x19, 0x13 },	// J1
	{ 0x1b, 0x12 },	// J2
	{ 0x1a, 0x13 },	// J3
	{ 0x17, 0x12 },	// J4
	{ 0x16, 0x11 },	// J5
	{ 0x18, 0x10 },	// J6
	{ 0x18, 0x10 },	// J7
	{ 0x19, 0x14 },	// J8
	{ 0x19, 0x13 },	// J9
	{ 0x1b, 0x12 },	// JA
	{ 0x1a, 0x13 },	// JB
	{ 0x17, 0x12 },	// JC
	{ 0x16, 0x11 },	// JD
	{ 0x18, 0x10 },	// JE
	{ 0x18, 0x10 },	// JF
	{ 0x19, 0x14 },	// JG
	{ 0x19, 0x13 },	// JH
	{ 0x1b, 0x12 },	// JI
	{ 0x1a, 0x13 },	// JJ
	{ 0x1a, 0x13 },	// JK
	{ 0x19, 0x13 },	// JL
	{ 0x1b, 0x12 },	// JM
	{ 0x1b, 0x13 },	// JN
	{ 0x1d, 0x15 },	// JO
	{ 0x1c, 0x15 },	// JP
	{ 0x1e, 0x14 },	// JQ
	{ 0x1d, 0x15 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_PAR[][MCA_PER_MCS] = {
	{ 0x15, 0x16 },	// J0
	{ 0x15, 0x0d },	// J1
	{ 0x17, 0x12 },	// J2
	{ 0x18, 0x12 },	// J3
	{ 0x13, 0x14 },	// J4
	{ 0x13, 0x0c },	// J5
	{ 0x15, 0x10 },	// J6
	{ 0x16, 0x10 },	// J7
	{ 0x15, 0x16 },	// J8
	{ 0x15, 0x0d },	// J9
	{ 0x17, 0x12 },	// JA
	{ 0x18, 0x12 },	// JB
	{ 0x13, 0x14 },	// JC
	{ 0x13, 0x0c },	// JD
	{ 0x15, 0x10 },	// JE
	{ 0x16, 0x10 },	// JF
	{ 0x15, 0x16 },	// JG
	{ 0x15, 0x0d },	// JH
	{ 0x17, 0x12 },	// JI
	{ 0x18, 0x12 },	// JJ
	{ 0x15, 0x15 },	// JK
	{ 0x15, 0x0d },	// JL
	{ 0x17, 0x12 },	// JM
	{ 0x18, 0x12 },	// JN
	{ 0x18, 0x17 },	// JO
	{ 0x18, 0x0f },	// JP
	{ 0x1a, 0x13 },	// JQ
	{ 0x1b, 0x14 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN0[][MCA_PER_MCS] = {
	{ 0x1b, 0x1d },	// J0
	{ 0x24, 0x15 },	// J1
	{ 0x22, 0x18 },	// J2
	{ 0x1f, 0x14 },	// J3
	{ 0x14, 0x1c },	// J4
	{ 0x17, 0x13 },	// J5
	{ 0x19, 0x15 },	// J6
	{ 0x13, 0x14 },	// J7
	{ 0x1b, 0x1d },	// J8
	{ 0x24, 0x15 },	// J9
	{ 0x22, 0x18 },	// JA
	{ 0x1f, 0x14 },	// JB
	{ 0x14, 0x1c },	// JC
	{ 0x17, 0x13 },	// JD
	{ 0x19, 0x15 },	// JE
	{ 0x13, 0x14 },	// JF
	{ 0x1b, 0x1d },	// JG
	{ 0x24, 0x15 },	// JH
	{ 0x22, 0x18 },	// JI
	{ 0x1f, 0x14 },	// JJ
	{ 0x16, 0x1d },	// JK
	{ 0x19, 0x15 },	// JL
	{ 0x1b, 0x17 },	// JM
	{ 0x14, 0x16 },	// JN
	{ 0x1f, 0x20 },	// JO
	{ 0x2a, 0x19 },	// JP
	{ 0x26, 0x1a },	// JQ
	{ 0x23, 0x18 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_RASN_A16[][MCA_PER_MCS] = {
	{ 0x17, 0x14 },	// J0
	{ 0x13, 0x12 },	// J1
	{ 0x17, 0x14 },	// J2
	{ 0x1a, 0x13 },	// J3
	{ 0x14, 0x12 },	// J4
	{ 0x11, 0x10 },	// J5
	{ 0x15, 0x11 },	// J6
	{ 0x18, 0x11 },	// J7
	{ 0x17, 0x14 },	// J8
	{ 0x13, 0x12 },	// J9
	{ 0x17, 0x14 },	// JA
	{ 0x1a, 0x13 },	// JB
	{ 0x14, 0x12 },	// JC
	{ 0x11, 0x10 },	// JD
	{ 0x15, 0x11 },	// JE
	{ 0x18, 0x11 },	// JF
	{ 0x17, 0x14 },	// JG
	{ 0x13, 0x12 },	// JH
	{ 0x17, 0x14 },	// JI
	{ 0x1a, 0x13 },	// JJ
	{ 0x17, 0x14 },	// JK
	{ 0x14, 0x12 },	// JL
	{ 0x17, 0x13 },	// JM
	{ 0x1a, 0x13 },	// JN
	{ 0x1a, 0x15 },	// JO
	{ 0x16, 0x14 },	// JP
	{ 0x1a, 0x15 },	// JQ
	{ 0x1d, 0x16 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A08[][MCA_PER_MCS] = {
	{ 0x14, 0x14 },	// J0
	{ 0x1a, 0x10 },	// J1
	{ 0x19, 0x0f },	// J2
	{ 0x1c, 0x13 },	// J3
	{ 0x12, 0x12 },	// J4
	{ 0x17, 0x0e },	// J5
	{ 0x16, 0x0d },	// J6
	{ 0x19, 0x11 },	// J7
	{ 0x14, 0x14 },	// J8
	{ 0x1a, 0x10 },	// J9
	{ 0x19, 0x0f },	// JA
	{ 0x1c, 0x13 },	// JB
	{ 0x12, 0x12 },	// JC
	{ 0x17, 0x0e },	// JD
	{ 0x16, 0x0d },	// JE
	{ 0x19, 0x11 },	// JF
	{ 0x14, 0x14 },	// JG
	{ 0x1a, 0x10 },	// JH
	{ 0x19, 0x0f },	// JI
	{ 0x1c, 0x13 },	// JJ
	{ 0x14, 0x13 },	// JK
	{ 0x1b, 0x10 },	// JL
	{ 0x19, 0x0e },	// JM
	{ 0x1c, 0x13 },	// JN
	{ 0x17, 0x15 },	// JO
	{ 0x1e, 0x12 },	// JP
	{ 0x1c, 0x10 },	// JQ
	{ 0x1f, 0x15 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A05[][MCA_PER_MCS] = {
	{ 0x18, 0x12 },	// J0
	{ 0x17, 0x0f },	// J1
	{ 0x17, 0x12 },	// J2
	{ 0x1b, 0x10 },	// J3
	{ 0x15, 0x10 },	// J4
	{ 0x15, 0x0d },	// J5
	{ 0x15, 0x10 },	// J6
	{ 0x19, 0x0f },	// J7
	{ 0x18, 0x12 },	// J8
	{ 0x17, 0x0f },	// J9
	{ 0x17, 0x12 },	// JA
	{ 0x1b, 0x10 },	// JB
	{ 0x15, 0x10 },	// JC
	{ 0x15, 0x0d },	// JD
	{ 0x15, 0x10 },	// JE
	{ 0x19, 0x0f },	// JF
	{ 0x18, 0x12 },	// JG
	{ 0x17, 0x0f },	// JH
	{ 0x17, 0x12 },	// JI
	{ 0x1b, 0x10 },	// JJ
	{ 0x18, 0x11 },	// JK
	{ 0x18, 0x0f },	// JL
	{ 0x18, 0x12 },	// JM
	{ 0x1c, 0x10 },	// JN
	{ 0x1b, 0x13 },	// JO
	{ 0x1b, 0x11 },	// JP
	{ 0x1a, 0x14 },	// JQ
	{ 0x1e, 0x13 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A03[][MCA_PER_MCS] = {
	{ 0x18, 0x19 },	// J0
	{ 0x19, 0x13 },	// J1
	{ 0x17, 0x12 },	// J2
	{ 0x1a, 0x12 },	// J3
	{ 0x15, 0x17 },	// J4
	{ 0x16, 0x11 },	// J5
	{ 0x15, 0x10 },	// J6
	{ 0x18, 0x10 },	// J7
	{ 0x18, 0x19 },	// J8
	{ 0x19, 0x13 },	// J9
	{ 0x17, 0x12 },	// JA
	{ 0x1a, 0x12 },	// JB
	{ 0x15, 0x17 },	// JC
	{ 0x16, 0x11 },	// JD
	{ 0x15, 0x10 },	// JE
	{ 0x18, 0x10 },	// JF
	{ 0x18, 0x19 },	// JG
	{ 0x19, 0x13 },	// JH
	{ 0x17, 0x12 },	// JI
	{ 0x1a, 0x12 },	// JJ
	{ 0x18, 0x19 },	// JK
	{ 0x19, 0x13 },	// JL
	{ 0x17, 0x12 },	// JM
	{ 0x1a, 0x12 },	// JN
	{ 0x1b, 0x1b },	// JO
	{ 0x1c, 0x15 },	// JP
	{ 0x1a, 0x13 },	// JQ
	{ 0x1d, 0x15 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A01[][MCA_PER_MCS] = {
	{ 0x17, 0x15 },	// J0
	{ 0x18, 0x13 },	// J1
	{ 0x1a, 0x12 },	// J2
	{ 0x18, 0x13 },	// J3
	{ 0x15, 0x13 },	// J4
	{ 0x16, 0x11 },	// J5
	{ 0x17, 0x10 },	// J6
	{ 0x16, 0x11 },	// J7
	{ 0x17, 0x15 },	// J8
	{ 0x18, 0x13 },	// J9
	{ 0x1a, 0x12 },	// JA
	{ 0x18, 0x13 },	// JB
	{ 0x15, 0x13 },	// JC
	{ 0x16, 0x11 },	// JD
	{ 0x17, 0x10 },	// JE
	{ 0x16, 0x11 },	// JF
	{ 0x17, 0x15 },	// JG
	{ 0x18, 0x13 },	// JH
	{ 0x1a, 0x12 },	// JI
	{ 0x18, 0x13 },	// JJ
	{ 0x18, 0x14 },	// JK
	{ 0x19, 0x13 },	// JL
	{ 0x1a, 0x12 },	// JM
	{ 0x19, 0x13 },	// JN
	{ 0x1b, 0x16 },	// JO
	{ 0x1c, 0x15 },	// JP
	{ 0x1d, 0x14 },	// JQ
	{ 0x1b, 0x16 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A04[][MCA_PER_MCS] = {
	{ 0x13, 0x11 },	// J0
	{ 0x19, 0x11 },	// J1
	{ 0x17, 0x0e },	// J2
	{ 0x1a, 0x11 },	// J3
	{ 0x11, 0x10 },	// J4
	{ 0x16, 0x0f },	// J5
	{ 0x15, 0x0c },	// J6
	{ 0x18, 0x0f },	// J7
	{ 0x13, 0x11 },	// J8
	{ 0x19, 0x11 },	// J9
	{ 0x17, 0x0e },	// JA
	{ 0x1a, 0x11 },	// JB
	{ 0x11, 0x10 },	// JC
	{ 0x16, 0x0f },	// JD
	{ 0x15, 0x0c },	// JE
	{ 0x18, 0x0f },	// JF
	{ 0x13, 0x11 },	// JG
	{ 0x19, 0x11 },	// JH
	{ 0x17, 0x0e },	// JI
	{ 0x1a, 0x11 },	// JJ
	{ 0x13, 0x11 },	// JK
	{ 0x19, 0x11 },	// JL
	{ 0x17, 0x0e },	// JM
	{ 0x1a, 0x11 },	// JN
	{ 0x15, 0x12 },	// JO
	{ 0x1c, 0x13 },	// JP
	{ 0x1a, 0x0f },	// JQ
	{ 0x1d, 0x13 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A07[][MCA_PER_MCS] = {
	{ 0x13, 0x13 },	// J0
	{ 0x19, 0x13 },	// J1
	{ 0x18, 0x11 },	// J2
	{ 0x1b, 0x13 },	// J3
	{ 0x11, 0x12 },	// J4
	{ 0x16, 0x11 },	// J5
	{ 0x16, 0x0f },	// J6
	{ 0x18, 0x11 },	// J7
	{ 0x13, 0x13 },	// J8
	{ 0x19, 0x13 },	// J9
	{ 0x18, 0x11 },	// JA
	{ 0x1b, 0x13 },	// JB
	{ 0x11, 0x12 },	// JC
	{ 0x16, 0x11 },	// JD
	{ 0x16, 0x0f },	// JE
	{ 0x18, 0x11 },	// JF
	{ 0x13, 0x13 },	// JG
	{ 0x19, 0x13 },	// JH
	{ 0x18, 0x11 },	// JI
	{ 0x1b, 0x13 },	// JJ
	{ 0x13, 0x13 },	// JK
	{ 0x19, 0x13 },	// JL
	{ 0x19, 0x11 },	// JM
	{ 0x1b, 0x13 },	// JN
	{ 0x16, 0x14 },	// JO
	{ 0x1c, 0x15 },	// JP
	{ 0x1b, 0x13 },	// JQ
	{ 0x1d, 0x16 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A09[][MCA_PER_MCS] = {
	{ 0x14, 0x19 },	// J0
	{ 0x17, 0x13 },	// J1
	{ 0x17, 0x12 },	// J2
	{ 0x1a, 0x13 },	// J3
	{ 0x11, 0x17 },	// J4
	{ 0x15, 0x11 },	// J5
	{ 0x15, 0x10 },	// J6
	{ 0x17, 0x11 },	// J7
	{ 0x14, 0x19 },	// J8
	{ 0x17, 0x13 },	// J9
	{ 0x17, 0x12 },	// JA
	{ 0x1a, 0x13 },	// JB
	{ 0x11, 0x17 },	// JC
	{ 0x15, 0x11 },	// JD
	{ 0x15, 0x10 },	// JE
	{ 0x17, 0x11 },	// JF
	{ 0x14, 0x19 },	// JG
	{ 0x17, 0x13 },	// JH
	{ 0x17, 0x12 },	// JI
	{ 0x1a, 0x13 },	// JJ
	{ 0x14, 0x18 },	// JK
	{ 0x17, 0x13 },	// JL
	{ 0x17, 0x12 },	// JM
	{ 0x1a, 0x13 },	// JN
	{ 0x16, 0x1a },	// JO
	{ 0x1a, 0x15 },	// JP
	{ 0x1a, 0x14 },	// JQ
	{ 0x1c, 0x16 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A06[][MCA_PER_MCS] = {
	{ 0x14, 0x16 },	// J0
	{ 0x19, 0x13 },	// J1
	{ 0x16, 0x12 },	// J2
	{ 0x19, 0x13 },	// J3
	{ 0x12, 0x14 },	// J4
	{ 0x16, 0x11 },	// J5
	{ 0x14, 0x10 },	// J6
	{ 0x17, 0x11 },	// J7
	{ 0x14, 0x16 },	// J8
	{ 0x19, 0x13 },	// J9
	{ 0x16, 0x12 },	// JA
	{ 0x19, 0x13 },	// JB
	{ 0x12, 0x14 },	// JC
	{ 0x16, 0x11 },	// JD
	{ 0x14, 0x10 },	// JE
	{ 0x17, 0x11 },	// JF
	{ 0x14, 0x16 },	// JG
	{ 0x19, 0x13 },	// JH
	{ 0x16, 0x12 },	// JI
	{ 0x19, 0x13 },	// JJ
	{ 0x15, 0x15 },	// JK
	{ 0x19, 0x13 },	// JL
	{ 0x16, 0x12 },	// JM
	{ 0x19, 0x13 },	// JN
	{ 0x17, 0x17 },	// JO
	{ 0x1d, 0x15 },	// JP
	{ 0x19, 0x14 },	// JQ
	{ 0x1c, 0x16 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE1[][MCA_PER_MCS] = {
	{ 0x17, 0x14 },	// J0
	{ 0x1d, 0x0e },	// J1
	{ 0x1a, 0x0d },	// J2
	{ 0x1b, 0x0d },	// J3
	{ 0x12, 0x0f },	// J4
	{ 0x16, 0x0a },	// J5
	{ 0x14, 0x08 },	// J6
	{ 0x16, 0x08 },	// J7
	{ 0x17, 0x14 },	// J8
	{ 0x1d, 0x0e },	// J9
	{ 0x1a, 0x0d },	// JA
	{ 0x1b, 0x0d },	// JB
	{ 0x12, 0x0f },	// JC
	{ 0x16, 0x0a },	// JD
	{ 0x14, 0x08 },	// JE
	{ 0x16, 0x08 },	// JF
	{ 0x17, 0x14 },	// JG
	{ 0x1d, 0x0e },	// JH
	{ 0x1a, 0x0d },	// JI
	{ 0x1b, 0x0d },	// JJ
	{ 0x15, 0x10 },	// JK
	{ 0x19, 0x0b },	// JL
	{ 0x17, 0x0a },	// JM
	{ 0x18, 0x0a },	// JN
	{ 0x1b, 0x16 },	// JO
	{ 0x21, 0x11 },	// JP
	{ 0x1e, 0x0f },	// JQ
	{ 0x1f, 0x11 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A12[][MCA_PER_MCS] = {
	{ 0x15, 0x19 },	// J0
	{ 0x17, 0x0f },	// J1
	{ 0x17, 0x12 },	// J2
	{ 0x18, 0x12 },	// J3
	{ 0x13, 0x17 },	// J4
	{ 0x15, 0x0d },	// J5
	{ 0x15, 0x10 },	// J6
	{ 0x16, 0x10 },	// J7
	{ 0x15, 0x19 },	// J8
	{ 0x17, 0x0f },	// J9
	{ 0x17, 0x12 },	// JA
	{ 0x18, 0x12 },	// JB
	{ 0x13, 0x17 },	// JC
	{ 0x15, 0x0d },	// JD
	{ 0x15, 0x10 },	// JE
	{ 0x16, 0x10 },	// JF
	{ 0x15, 0x19 },	// JG
	{ 0x17, 0x0f },	// JH
	{ 0x17, 0x12 },	// JI
	{ 0x18, 0x12 },	// JJ
	{ 0x16, 0x19 },	// JK
	{ 0x17, 0x0f },	// JL
	{ 0x18, 0x12 },	// JM
	{ 0x18, 0x12 },	// JN
	{ 0x18, 0x1b },	// JO
	{ 0x1a, 0x11 },	// JP
	{ 0x1a, 0x13 },	// JQ
	{ 0x1a, 0x14 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ACTN[][MCA_PER_MCS] = {
	{ 0x16, 0x19 },	// J0
	{ 0x17, 0x12 },	// J1
	{ 0x18, 0x14 },	// J2
	{ 0x1a, 0x10 },	// J3
	{ 0x13, 0x17 },	// J4
	{ 0x14, 0x10 },	// J5
	{ 0x15, 0x11 },	// J6
	{ 0x18, 0x0e },	// J7
	{ 0x16, 0x19 },	// J8
	{ 0x17, 0x12 },	// J9
	{ 0x18, 0x14 },	// JA
	{ 0x1a, 0x10 },	// JB
	{ 0x13, 0x17 },	// JC
	{ 0x14, 0x10 },	// JD
	{ 0x15, 0x11 },	// JE
	{ 0x18, 0x0e },	// JF
	{ 0x16, 0x19 },	// JG
	{ 0x17, 0x12 },	// JH
	{ 0x18, 0x14 },	// JI
	{ 0x1a, 0x10 },	// JJ
	{ 0x16, 0x19 },	// JK
	{ 0x17, 0x12 },	// JL
	{ 0x18, 0x13 },	// JM
	{ 0x1b, 0x10 },	// JN
	{ 0x19, 0x1b },	// JO
	{ 0x1a, 0x14 },	// JP
	{ 0x1a, 0x15 },	// JQ
	{ 0x1d, 0x12 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A11[][MCA_PER_MCS] = {
	{ 0x11, 0x11 },	// J0
	{ 0x18, 0x13 },	// J1
	{ 0x17, 0x11 },	// J2
	{ 0x19, 0x13 },	// J3
	{ 0x0f, 0x10 },	// J4
	{ 0x15, 0x11 },	// J5
	{ 0x14, 0x0f },	// J6
	{ 0x17, 0x11 },	// J7
	{ 0x11, 0x11 },	// J8
	{ 0x18, 0x13 },	// J9
	{ 0x17, 0x11 },	// JA
	{ 0x19, 0x13 },	// JB
	{ 0x0f, 0x10 },	// JC
	{ 0x15, 0x11 },	// JD
	{ 0x14, 0x0f },	// JE
	{ 0x17, 0x11 },	// JF
	{ 0x11, 0x11 },	// JG
	{ 0x18, 0x13 },	// JH
	{ 0x17, 0x11 },	// JI
	{ 0x19, 0x13 },	// JJ
	{ 0x11, 0x11 },	// JK
	{ 0x18, 0x13 },	// JL
	{ 0x17, 0x11 },	// JM
	{ 0x19, 0x13 },	// JN
	{ 0x13, 0x12 },	// JO
	{ 0x1b, 0x16 },	// JP
	{ 0x19, 0x12 },	// JQ
	{ 0x1b, 0x16 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG0[][MCA_PER_MCS] = {
	{ 0x14, 0x12 },	// J0
	{ 0x1a, 0x11 },	// J1
	{ 0x17, 0x13 },	// J2
	{ 0x1a, 0x0f },	// J3
	{ 0x12, 0x10 },	// J4
	{ 0x17, 0x0f },	// J5
	{ 0x15, 0x11 },	// J6
	{ 0x17, 0x0e },	// J7
	{ 0x14, 0x12 },	// J8
	{ 0x1a, 0x11 },	// J9
	{ 0x17, 0x13 },	// JA
	{ 0x1a, 0x0f },	// JB
	{ 0x12, 0x10 },	// JC
	{ 0x17, 0x0f },	// JD
	{ 0x15, 0x11 },	// JE
	{ 0x17, 0x0e },	// JF
	{ 0x14, 0x12 },	// JG
	{ 0x1a, 0x11 },	// JH
	{ 0x17, 0x13 },	// JI
	{ 0x1a, 0x0f },	// JJ
	{ 0x14, 0x11 },	// JK
	{ 0x1a, 0x11 },	// JL
	{ 0x17, 0x13 },	// JM
	{ 0x1a, 0x0f },	// JN
	{ 0x17, 0x13 },	// JO
	{ 0x1d, 0x14 },	// JP
	{ 0x1a, 0x15 },	// JQ
	{ 0x1c, 0x11 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE0[][MCA_PER_MCS] = {
	{ 0x1c, 0x20 },	// J0
	{ 0x19, 0x14 },	// J1
	{ 0x21, 0x19 },	// J2
	{ 0x1c, 0x13 },	// J3
	{ 0x16, 0x1a },	// J4
	{ 0x14, 0x0f },	// J5
	{ 0x1a, 0x13 },	// J6
	{ 0x17, 0x0d },	// J7
	{ 0x1c, 0x20 },	// J8
	{ 0x19, 0x14 },	// J9
	{ 0x21, 0x19 },	// JA
	{ 0x1c, 0x13 },	// JB
	{ 0x16, 0x1a },	// JC
	{ 0x14, 0x0f },	// JD
	{ 0x1a, 0x13 },	// JE
	{ 0x17, 0x0d },	// JF
	{ 0x1c, 0x20 },	// JG
	{ 0x19, 0x14 },	// JH
	{ 0x21, 0x19 },	// JI
	{ 0x1c, 0x13 },	// JJ
	{ 0x19, 0x1c },	// JK
	{ 0x16, 0x11 },	// JL
	{ 0x1e, 0x16 },	// JM
	{ 0x19, 0x10 },	// JN
	{ 0x21, 0x23 },	// JO
	{ 0x1e, 0x18 },	// JP
	{ 0x25, 0x1c },	// JQ
	{ 0x20, 0x16 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE1[][MCA_PER_MCS] = {
	{ 0x17, 0x14 },	// J0
	{ 0x1d, 0x0e },	// J1
	{ 0x1a, 0x0d },	// J2
	{ 0x1b, 0x0d },	// J3
	{ 0x10, 0x17 },	// J4
	{ 0x18, 0x10 },	// J5
	{ 0x16, 0x10 },	// J6
	{ 0x1b, 0x11 },	// J7
	{ 0x17, 0x14 },	// J8
	{ 0x1d, 0x0e },	// J9
	{ 0x1a, 0x0d },	// JA
	{ 0x1b, 0x0d },	// JB
	{ 0x10, 0x17 },	// JC
	{ 0x18, 0x10 },	// JD
	{ 0x16, 0x10 },	// JE
	{ 0x1b, 0x11 },	// JF
	{ 0x17, 0x14 },	// JG
	{ 0x1d, 0x0e },	// JH
	{ 0x1a, 0x0d },	// JI
	{ 0x1b, 0x0d },	// JJ
	{ 0x12, 0x18 },	// JK
	{ 0x1a, 0x11 },	// JL
	{ 0x18, 0x12 },	// JM
	{ 0x1d, 0x12 },	// JN
	{ 0x1b, 0x16 },	// JO
	{ 0x21, 0x11 },	// JP
	{ 0x1e, 0x0f },	// JQ
	{ 0x1f, 0x11 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG1[][MCA_PER_MCS] = {
	{ 0x14, 0x13 },	// J0
	{ 0x18, 0x13 },	// J1
	{ 0x18, 0x11 },	// J2
	{ 0x1a, 0x11 },	// J3
	{ 0x11, 0x11 },	// J4
	{ 0x15, 0x11 },	// J5
	{ 0x15, 0x0f },	// J6
	{ 0x18, 0x0f },	// J7
	{ 0x14, 0x13 },	// J8
	{ 0x18, 0x13 },	// J9
	{ 0x18, 0x11 },	// JA
	{ 0x1a, 0x11 },	// JB
	{ 0x11, 0x11 },	// JC
	{ 0x15, 0x11 },	// JD
	{ 0x15, 0x0f },	// JE
	{ 0x18, 0x0f },	// JF
	{ 0x14, 0x13 },	// JG
	{ 0x18, 0x13 },	// JH
	{ 0x18, 0x11 },	// JI
	{ 0x1a, 0x11 },	// JJ
	{ 0x14, 0x13 },	// JK
	{ 0x18, 0x13 },	// JL
	{ 0x18, 0x11 },	// JM
	{ 0x1a, 0x11 },	// JN
	{ 0x16, 0x14 },	// JO
	{ 0x1b, 0x15 },	// JP
	{ 0x1a, 0x12 },	// JQ
	{ 0x1d, 0x14 },	// JR
};

static const uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE0[][MCA_PER_MCS] = {
	{ 0x1c, 0x20 },	// J0
	{ 0x19, 0x14 },	// J1
	{ 0x21, 0x19 },	// J2
	{ 0x1c, 0x13 },	// J3
	{ 0x12, 0x0f },	// J4
	{ 0x17, 0x12 },	// J5
	{ 0x17, 0x0a },	// J6
	{ 0x19, 0x13 },	// J7
	{ 0x1c, 0x20 },	// J8
	{ 0x19, 0x14 },	// J9
	{ 0x21, 0x19 },	// JA
	{ 0x1c, 0x13 },	// JB
	{ 0x12, 0x0f },	// JC
	{ 0x17, 0x12 },	// JD
	{ 0x17, 0x0a },	// JE
	{ 0x19, 0x13 },	// JF
	{ 0x1c, 0x20 },	// JG
	{ 0x19, 0x14 },	// JH
	{ 0x21, 0x19 },	// JI
	{ 0x1c, 0x13 },	// JJ
	{ 0x13, 0x0f },	// JK
	{ 0x19, 0x13 },	// JL
	{ 0x19, 0x0b },	// JM
	{ 0x1b, 0x15 },	// JN
	{ 0x21, 0x23 },	// JO
	{ 0x1e, 0x18 },	// JP
	{ 0x25, 0x1c },	// JQ
	{ 0x20, 0x16 },	// JR
};

static void reset_delay(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

	/* See comments in ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN0 for layout */
	int speed_idx = mem_data.speed == 1866 ? 0 :
	                mem_data.speed == 2133 ? 8 :
	                mem_data.speed == 2400 ? 16 : 24;
	int dimm_idx = (mca->dimm[0].present && mca->dimm[1].present) ? 4 : 0;
	/* TODO: second CPU not supported */
	int vpd_idx = speed_idx + dimm_idx + mcs_i;

	/*
	 * From documentation:
	 * "If the reset value is not sufficient for the given system, these
	 * registers must be set via the programming interface."
	 *
	 * Unsure if this is the case. Hostboot sets it, so lets do it too.
	 */
	/* IOM0.DDRPHY_ADR_DELAY0_P0_ADR0 =
	    [all]   0
	    [49-55] ADR_DELAY0 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN0
	    [57-63] ADR_DELAY1 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_WEN_A14
	*/
	mca_and_or(id, mca_i, 0x800040040701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN0[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_WEN_A14[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY1_P0_ADR0 =
	    [all]   0
	    [49-55] ADR_DELAY2 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT1
	    [57-63] ADR_DELAY3 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C0
	*/
	mca_and_or(id, mca_i, 0x800040050701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT1[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C0[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY2_P0_ADR0 =
	    [all]   0
	    [49-55] ADR_DELAY4 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA1
	    [57-63] ADR_DELAY5 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A10
	*/
	mca_and_or(id, mca_i, 0x800040060701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA1[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A10[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY3_P0_ADR0 =
	    [all]   0
	    [49-55] ADR_DELAY6 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT1
	    [57-63] ADR_DELAY7 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA0
	*/
	mca_and_or(id, mca_i, 0x800040070701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT1[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA0[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY4_P0_ADR0 =
	    [all]   0
	    [49-55] ADR_DELAY8 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A00
	    [57-63] ADR_DELAY9 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT0
	*/
	mca_and_or(id, mca_i, 0x800040080701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A00[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT0[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY5_P0_ADR0 =
	    [all]   0
	    [49-55] ADR_DELAY10 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT0
	    [57-63] ADR_DELAY11 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_CASN_A15
	*/
	mca_and_or(id, mca_i, 0x800040090701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT0[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_CASN_A15[vpd_idx][mca_i], 63));


	/* IOM0.DDRPHY_ADR_DELAY0_P0_ADR1 =
	    [all]   0
	    [49-55] ADR_DELAY0 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A13
	    [57-63] ADR_DELAY1 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN1
	*/
	mca_and_or(id, mca_i, 0x800044040701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A13[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN1[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY1_P0_ADR1 =
	    [all]   0
	    [49-55] ADR_DELAY2 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKN
	    [57-63] ADR_DELAY3 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKP
	*/
	mca_and_or(id, mca_i, 0x800044050701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKN[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKP[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY2_P0_ADR1 =
	    [all]   0
	    [49-55] ADR_DELAY4 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A17
	    [57-63] ADR_DELAY5 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C1
	*/
	mca_and_or(id, mca_i, 0x800044060701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A17[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C1[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY3_P0_ADR1 =
	    [all]   0
	    [49-55] ADR_DELAY6 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKN
	    [57-63] ADR_DELAY7 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKP
	*/
	mca_and_or(id, mca_i, 0x800044070701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKN[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKP[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY4_P0_ADR1 =
	    [all]   0
	    [49-55] ADR_DELAY8 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C2
	    [57-63] ADR_DELAY9 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN1
	*/
	mca_and_or(id, mca_i, 0x800044080701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C2[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN1[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY5_P0_ADR1 =
	    [all]   0
	    [49-55] ADR_DELAY10 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A02
	    [57-63] ADR_DELAY11 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_PAR
	*/
	mca_and_or(id, mca_i, 0x800044090701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A02[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_PAR[vpd_idx][mca_i], 63));


	/* IOM0.DDRPHY_ADR_DELAY0_P0_ADR2 =
	    [all]   0
	    [49-55] ADR_DELAY0 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN0
	    [57-63] ADR_DELAY1 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_RASN_A16
	*/
	mca_and_or(id, mca_i, 0x800048040701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN0[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_RASN_A16[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY1_P0_ADR2 =
	    [all]   0
	    [49-55] ADR_DELAY2 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A08
	    [57-63] ADR_DELAY3 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A05
	*/
	mca_and_or(id, mca_i, 0x800048050701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A08[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A05[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY2_P0_ADR2 =
	    [all]   0
	    [49-55] ADR_DELAY4 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A03
	    [57-63] ADR_DELAY5 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A01
	*/
	mca_and_or(id, mca_i, 0x800048060701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A03[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A01[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY3_P0_ADR2 =
	    [all]   0
	    [49-55] ADR_DELAY6 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A04
	    [57-63] ADR_DELAY7 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A07
	*/
	mca_and_or(id, mca_i, 0x800048070701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A04[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A07[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY4_P0_ADR2 =
	    [all]   0
	    [49-55] ADR_DELAY8 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A09
	    [57-63] ADR_DELAY9 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A06
	*/
	mca_and_or(id, mca_i, 0x800048080701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A09[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A06[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY5_P0_ADR2 =
	    [all]   0
	    [49-55] ADR_DELAY10 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE1
	    [57-63] ADR_DELAY11 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A12
	*/
	mca_and_or(id, mca_i, 0x800048090701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE1[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A12[vpd_idx][mca_i], 63));


	/* IOM0.DDRPHY_ADR_DELAY0_P0_ADR3 =
	    [all]   0
	    [49-55] ADR_DELAY0 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ACTN
	    [57-63] ADR_DELAY1 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A11
	*/
	mca_and_or(id, mca_i, 0x80004C040701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ACTN[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A11[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY1_P0_ADR3 =
	    [all]   0
	    [49-55] ADR_DELAY2 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG0
	    [57-63] ADR_DELAY3 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE0
	*/
	mca_and_or(id, mca_i, 0x80004C050701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG0[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE0[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY2_P0_ADR3 =
	    [all]   0
	    [49-55] ADR_DELAY4 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE1
	    [57-63] ADR_DELAY5 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG1
	*/
	mca_and_or(id, mca_i, 0x80004C060701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE1[vpd_idx][mca_i], 55) |
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG1[vpd_idx][mca_i], 63));

	/* IOM0.DDRPHY_ADR_DELAY3_P0_ADR3 =
	    [all]   0
	    [49-55] ADR_DELAY6 = ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE0
	*/
	mca_and_or(id, mca_i, 0x80004C070701103F, 0,
	           PPC_SHIFT(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE0[vpd_idx][mca_i], 55));

}

/* FIXME: these can be updated by MVPD in istep 7.5. Values below (from MEMD)
 * are different than in documentation. */
static const uint8_t ATTR_MSS_VPD_MR_TSYS_ADR[] = { 0x79, 0x7B, 0x7D, 0x7F };

static void reset_tsys_adr(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int i = mem_data.speed == 1866 ? 0 :
	        mem_data.speed == 2133 ? 1 :
	        mem_data.speed == 2400 ? 2 : 3;

	/* IOM0.DDRPHY_ADR_MCCLK_WRCLK_PR_STATIC_OFFSET_P0_ADR32S{0,1} =
	    [all]   0
	    [49-55] TSYS_WRCLK = ATTR_MSS_VPD_MR_TSYS_ADR
		  // From regs spec:
		  // Set to ‘19’h for 2666 MT/s.
		  // Set to ‘17’h for 2400 MT/s.
		  // Set to ‘14’h for 2133 MT/s.
		  // Set to ‘12’h for 1866 MT/s.
	*/
	/* Has the same stride as DP16. */
	dp_mca_and_or(id, 0, mca_i, 0x800080330701103F, 0,
	              PPC_SHIFT(ATTR_MSS_VPD_MR_TSYS_ADR[i], 55));
	dp_mca_and_or(id, 1, mca_i, 0x800080330701103F, 0,
	              PPC_SHIFT(ATTR_MSS_VPD_MR_TSYS_ADR[i], 55));
}

/* FIXME: these can be updated by MVPD in istep 7.5. Values below (from MEMD)
 * are different than in documentation. */
static const uint8_t ATTR_MSS_VPD_MR_TSYS_DATA[] = { 0x74, 0x77, 0x79, 0x7C };

static void reset_tsys_data(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int i = mem_data.speed == 1866 ? 0 :
	        mem_data.speed == 2133 ? 1 :
	        mem_data.speed == 2400 ? 2 : 3;
	int dp;

	/* IOM0.DDRPHY_DP16_WRCLK_PR_P0_{0,1,2,3,4} =
	    [all]   0
	    [49-55] TSYS_WRCLK = ATTR_MSS_VPD_MR_TSYS_DATA
		  // From regs spec:
		  // Set to ‘12’h for 2666 MT/s.
		  // Set to ‘10’h for 2400 MT/s.
		  // Set to ‘0F’h for 2133 MT/s.
		  // Set to ‘0D’h for 1866 MT/s.
	*/
	for (dp = 0; dp < 5; dp++) {
		dp_mca_and_or(id, dp, mca_i, 0x800000740701103F, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MR_TSYS_DATA[i], 55));
	}
}

static void reset_io_impedances(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_IO_TX_FET_SLICE_P0_{0,1,2,3,4} =
		    [all]   0
		    // 0 - Hi-Z, otherwise impedance = 240/<num of set bits> Ohms
		    [49-55] EN_SLICE_N_WR = ATTR_MSS_VPD_MT_MC_DRV_IMP_DQ_DQS[{0,1,2,3,4}]
		    [57-63] EN_SLICE_P_WR = ATTR_MSS_VPD_MT_MC_DRV_IMP_DQ_DQS[{0,1,2,3,4}]
		*/
		/*
		 * For all rank configurations and MCAs, ATTR_MSS_VPD_MT_MC_DRV_IMP_DQ_DQS
		 * is 34 Ohms. 240/34 = 7 bits set. According to documentation this is the
		 * default value, but set it just to be safe.
		 */
		dp_mca_and_or(id, dp, mca_i, 0x800000780701103F, 0,
		              PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63));

		/* IOM0.DDRPHY_DP16_IO_TX_PFET_TERM_P0_{0,1,2,3,4} =
		    [all]   0
		    // 0 - Hi-Z, otherwise impedance = 240/<num of set bits> Ohms
		    [49-55] EN_SLICE_N_WR = ATTR_MSS_VPD_MT_MC_RCV_IMP_DQ_DQS[{0,1,2,3,4}]
		*/
		/* 60 Ohms for all configurations, 240/60 = 4 bits set. */
		dp_mca_and_or(id, dp, mca_i, 0x8000007B0701103F, 0,
		              PPC_BITMASK(52, 55));
	}

	/* IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR1 =    // yes, ADR1
	    // These are RMW one at a time. I don't see why not all at once, or at least in pairs (P and N of the same clocks)
	    if (ATTR_MSS_VPD_MT_MC_DRV_IMP_CLK == ENUM_ATTR_MSS_VPD_MT_MC_DRV_IMP_CLK_OHM30):
	      [54,52,62,60] SLICE_SELn = 1    // CLK00 P, CLK00 N, CLK01 P, CLK01 N
	    else
	      [54,52,62,60] = 0
	*/
	/* 30 Ohms for all configurations. */
	mca_and_or(id, mca_i, 0x800044200701103F, ~0,
	           PPC_BIT(54) | PPC_BIT(52) | PPC_BIT(62) | PPC_BIT(60));

	/*
	 * Following are reordered to minimalize number of register reads/writes
	------------------------------------------------------------------------
	val = (ATTR_MSS_VPD_MT_MC_DRV_IMP_CMD_ADDR == ENUM_ATTR_MSS_VPD_MT_MC_DRV_IMP_CMD_ADDR_OHM30) ? 1 : 0
	// val = 30 for all VPD configurations
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR0 =
	    [50,56,58,62] =           val       // ADDR14/WEN, BA1, ADDR10, BA0
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR0 =
	    [48,54] =                 val       // ADDR0, ADDR15/CAS
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR1 =        // same as CLK, however it uses different VPD
	    [48,56] =                 val       // ADDR13, ADDR17/RAS
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR1 =
	    [52]    =                 val       // ADDR2
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR2 =
	    [50,52,54,56,58,60,62] =  val       // ADDR16/RAS, ADDR8, ADDR5, ADDR3, ADDR1, ADDR4, ADDR7
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR2 =
	    [48,50,54] =              val       // ADDR9, ADDR6, ADDR12
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR3 =
	    [48,50,52,58] =           val       // ACT_N, ADDR11, BG0, BG1
	*/
	mca_and_or(id, mca_i, 0x800040200701103F, ~0,
	           PPC_BIT(50) | PPC_BIT(56) | PPC_BIT(58) | PPC_BIT(62));
	mca_and_or(id, mca_i, 0x800040210701103F, ~0,
	           PPC_BIT(48) | PPC_BIT(54));
	mca_and_or(id, mca_i, 0x800044200701103F, ~0,
	           PPC_BIT(48) | PPC_BIT(56));
	mca_and_or(id, mca_i, 0x800044210701103F, ~0,
	           PPC_BIT(52));
	mca_and_or(id, mca_i, 0x800048200701103F, ~0,
	           PPC_BIT(50) | PPC_BIT(52) | PPC_BIT(54) | PPC_BIT(56) |
	           PPC_BIT(58) | PPC_BIT(60) | PPC_BIT(62));
	mca_and_or(id, mca_i, 0x800048210701103F, ~0,
	           PPC_BIT(48) | PPC_BIT(50) | PPC_BIT(54));
	mca_and_or(id, mca_i, 0x80004C200701103F, ~0,
	           PPC_BIT(48) | PPC_BIT(50) | PPC_BIT(52) | PPC_BIT(58));

	/*
	 * Following are reordered to minimalize number of register reads/writes
	------------------------------------------------------------------------
	val = (ATTR_MSS_VPD_MT_MC_DRV_IMP_CNTL == ENUM_ATTR_MSS_VPD_MT_MC_DRV_IMP_CNTL_OHM30) ? 1 : 0
	// val = 30 for all VPD sets
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR0 =        // same as CMD/ADDR, however it uses different VPD
	    [52,60] =                 val       // ODT3, ODT1
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR0 =        // same as CMD/ADDR, however it uses different VPD
	    [50,52] =                 val       // ODT2, ODT0
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR1 =        // same as CMD/ADDR, however it uses different VPD
	    [54] =                    val       // PARITY
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR2 =        // same as CMD/ADDR, however it uses different VPD
	    [52] =                    val       // CKE1
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR3 =        // same as CMD/ADDR, however it uses different VPD
	    [54,56,60,62] =           val       // CKE0, CKE3, CKE2, RESET_N
	*/
	mca_and_or(id, mca_i, 0x800040200701103F, ~0,
	           PPC_BIT(52) | PPC_BIT(60));
	mca_and_or(id, mca_i, 0x800040210701103F, ~0,
	           PPC_BIT(50) | PPC_BIT(52));
	mca_and_or(id, mca_i, 0x800044210701103F, ~0,
	           PPC_BIT(54));
	mca_and_or(id, mca_i, 0x800048210701103F, ~0,
	           PPC_BIT(52));
	mca_and_or(id, mca_i, 0x80004C200701103F, ~0,
	           PPC_BIT(54) | PPC_BIT(56) | PPC_BIT(60) | PPC_BIT(62));

	/*
	 * Following are reordered to minimalize number of register reads/writes
	------------------------------------------------------------------------
	val = (ATTR_MSS_VPD_MT_MC_DRV_IMP_CSCID == ENUM_ATTR_MSS_VPD_MT_MC_DRV_IMP_CSCID_OHM30) ? 1 : 0
	// val = 30 for all VPD sets
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR0 =        // same as CMD/ADDR and CNTL, however it uses different VPD
	    [48,54] =                 val       // CS0, CID0
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR1 =        // same as CLK and CMD/ADDR, however it uses different VPD
	    [50,58] =                 val       // CS1, CID1
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP1_P0_ADR1 =        // same as CMD/ADDR and CNTL, however it uses different VPD
	    [48,50] =                 val       // CS3, CID2
	IOM0.DDRPHY_ADR_IO_FET_SLICE_EN_MAP0_P0_ADR2 =        // same as CMD/ADDR, however it uses different VPD
	    [48] =                    val       // CS2
	*/
	mca_and_or(id, mca_i, 0x800040200701103F, ~0,
	           PPC_BIT(48) | PPC_BIT(54));
	mca_and_or(id, mca_i, 0x800044200701103F, ~0,
	           PPC_BIT(50) | PPC_BIT(58));
	mca_and_or(id, mca_i, 0x800044210701103F, ~0,
	           PPC_BIT(48) | PPC_BIT(50));
	mca_and_or(id, mca_i, 0x800048200701103F, ~0,
	           PPC_BIT(48));

	/*
	 * IO impedance regs summary:            lanes 9-15 have different possible settings (results in 15/30 vs 40/30 Ohm)
	 * MAP0_ADR0: all set                       MAP1_ADR0: lanes 12-15 not set
	 * MAP0_ADR1: all set                       MAP1_ADR1: lanes 12-15 not set
	 * MAP0_ADR2: all set                       MAP1_ADR2: lanes 12-15 not set
	 * MAP0_ADR3: all set                       MAP1_ADR3: not used
	 * This mapping is consistent with ADR_DELAYx_P0_ADRy settings.
	 */
}

static const uint8_t ATTR_MSS_VPD_MT_VREF_DRAM_WR[] = {0x0E, 0x18, 0x1E, 0x22};

static void reset_wr_vref_registers(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	int dp;
	int vpd_idx = mca->dimm[0].present ? (mca->dimm[0].mranks == 2 ? 2 : 0) :
	                                     (mca->dimm[1].mranks == 2 ? 2 : 0);
	if (mca->dimm[0].present && mca->dimm[1].present)
		vpd_idx++;

	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_WR_VREF_CONFIG0_P0_{0,1,2,3,4} =
		    [all]   0
		    [48]    WR_CTR_1D_MODE_SWITCH =       0     // 1 for <DD2
		    [49]    WR_CTR_RUN_FULL_1D =          1
		    [50-52] WR_CTR_2D_SMALL_STEP_VAL =    0     // implicit +1
		    [53-56] WR_CTR_2D_BIG_STEP_VAL =      1     // implicit +1
		    [57-59] WR_CTR_NUM_BITS_TO_SKIP =     0     // skip nothing
		    [60-62] WR_CTR_NUM_NO_INC_VREF_COMP = 7
		*/
		dp_mca_and_or(id, dp, mca_i, 0x8000006C0701103F, 0,
		              PPC_BIT(49) | PPC_SHIFT(1, 56) | PPC_SHIFT(7, 62));

		/* IOM0.DDRPHY_DP16_WR_VREF_CONFIG1_P0_{0,1,2,3,4} =
		    [all]   0
		    [48]    WR_CTR_VREF_RANGE_SELECT =      0       // range 1 by default (60-92.5%)
		    [49-55] WR_CTR_VREF_RANGE_CROSSOVER =   0x18    // JEDEC table 34
		    [56-62] WR_CTR_VREF_SINGLE_RANGE_MAX =  0x32    // JEDEC table 34
		*/
		dp_mca_and_or(id, dp, mca_i, 0x800000EC0701103F, 0,
		              PPC_SHIFT(0x18, 55) | PPC_SHIFT(0x32, 62));

		/* IOM0.DDRPHY_DP16_WR_VREF_STATUS0_P0_{0,1,2,3,4} =
		    [all]   0
		*/
		dp_mca_and_or(id, dp, mca_i, 0x8000002E0701103F, 0, 0);

		/* IOM0.DDRPHY_DP16_WR_VREF_STATUS1_P0_{0,1,2,3,4} =
		    [all]   0
		*/
		dp_mca_and_or(id, dp, mca_i, 0x8000002F0701103F, 0, 0);

		/* IOM0.DDRPHY_DP16_WR_VREF_ERROR_MASK{0,1}_P0_{0,1,2,3,4} =
		    [all]   0
		    [48-63] 0xffff
		*/
		dp_mca_and_or(id, dp, mca_i, 0x800000FA0701103F, 0, PPC_BITMASK(48, 63));
		dp_mca_and_or(id, dp, mca_i, 0x800000FB0701103F, 0, PPC_BITMASK(48, 63));

		/* IOM0.DDRPHY_DP16_WR_VREF_ERROR{0,1}_P0_{0,1,2,3,4} =
		    [all]   0
		*/
		dp_mca_and_or(id, dp, mca_i, 0x800000AE0701103F, 0, 0);
		dp_mca_and_or(id, dp, mca_i, 0x800000AE0701103F, 0, 0);

		/* Assume RDIMM
		 * Assume unpopulated DIMMs/ranks are not calibrated so their settings doesn't matter (more reg accesses, much simpler code)
		IOM0.DDRPHY_DP16_WR_VREF_VALUE{0,1}_RANK_PAIR0_P0_{0,1,2,3,4} =   // 0x8000005{E,F}0701103F, +0x0400_0000_0000
		IOM0.DDRPHY_DP16_WR_VREF_VALUE{0,1}_RANK_PAIR1_P0_{0,1,2,3,4} =   // 0x8000015{E,F}0701103F, +0x0400_0000_0000
		IOM0.DDRPHY_DP16_WR_VREF_VALUE{0,1}_RANK_PAIR2_P0_{0,1,2,3,4} =   // 0x8000025{E,F}0701103F, +0x0400_0000_0000
		IOM0.DDRPHY_DP16_WR_VREF_VALUE{0,1}_RANK_PAIR3_P0_{0,1,2,3,4} =   // 0x8000035{E,F}0701103F, +0x0400_0000_0000
		    [all]   0
		    [49]    WR_VREF_RANGE_DRAM{0,2} = ATTR_MSS_VPD_MT_VREF_DRAM_WR & 0x40
		    [50-55] WR_VREF_VALUE_DRAM{0,2} = ATTR_MSS_VPD_MT_VREF_DRAM_WR & 0x3f
		    [57]    WR_VREF_RANGE_DRAM{1,3} = ATTR_MSS_VPD_MT_VREF_DRAM_WR & 0x40
		    [58-63] WR_VREF_VALUE_DRAM{1,3} = ATTR_MSS_VPD_MT_VREF_DRAM_WR & 0x3f
		*/
		dp_mca_and_or(id, dp, mca_i, 0x8000005E0701103F, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 55) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 63));
		dp_mca_and_or(id, dp, mca_i, 0x8000005F0701103F, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 55) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 63));
		dp_mca_and_or(id, dp, mca_i, 0x8000015E0701103F, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 55) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 63));
		dp_mca_and_or(id, dp, mca_i, 0x8000015F0701103F, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 55) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 63));
		dp_mca_and_or(id, dp, mca_i, 0x8000025E0701103F, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 55) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 63));
		dp_mca_and_or(id, dp, mca_i, 0x8000025F0701103F, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 55) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 63));
		dp_mca_and_or(id, dp, mca_i, 0x8000035E0701103F, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 55) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 63));
		dp_mca_and_or(id, dp, mca_i, 0x8000035F0701103F, 0,
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 55) |
		              PPC_SHIFT(ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx], 63));
	}
}

static void reset_drift_limits(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_DRIFT_LIMITS_P0_{0,1,2,3,4} =
		    [48-49] DD2_BLUE_EXTEND_RANGE = 1         // always ONE_TO_FOUR due to red waterfall workaround
		*/
		dp_mca_and_or(id, dp, mca_i, 0x8000000A0701103F, ~PPC_BITMASK(48, 49),
		              PPC_SHIFT(1, 49));
	}
}

static void rd_dia_config5(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_RD_DIA_CONFIG5_P0_{0,1,2,3,4} =
		    // "this isn't an EC feature workaround, it's a incorrect documentation workaround"
		    [all]   0
		    [49]    DYN_MCTERM_CNTL_EN =      1
		    [52]    PER_CAL_UPDATE_DISABLE =  1     // "This bit must be set to 0 for normal operation"
		    [59]    PERCAL_PWR_DIS =          1
		*/
		dp_mca_and_or(id, dp, mca_i, 0x800000120701103F, 0,
		              PPC_BIT(49) | PPC_BIT(52) | PPC_BIT(59));
	}
}

static void dqsclk_offset(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int dp;

	for (dp = 0; dp < 5; dp++) {
		/* IOM0.DDRPHY_DP16_DQSCLK_OFFSET_P0_{0,1,2,3,4} =
		    // "this isn't an EC feature workaround, it's a incorrect documentation workaround"
		    [all]   0
		    [49-55] DQS_OFFSET = 0x08       // Config provided by S. Wyatt 9/13
		*/
		dp_mca_and_or(id, dp, mca_i, 0x800000370701103F, 0,
		              PPC_SHIFT(0x08, 55));
	}
}

static void phy_scominit(int mcs_i, int mca_i)
{
	/* Hostboot here sets strength, we did it in p9n_ddrphy_scom(). */
	set_rank_pairs(mcs_i, mca_i);

	reset_data_bit_enable(mcs_i, mca_i);

	/* Assume there are no bad bits (disabled DQ/DQS lines) for now */
	// reset_bad_bits();

	reset_clock_enable(mcs_i, mca_i);
	reset_rd_vref(mcs_i, mca_i);

	pc_reset(mcs_i, mca_i);
	wc_reset(mcs_i, mca_i);
	rc_reset(mcs_i, mca_i);
	seq_reset(mcs_i, mca_i);

	reset_ac_boost_cntl(mcs_i, mca_i);
	reset_ctle_cntl(mcs_i, mca_i);
	reset_delay(mcs_i, mca_i);
	reset_tsys_adr(mcs_i, mca_i);
	reset_tsys_data(mcs_i, mca_i);
	reset_io_impedances(mcs_i, mca_i);
	reset_wr_vref_registers(mcs_i, mca_i);
	reset_drift_limits(mcs_i, mca_i);

	/* Workarounds */

	/* Doesn't apply to DD2 */
	// dqs_polarity();

	rd_dia_config5(mcs_i, mca_i);
	dqsclk_offset(mcs_i, mca_i);

	/* Doesn't apply to DD2 */
	// odt_config();
}

static void fir_unmask(int mcs_i, int mca_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	/* IOM0.IOM_PHY0_DDRPHY_FIR_REG =
	  [56]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_2 = 0   // calibration errors
	  [58]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_4 = 0   // DLL errors
	*/
	mca_and_or(id, mca_i, 0x07011000, ~(PPC_BIT(56) | PPC_BIT(58)), 0);

	/* MC01.PORT0.SRQ.MBACALFIRQ =
	  [4]   MBACALFIRQ_RCD_PARITY_ERROR = 0
	  [8]   MBACALFIRQ_DDR_MBA_EVENT_N =  0
	*/
	mca_and_or(id, mca_i, 0x07010900, ~(PPC_BIT(4) | PPC_BIT(8)), 0);
}

/*
 * 13.8 mss_scominit: Perform scom inits to MC and PHY
 *
 * - HW units included are MCBIST, MCA/PHY (Nimbus) or membuf, L4, MBAs (Cumulus)
 * - Does not use initfiles, coded into HWP
 * - Uses attributes from previous step
 * - Pushes memory extent configuration into the MBA/MCAs
 *   - Addresses are pulled from attributes, set previously by mss_eff_config
 *   - MBA/MCAs always start at address 0, address map controlled by
 *     proc_setup_bars below
 */
void istep_13_8(void)
{
	printk(BIOS_EMERG, "starting istep 13.8\n");
	int mcs_i, mca_i;

	report_istep(13,8);

	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		/* We neither started clocks nor dropped fences in 13.6 for non-functional MCS */
		if (!mem_data.mcs[mcs_i].functional)
			continue;

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
			/*
			 * 0th MCA is 'magic' - it has a logic PHY block that is not contained
			 * in other MCA. The magic MCA must be always initialized, even when it
			 * doesn't have any DIMM installed.
			 */
			if (mca_i != 0 && !mca->functional)
				continue;

			/* Some registers cannot be initialized without data from SPD */
			if (mca->functional) {
				/* Assume DIMM mixing rules are followed - same rank config on both DIMMs*/
				p9n_mca_scom(mcs_i, mca_i);
				thermal_throttle_scominit(mcs_i, mca_i);
			}

			/* The rest can and should be initialized also on magic port */
			p9n_ddrphy_scom(mcs_i, mca_i);
		}
		p9n_mcbist_scom(mcs_i);
	}

	/* This double loop is a part of phy_scominit() in Hostboot, but this is simpler. */
	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		if (!mem_data.mcs[mcs_i].functional)
			continue;

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
			/* No magic for phy_scmoinit(). */
			if (mca->functional)
				phy_scominit(mcs_i, mca_i);

			/*
			 * TODO: test this with DIMMs on both MCS. Maybe this has to be done
			 * in a separate loop, after phy_scominit()'s are done on both MCSs.
			 */
			if (mca_i == 0 || mca->functional)
				fir_unmask(mcs_i, mca_i);
		}
	}

	printk(BIOS_EMERG, "ending istep 13.8\n");
}
