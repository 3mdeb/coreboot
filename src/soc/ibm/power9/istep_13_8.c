/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <console/console.h>
#include <cpu/power/scom.h>
#include <cpu/power/scom_registers.h>
#include <delay.h>
#include <timer.h>

#define ATTR_PG			0xE000000000000000ull
#define FREQ_PB_MHZ		1866

#define PPC_SHIFT(val, lsb)	(((uint64_t)(val)) << (63 - lsb))

/* TODO: discover how MCAs are numbered (0,1,2,3? 0,1,6,7? 0,1,4,5?) */
/* TODO: consider non-RMW variant */
static inline void mca_and_or(chiplet_id_t mcs, int mca, uint64_t scom,
                              uint64_t and, uint64_t or)
{
	/* Indirect registers have different stride than the direct ones. */
	unsigned mul = (scom & PPC_BIT(0)) ? 0x400 : 0x40;
	scom_and_or_for_chiplet(mcs, scom + mca * mul, and, or);
}

static inline void dp_mca_and_or(chiplet_id_t mcs, int dp, int mca,
                                 uint64_t scom, uint64_t and, uint64_t or)
{
	mca_and_or(mcs, mca, scom + dp * 0x40000000000, and, or);
}

/*
 * This function was generated from initfiles. Some of the registers used here
 * are not documented, except for occasional name of a constant written to it.
 * They also access registers at addresses for chiplet ID = 5 (Nest west), even
 * though the specified target is MCA. It is not clear if MCA offset has to be
 * added to SCOM address for those registers or not. Undocumented registers are
 * marked with (?) in the comments.
 */
static void p9n_mca_scom(chiplet_id_t id, int mca_i, mca_data_t *mca)
{
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
	uint64_t en_ref_blk = log_ranks > 8 ? 0 :
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

static void thermal_throttle_scominit(chiplet_id_t id, int mca_i)
{
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
	mca_and_or(id, mca_i, 0x07010916, ~(PPC_BITMASK(0, 50) | PPC_BIT(53)),
	           /* PPC_SHIFT(nm_n_per_slot, 14) | PPC_SHIFT(nm_n_per_port, 30) | */
	           PPC_SHIFT(0x200, 44) | PPC_SHIFT(1, 50));

	/* Set safemode throttles */
	/* MC01.PORT0.SRQ.MBA_FARB4Q =
	    [27-41] MBA_FARB4Q_EMERGENCY_N = ATTR_MSS_RUNTIME_MEM_THROTTLED_N_COMMANDS_PER_PORT[mss::index(MCA)]  // BUG? var name says per_slot...
	    [42-55] MBA_FARB4Q_EMERGENCY_M = ATTR_MSS_MRW_MEM_M_DRAM_CLOCKS
	*/
	mca_and_or(id, mca_i, 0x07010917, ~PPC_BITMASK(27, 55),
	           /* PPC_SHIFT(nm_n_per_port, 41) | */
	           PPC_SHIFT(0x200, 55));
}

/*
 * Values set in this function are mostly for magic MCA, other (functional) MCAs
 * are set later. If all of these registers are later written with proper values
 * for functional MCAs, maybe this can be called just for magic, non-functional
 * ones to save time.
 */
static void p9n_ddrphy_scom(chiplet_id_t id, int mca_i)
{
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
		              ~(PPC_BITMASK(48, 51) | PPC_BITMASK(53, 56) | PPC_BITMASK(61, 62)),
		              PPC_BIT(61) | PPC_BIT(62));

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
	}

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

static void p9n_mcbist_scom(chiplet_id_t id)
{
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

static void set_rank_pairs(chiplet_id_t id, int mca_i, mca_data_t *mca)
{
	/*
	 * Assumptions:
	 * - non-LR DIMMs (platform wiki),
	 * - no ATTR_EFF_RANK_GROUP_OVERRIDE,
	 * - mixing rules followed - the same rank configuration for both DIMMs.
	 */

	static const uint16_t F[] = {0, 0xf000, 0xf0f0, 0xfff0, 0xffff};
	/* TODO: maybe we should check if dimm[0].mranks == dimm[1].mranks? */
	uint64_t rp = 0x1537 & F[mca->dimm[0].present ? mca->dimm[0].mranks :
	                                                mca->dimm[1].mranks];

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
	           PPC_SHIFT(rp, 63));

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
	           PPC_SHIFT(rp, 63));

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

static void reset_data_bit_enable(chiplet_id_t id, int mca_i)
{
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

static void reset_clock_enable(chiplet_id_t id, int mcs_i, int mca_i, mca_data_t *mca)
{
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

static void phy_scominit(chiplet_id_t id, int mcs_i, int mca_i, mca_data_t *mca)
{
	/* Hostboot here sets strength, we did it in p9n_ddrphy_scom(). */
	set_rank_pairs(id, mca_i, mca);

	reset_data_bit_enable(id, mca_i);

	/* Assume there are no bad bits (disabled DQ/DQS lines) for now */
	// reset_bad_bits();

	reset_clock_enable(id, mcs_i, mca_i, mca);

	//reset_rd_vref();
	// ... TBD ...
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
	chiplet_id_t mcs_ids[MCS_PER_PROC] = {MC01_CHIPLET_ID, MC23_CHIPLET_ID};

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
				p9n_mca_scom(mcs_ids[mcs_i], mca_i, mca);
				thermal_throttle_scominit(mcs_ids[mcs_i], mca_i);
			}

			/* The rest can and should be initialized also on magic port */
			p9n_ddrphy_scom(mcs_ids[mcs_i], mca_i);
		}
		/* This is for MCBIST, does it need chiplet ID? */
		p9n_mcbist_scom(mcs_ids[mcs_i]);
	}


	/* This double loop is a part of phy_scominit() in Hostboot, but this is simpler. */
	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
			/* No magic this time. */
			if (!mca->functional)
				continue;

			phy_scominit(mcs_ids[mcs_i], mcs_i, mca_i, mca);
		}
	}

	// mss::unmask::after_scominit();

	printk(BIOS_EMERG, "ending istep 13.8\n");
}
