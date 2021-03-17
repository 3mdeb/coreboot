/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <console/console.h>
#include <cpu/power/scom.h>

static int test_dll_calib_done(chiplet_id_t id, int mca_i, bool *do_workaround)
{
	uint64_t status = mca_read(mcs_ids[mcs_i], 0, 0x8000C0000701103F);
	/*
	if (IOM0.DDRPHY_PC_DLL_ZCAL_CAL_STATUS_P0
			[48]  DP_DLL_CAL_GOOD ==        1
			[49]  DP_DLL_CAL_ERROR ==       0
			[50]  DP_DLL_CAL_ERROR_FINE ==  0
			[51]  ADR_DLL_CAL_GOOD ==       1
			[52]  ADR_DLL_CAL_ERROR ==      0
			[53]  ADR_DLL_CAL_ERROR_FINE == 0) break    // success
	if (IOM0.DDRPHY_PC_DLL_ZCAL_CAL_STATUS_P0
			[49]  DP_DLL_CAL_ERROR ==       1 |
			[50]  DP_DLL_CAL_ERROR_FINE ==  1 |
			[52]  ADR_DLL_CAL_ERROR ==      1 |
			[53]  ADR_DLL_CAL_ERROR_FINE == 1) break and do the workaround
	*/
	if (status & PPC_BITMASK(48, 53) == PPC_BIT(48) | PPC_BIT(51)) {
		/* DLL calibration finished without errors */
		return 1;
	}

	if (status & (PPC_BIT(49) | PPC_BIT(50) | PPC_BIT(52) | PPC_BIT(53))) {
		/* DLL calibration finished, but with errors */
		*do_workaround = true;
		return 1;
	}

	/* Not done yet */
	return 0;
}

static void fix_bad_voltage_settings(void)
{
	die("fix_bad_voltage_settings() required, but not implemented yet\n");

	/* TODO: implement if needed */
/*
  for each functional MCA
	// Each MCA has 10 DLLs: ADR DLL0, DP0-4 DLL0, DP0-3 DLL1. Each of those can fail. For each DLL there are 5 registers
	// used in this workaround, those are (see src/import/chips/p9/procedures/hwp/memory/lib/workarounds/dll_workaround.C):
	// - l_CNTRL:         DP16 or ADR CNTRL register
	// - l_COARSE_SAME:   VREG_COARSE register for same DLL as CNTRL reg
	// - l_COARSE_NEIGH:  VREG_COARSE register for DLL neighbor for this workaround
	// - l_DAC_LOWER:     DLL DAC Lower register
	// - l_DAC_UPPER:     DLL DAC Upper register
	// Warning: the last two have their descriptions swapped in dll_workaround.H
	// It seems that the code excepts that DLL neighbor is always good, what if it isn't?
	//
	// General flow, stripped from C++ bloating and repeated loops:
	for each DLL          // list in workarounds/dll_workaround.C
	  1. check if this DLL failed, if not - skip to the next one
			(l_CNTRL[62 | 63] | l_COARSE_SAME[56-62] == 1) -> failed
	  2. set reset bit, set skip VREG bit, clear the error bits
			l_CNTRL[48] =     1
			l_CNTRL[50-51] =  2     // REGS_RXDLL_CAL_SKIP, 2 - skip VREG calib., do coarse delay calib. only
			l_CNTRL[62-63] =  0
	  3. clear DLL FIR (see "Do FIRry things" at the end of 13.8)  // this was actually done for non-failed DLLs too, why?
			IOM0.IOM_PHY0_DDRPHY_FIR_REG =      // 0x07011000         // maybe use SCOM1 (AND) 0x07011001
				  [56]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_2 = 0   // calibration errors
				  [58]  IOM_PHY0_DDRPHY_FIR_REG_DDR_FIR_ERROR_4 = 0   // DLL errors
	  4. write the VREG DAC value found in neighbor (good) to the failing DLL VREG DAC
			l_COARSE_SAME[56-62] = l_COARSE_NEIGH[56-62]
	  5. reset the upper and lower fine calibration bits back to defaults
			l_DAC_LOWER[56-63] =  0x0x8000    // Hard coded default values per Steve Wyatt for this workaround
			l_DAC_UPPER[56-63] =  0x0xFFE0
	  6. run DLL Calibration again on failed DLLs
			l_CNTRL[48] = 0
	// Wait for calibration to finish
	delay(37,382 memclocks)     // again, we could do better than this

	// Check if calibration succeeded (same tests as in 1 above, for all DLLs)
	for each DLL
	  if (l_CNTRL[62 | 63] | l_COARSE_SAME[56-62] == 1): failed, assert and die?
*/
}

/*
 * Can't protect with do..while, this macro is supposed to exit 'for' loop in
 * which it is invoked. As a side effect, it is used without semicolon.
 *
 * "I want to break free" - Freddie Mercury
 */
#define TEST_VREF(dp, scom) \
if (dp_mca_read(mcs_ids[mcs_i], dp, mca_i, scom) & PPC_BITMASK(56, 62) == \
             PPC_SHIFT(1,62)) { \
	need_dll_workaround = true; \
	break; \
}

/*
 * 13.9 mss_ddr_phy_reset: Soft reset of DDR PHY macros
 *
 * - Lock DDR DLLs
 *   - Already configured DDR DLL in scaninit
 * - Sends Soft DDR Phy reset
 * - Kick off internal ZQ Cal
 * - Perform any config that wasn't scanned in (TBD)
 *   - Nothing known here
 */
void istep_13_9(void)
{
	printk(BIOS_EMERG, "starting istep 13.9\n");
	int mcs_i, mca_i, dp;
	long time;
	chiplet_id_t mcs_ids[MCS_PER_PROC] = {MC01_CHIPLET_ID, MC23_CHIPLET_ID};
	bool need_dll_workaround;

	report_istep(13,9);

	/*
	 * Most of this istep consists of:
	 * 1. asserting reset bit or starting calibration
	 * 2. delay
	 * 3. deasserting reset bit or checking the result of calibration
	 *
	 * These are done for each (functional and/or magic) MCA. Because the delay
	 * is required between points 1 and 3 for a given MCA, those delays are done
	 * outside of 'for each MCA' loops. They are still inside 'for each MCS'
	 * loop, unclear if we can break it into pieces too.
	 */
	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		if (!mem_data.mcs[mcs_i].functional)
			continue;

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (mca_i != 0 && !mca->functional)
				continue;

			/* MC01.PORT0.SRQ.MBA_FARB5Q =
				[8]     MBA_FARB5Q_CFG_FORCE_MCLK_LOW_N = 0
			*/
			mca_and_or(mcs_ids[mcs_i], mca_i, 0x07010918, ~PPC_BIT(8), 0);

			/* Drive all control signals to their inactive/idle state, or
			 * inactive value
			IOM0.DDRPHY_DP16_SYSCLK_PR0_P0_{0,1,2,3,4} =
			IOM0.DDRPHY_DP16_SYSCLK_PR1_P0_{0,1,2,3,4} =
				[all]   0
				[48]    reserved = 1            // MCA_DDRPHY_DP16_SYSCLK_PR0_P0_0_01_ENABLE
			*/
			for (dp = 0; dp < 5; dp++) {
				dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i, 0x800000070701103F, 0,
				              PPC_BIT(48));
				dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i, 0x8000007F0701103F, 0,
				              PPC_BIT(48));
			}

			/* Assert reset to PHY for 32 memory clocks
			MC01.PORT0.SRQ.MBA_CAL0Q =
				[57]    MBA_CAL0Q_RESET_RECOVER = 1
			*/
			mca_and_or(mcs_ids[mcs_i], mca_i, 0x0701090F, ~0, PPC_BIT(57));
		}

		delay_nck(32);

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (mca_i != 0 && !mca->functional)
				continue;

			/* Deassert reset_n
			MC01.PORT0.SRQ.MBA_CAL0Q =
					[57]    MBA_CAL0Q_RESET_RECOVER = 0
			*/
			mca_and_or(mcs_ids[mcs_i], mca_i, 0x0701090F, ~PPC_BIT(57), 0);

			/* Flush output drivers
			IOM0.DDRPHY_ADR_OUTPUT_FORCE_ATEST_CNTL_P0_ADR32S{0,1} =
					[all]   0
					[48]    FLUSH =   1
					[50]    INIT_IO = 1
			*/
			/* Has the same stride as DP16 */
			dp_mca_and_or(mcs_ids[mcs_i], 0, mca_i, 0x800080350701103F, 0,
			              PPC_BIT(48) | PPC_BIT(50));
			dp_mca_and_or(mcs_ids[mcs_i], 1, mca_i, 0x800080350701103F, 0,
			              PPC_BIT(48) | PPC_BIT(50));

			/* IOM0.DDRPHY_DP16_CONFIG0_P0_{0,1,2,3,4} =
					[all]   0
					[51]    FLUSH =                 1
					[54]    INIT_IO =               1
					[55]    ADVANCE_PING_PONG =     1
					[58]    DELAY_PING_PONG_HALF =  1
			*/
			for (dp = 0; dp < 5; dp++) {
				dp_mca_and_or(mcs_ids[mcs_i], 0, mca_i, 0x800000030701103F, 0,
				              PPC_BIT(51) | PPC_BIT(54) | PPC_BIT(55) | PPC_BIT(58));
			}
		}

		delay_nck(32);

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (mca_i != 0 && !mca->functional)
				continue;

			/* IOM0.DDRPHY_ADR_OUTPUT_FORCE_ATEST_CNTL_P0_ADR32S{0,1} =
					[all]   0
					[48]    FLUSH =   0
					[50]    INIT_IO = 0
			*/
			/* Has the same stride as DP16 */
			dp_mca_and_or(mcs_ids[mcs_i], 0, mca_i, 0x800080350701103F, 0, 0);
			dp_mca_and_or(mcs_ids[mcs_i], 1, mca_i, 0x800080350701103F, 0, 0);

			/* IOM0.DDRPHY_DP16_CONFIG0_P0_{0,1,2,3,4} =
					[all]   0
					[51]    FLUSH =                 0
					[54]    INIT_IO =               0
					[55]    ADVANCE_PING_PONG =     1
					[58]    DELAY_PING_PONG_HALF =  1
			*/
			for (dp = 0; dp < 5; dp++) {
				dp_mca_and_or(mcs_ids[mcs_i], 0, mca_i, 0x800000030701103F, 0,
				              PPC_BIT(55) | PPC_BIT(58));
			}
		}

		/* ZCTL Enable */
		/*
		 * In Hostboot this is 'for each magic MCA'. We know there is only one
		 * magic, and it has always the same index.
		IOM0.DDRPHY_PC_RESETS_P0 =                                  // 0x8000C00E0701103F
			// Yet another documentation error: all bits in this register are marked as read-only
			[51]    ENABLE_ZCAL = 1
		 * TODO: for MCS1, is it MCA0 or MCA2?
		 */
		mca_and_or(mcs_ids[mcs_i], 0, 0x8000C00E0701103F, ~0, PPC_BIT(51));

		/* Maybe it would be better to add another 1us later instead of this. */
		delay_nck(1024);

		/* for each magic MCA */
		/* 50*10ns, but we don't have such precision. */
		time = wait_us(1, mca_read(mcs_ids[mcs_i], 0, 0x8000C0000701103F) & PPC_BIT(63));

		if (!time)
			die("ZQ calibration timeout\n");

		/* DLL calibration */
		/*
		 * Here was an early return if no functional MCAs were found. Wouldn't
		 * that make whole MCBIST non-functional?
		 */
		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (!mca->functional)
				continue;

			/* IOM0.DDRPHY_ADR_DLL_CNTL_P0_ADR32S{0,1} =
				[48]    INIT_RXDLL_CAL_RESET = 0
			*/
			/* Has the same stride as DP16 */
			dp_mca_and_or(mcs_ids[mcs_i], 0, mca_i, 0x8000803A0701103F, ~PPC_BIT(48), 0);
			dp_mca_and_or(mcs_ids[mcs_i], 1, mca_i, 0x8000803A0701103F, ~PPC_BIT(48), 0);

			for (dp = 0; dp < 4; dp++) {
				/* IOM0.DDRPHY_DP16_DLL_CNTL{0,1}_P0_{0,1,2,3} =
					[48]    INIT_RXDLL_CAL_RESET = 0
				*/
				dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i, 0x800000240701103F, ~PPC_BIT(48), 0);
				dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i, 0x800000250701103F, ~PPC_BIT(48), 0);
			}
			/* Last DP16 is different
			IOM0.DDRPHY_DP16_DLL_CNTL0_P0_4
				[48]    INIT_RXDLL_CAL_RESET = 0
			IOM0.DDRPHY_DP16_DLL_CNTL1_P0_4
				[48]    INIT_RXDLL_CAL_RESET = 1
			*/
			dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i, 0x800000240701103F, ~PPC_BIT(48), 0);
			dp_mca_and_or(mcs_ids[mcs_i], dp, mca_i, 0x800000250701103F, ~0, PPC_BIT(48));
		}

		/* From Hostboot's comments:
		 * 32,772 dphy_nclk cycles from Reset=0 to VREG Calibration to exhaust all values
		 * 37,382 dphy_nclk cycles for full calibration to start and fail ("worst case")
		 *
		 * Why assume worst case instead of making the next timeout bigger?
		 */
		delay_nck(37382);

		/*
		 * The comment before poll says:
		 * > To keep things simple, we'll poll for the change in one of the ports.
		 * > Once that's completed, we'll check the others. If any one has failed,
		 * > or isn't notifying complete, we'll pop out an error
		 *
		 * The issue is that it only tests the first of the functional ports.
		 * Other ports may or may not have failed. Even if this times out, the
		 * rest of the function continues normally, without throwing any error...
		 *
		 * For now, leave it as it was done in Hostboot.
		 */
		/* timeout(50*10ns):
		  if (IOM0.DDRPHY_PC_DLL_ZCAL_CAL_STATUS_P0
					[48]  DP_DLL_CAL_GOOD ==        1
					[49]  DP_DLL_CAL_ERROR ==       0
					[50]  DP_DLL_CAL_ERROR_FINE ==  0
					[51]  ADR_DLL_CAL_GOOD ==       1
					[52]  ADR_DLL_CAL_ERROR ==      0
					[53]  ADR_DLL_CAL_ERROR_FINE == 0) break    // success
		  if (IOM0.DDRPHY_PC_DLL_ZCAL_CAL_STATUS_P0
					[49]  DP_DLL_CAL_ERROR ==       1 |
					[50]  DP_DLL_CAL_ERROR_FINE ==  1 |
					[52]  ADR_DLL_CAL_ERROR ==      1 |
					[53]  ADR_DLL_CAL_ERROR_FINE == 1) break and do the workaround
		*/
		need_dll_workaround = false;
		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			if (mem_data.mcs[mcs_i].mca[mca_i].functional)
				break;
		}
		/* 50*10ns, but we don't have such precision. */
		time = wait_us(1, test_dll_calib_done(mcs_ids[mcs_i], mca_i,
		                                      &need_dll_workaround));
		if (!time)
			die("DLL calibration timeout\n");

		/*
		 * Workaround is also required if any of coarse VREG has value 1 after
		 * calibration. Test from poll above is repeated here - this time for every
		 * MCA, but it doesn't wait until DLL gets calibrated if that is still in
		 * progress. The registers below (also used in the workaround) __must not__
		 * be written to while hardware calibration is in progress.
		 */
		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (need_dll_workaround)
				break;

			if (!mca->functional)
				continue;

			/*
			 * This assumes that by the time the first functional MCA completed
			 * successfully, all MCAs completed (with or without errors). If the
			 * first MCA failed then we won't even get here, we would bail earlier
			 * because need_dll_workaround == true in that case.
			 *
			 * This is not safe if DLL calibration takes more time for other MCAs,
			 * but this is the way Hostboot does it.
			 */
			test_dll_calib_done(mcs_ids[mcs_i], mca_i, &need_dll_workaround);

			/*
			if (IOM0.DDRPHY_ADR_DLL_VREG_COARSE_P0_ADR32S0        |       // 0x8000803E0701103F
			  IOM0.DDRPHY_DP16_DLL_VREG_COARSE0_P0_{0,1,2,3,4}  |       // 0x8000002C0701103F, +0x0400_0000_0000
			  IOM0.DDRPHY_DP16_DLL_VREG_COARSE1_P0_{0,1,2,3}    |       // 0x8000002D0701103F, +0x0400_0000_0000
					[56-62] REGS_RXDLL_VREG_DAC_COARSE = 1)    // The same offset for ADR and DP16
				  do the workaround
			*/
			TEST_VREF(0, 0x8000803E0701103F) /* ADR_DLL_VREG_COARSE_P0_ADR32S0 */
			TEST_VREF(4, 0x8000002C0701103F) /* DP16_DLL_VREG_COARSE0_P0_4 */
			for (dp = 0; dp < 4; dp++) {
				TEST_VREF(dp, 0x8000002C0701103F) /* DP16_DLL_VREG_COARSE0_P0_n */
				TEST_VREF(dp, 0x8000002D0701103F) /* DP16_DLL_VREG_COARSE1_P0_n */
			}
		}

		if (need_dll_workaround)
			fix_bad_voltage_settings();

		/* Start bang-bang-lock */
		// TBD...
	}

	printk(BIOS_EMERG, "ending istep 13.9\n");
}
