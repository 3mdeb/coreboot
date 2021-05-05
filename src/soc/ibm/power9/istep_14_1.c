/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <cpu/power/spr.h>
#include <console/console.h>

static void fir_unmask(int mcs_i)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int mca_i;
	const int is_dd20 = pvr_revision() == SPR_PVR_REV(2, 0);
	/* Bits in other registers (act0, mask) are already set properly.
	MC01.MCBIST.MBA_SCOMFIR.MCBISTFIRACT1
		  [3]   MCBISTFIRQ_MCBIST_BRODCAST_OUT_OF_SYNC =  0 // checkstop (0,0,0)
	*/
	scom_and_or_for_chiplet(id, 0x07012307, ~PPC_BIT(3), 0);

	for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
		uint64_t val;
		if (!mem_data.mcs[mcs_i].mca[mca_i].functional)
			continue;

		/* From broadcast_out_of_sync() workaround:
		MC01.PORT0.ECC64.SCOM.RECR
			[26]  MBSECCQ_ENABLE_UE_NOISE_WINDOW =  1
		*/
		mca_and_or(id, mca_i, 0x07010A0A, ~0, PPC_BIT(26));

		/*
		 * Read out the wr_done and rd_tag delays and find min and set the RCD
		 * Protect Time to this value.
		 *
		 * MC01.PORT0.SRQ.MBA_DSM0Q
		 *   [24-29] MBA_DSM0Q_CFG_WRDONE_DLY
		 *   [36-41] MBA_DSM0Q_CFG_RDTAG_DLY
		 *
		 * MC01.PORT0.SRQ.MBA_FARB0Q
		 *   [48-53] MBA_FARB0Q_CFG_RCD_PROTECTION_TIME
		 */
		val = mca_read(id, mca_i, 0x0701090A);
		val = MIN((val & PPC_BITMASK(24, 29)) >> 29,
		          (val & PPC_BITMASK(36, 41)) >> 41);
		mca_and_or(id, mca_i, 0x07010913,
		           ~PPC_BITMASK(48, 53), PPC_SHIFT(val, 53));

		/*
		 * Due to hardware defect with DD2.0 certain errors are not handled
		 * properly. As a result, these firs are marked as checkstop for DD2 to
		 * avoid any mishandling.
		 *
		 * MCA_FIR_MAINLINE_RCD stays masked on newer platforms. ACT0 and ACT1
		 * for RCD are not touched by Hostboot, but for simplicity set those to
		 * 0 always - they are "don't care" if masked, and 0 is their reset
		 * value. Affected bits are annotated with asterisk below - whatever is
		 * mentioned below is changed to checkstop for those bits.
		 *
		 * This also affects Cumulus DD1.0, but the rest of the code is for
		 * Nimbus only so don't bother checking for it.
		 *
		 * MC01.PORT0.ECC64.SCOM.ACTION0
		 *   [13] FIR_MAINLINE_AUE =         0
		 *   [14] FIR_MAINLINE_UE =          0
		 *   [15] FIR_MAINLINE_RCD =         0
		 *   [16] FIR_MAINLINE_IAUE =        0
		 *   [17] FIR_MAINLINE_IUE =         0
		 *   [37] MCA_FIR_MAINTENANCE_IUE =  0
		 * MC01.PORT0.ECC64.SCOM.ACTION1
		 *   [13] FIR_MAINLINE_AUE =         0
		 *   [14] FIR_MAINLINE_UE =          1*
		 *   [15] FIR_MAINLINE_RCD =         0
		 *   [16] FIR_MAINLINE_IAUE =        0
		 *   [17] FIR_MAINLINE_IUE =         1
		 *   [33] MCA_FIR_MAINTENANCE_AUE =  0  // Hostboot clears AUE and IAUE without
		 *   [36] MCA_FIR_MAINTENANCE_IAUE = 0  // unmasking, with no explanation why
		 *   [37] MCA_FIR_MAINTENANCE_IUE =  1
		 * MC01.PORT0.ECC64.SCOM.MASK
		 *   [13] FIR_MAINLINE_AUE =         0  // checkstop (0,0,0)
		 *   [14] FIR_MAINLINE_UE =          0  // *recoverable_error (0,1,0)
		 *   [15] FIR_MAINLINE_RCD =         1* // *masked (X,X,1)
		 *   [16] FIR_MAINLINE_IAUE =        0  // checkstop (0,0,0)
		 *   [17] FIR_MAINLINE_IUE =         0  // recoverable_error (0,1,0)
		 *   [37] MCA_FIR_MAINTENANCE_IUE =  0  // recoverable_error (0,1,0)
		 */
		mca_and_or(id, mca_i, 0x07010A06,
		           ~(PPC_BITMASK(13, 17) | PPC_BIT(37)),
		           0);
		mca_and_or(id, mca_i, 0x07010A07,
		           ~(PPC_BITMASK(13, 17) | PPC_BIT(33) | PPC_BITMASK(36, 37)),
		           (is_dd20 ? 0 : PPC_BIT(14)) | PPC_BIT(17) | PPC_BIT(37));
		mca_and_or(id, mca_i, 0x07010A03,
		           ~(PPC_BITMASK(13, 17) | PPC_BIT(37)),
		           (is_dd20 ? 0 : PPC_BIT(15)));

		/*
		 * WARNING: checkstop is encoded differently (1,0,0). **Do not** try to
		 * make a function/macro that pretends to be universal.
		 *
		 * MC01.PORT0.SRQ.MBACALFIR_ACTION0
		 *   [13] MBACALFIRQ_PORT_FAIL = 0*
		 * MC01.PORT0.SRQ.MBACALFIR_ACTION1
		 *   [13] MBACALFIRQ_PORT_FAIL = 1*
		 * MC01.PORT0.SRQ.MBACALFIR_MASK
		 *   [13] MBACALFIRQ_PORT_FAIL = 0  // *recoverable_error (0,1,0)
		 */
		mca_and_or(id, mca_i, 0x07010906,
		           ~PPC_BIT(13),
		           (is_dd20 ? PPC_BIT(13) : 0));
		mca_and_or(id, mca_i, 0x07010907,
		           ~PPC_BIT(13),
		           (is_dd20 ? 0 : PPC_BIT(13)));
		mca_and_or(id, mca_i, 0x07010903, ~PPC_BIT(13), 0);

		/*
		 * Enable port fail and RCD recovery
		 * TODO: check if we can set this together with RCD protection time.
		 *
		 * MC01.PORT0.SRQ.MBA_FARB0Q
		 *   [54] MBA_FARB0Q_CFG_DISABLE_RCD_RECOVERY = 0
		 *   [57] MBA_FARB0Q_CFG_PORT_FAIL_DISABLE =    0
		 */
		mca_and_or(id, mca_i, 0x07010913, ~(PPC_BIT(54) | PPC_BIT(57)), 0);
	}
}

static void set_fifo_mode(int mcs_i, int fifo)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	int mca_i;
	/* Make sure fifo is either 0 or 1, nothing else. */
	fifo = !!fifo;

	/* MC01.PORT0.SRQ.MBA_RRQ0Q
	 *   [6] MBA_RRQ0Q_CFG_RRQ_FIFO_MODE = fifo
	 * MC01.PORT0.SRQ.MBA_WRQ0Q
	 *   [5] MBA_WRQ0Q_CFG_WRQ_FIFO_MODE = fifo
	 */
	for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
		if (!mem_data.mcs[mcs_i].mca[mca_i].functional)
			continue;

		mca_and_or(id, mca_i, 0x0701090E, ~PPC_BIT(6), PPC_SHIFT(fifo, 6));
		mca_and_or(id, mca_i, 0x0701090D, ~PPC_BIT(5), PPC_SHIFT(fifo, 5));
	}
}

/*
 * 14.1 mss_memdiag: Mainstore Pattern Testing
 *
 * - The following step documents the generalities of this step
 *   - In FW PRD will control mem diags via interrupts. It doesn't use
 *     mss_memdiags.C directly but the HWP subroutines
 *   - In cronus it will execute mss_memdiags.C directly
 * b) p9_mss_memdiags.C (mcbist)--Nimbus
 * c) p9_mss_memdiags.C (mba) -- Cumulus
 *    - Prior to running this procedure will apply known DQ bad bits to prevent
 *      them from participating in training. This information is extracted from
 *      the bad DQ attribute and applied to Hardware
 *    - Nimbus uses the mcbist engine
 *      - Still supports superfast read/init/scrub
 *    - Cumulus/Centaur uses the scrub engine
 *    - Modes:
 *      - Minimal: Write-only with 0's
 *      - Standard: Write of 0’s followed by a Read
 *      - Medium: Write-followed by Read, 4 patterns, last of 0's
 *      - Max: Write-followed by Read, 9 patterns, last of 0's
 *    - Run on the host
 *    - This procedure will update the bad DQ attribute for each dimm based on
 *      its findings
 *    - At the end of this procedure sets FIR masks correctly for runtime
 *      analysis
 *    - All subsequent repairs are considered runtime issues
 */
void istep_14_1(void)
{
	int mcs_i;
	printk(BIOS_EMERG, "starting istep 14.1\n");
	report_istep(14,1);

	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		if (!mem_data.mcs[mcs_i].functional)
			continue;

		/*
		 * FIXME: add testing
		 *
		 * Testing touches bad DQ registers. This step also configures MC to
		 * deal with bad nibbles/DQs - see can_recover() in 13.11. It repeats,
		 * to some extent, training done in 13.12 which is TODO. Following the
		 * assumptions made in previous isteps, skip this for now.
		 *
		 * Note that skipping this step results in RAM not being zeroed. This is
		 * both a security issue (secrets in memory are not cleared) and
		 * possibly may impact next stages - BSS is cleared by selfloader, but
		 * what about CBMEM etc?
		 */

		/* Unmask mainline FIRs. */
		fir_unmask(mcs_i);

		/* Turn off FIFO mode to improve performance. */
		/*
		 * TODO: this is not needed until real testing is implemented. FIFO mode
		 * is enabled as a part of the testing (twice, actually), but before
		 * this step it was disabled.
		 */
		set_fifo_mode(mcs_i, 0);
	}

	printk(BIOS_EMERG, "ending istep 14.1\n");
}
