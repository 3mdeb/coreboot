/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <console/console.h>
#include <timer.h>

/*
 * TODO: ATTR_PG value should come from MEMD partition, but it is empty after
 * build. Default value from talos.xml (5 for all chiplets) probably never makes
 * sense. Value read from already booted MVPD is 0xE0 for both MCSs. We can
 * either add functions to read and parse MVPD or just hardcode the values. So
 * far I haven't found the code that writes to MVPD in Hostboot, other than for
 * PDI keyword (PG keyword should be used here).
 *
 * Value below is already shifted to simplify the code.
 */
#define ATTR_PG			0xE000000000000000ull

static inline void p9_mem_startclocks_cplt_ctrl_action_function(chiplet_id_t id)
{
	// Drop partial good fences
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL1 (WO_CLEAR)                // 0x07000021
	  [all]   0
	  [3]     TC_VITL_REGION_FENCE =                  ~ATTR_PG[3]
	  [4-14]  TC_REGION{1-3}_FENCE, UNUSED_{8-14}B =  ~ATTR_PG[4-14]
	*/
	write_scom_for_chiplet(id, 0x07000021, ~ATTR_PG & PPC_BITMASK(3,14));

	// Reset abistclk_muxsel and syncclk_muxsel
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_CLEAR)                // 0x07000020
	  [all]   0
	  [0]     CTRL_CC_ABSTCLK_MUXSEL_DC = 1
	  [1]     TC_UNIT_SYNCCLK_MUXSEL_DC = 1
	*/
	write_scom_for_chiplet(id, 0x07000020, PPC_BIT(0) | PPC_BIT(1));

}

static inline void p9_sbe_common_align_chiplets(chiplet_id_t id)
{
	// Exit flush
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_OR)                   // 0x07000010
	  [all]   0
	  [2]     CTRL_CC_FLUSHMODE_INH_DC =  1
	*/
	write_scom_for_chiplet(id, 0x07000010, PPC_BIT(2));

	// Enable alignement
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_OR)                   // 0x07000010
	  [all]   0
	  [3]     CTRL_CC_FORCE_ALIGN_DC =    1
	*/
	write_scom_for_chiplet(id, 0x07000010, PPC_BIT(3));

	// Clear chiplet is aligned
	/*
	TP.TCMC01.MCSLOW.SYNC_CONFIG                          // 0x07030000
	  [7]     CLEAR_CHIPLET_IS_ALIGNED =  1
	*/
	scom_or_for_chiplet(id, 0x07030000, PPC_BIT(7));

	// Unset Clear chiplet is aligned
	/*
	TP.TCMC01.MCSLOW.SYNC_CONFIG                          // 0x07030000
	  [7]     CLEAR_CHIPLET_IS_ALIGNED =  0
	*/
	scom_and_for_chiplet(id, 0x07030000, ~PPC_BIT(7));

	udelay(100);

	// Poll aligned bit
	/*
	timeout(10*100us):
	TP.TCMC01.MCSLOW.CPLT_STAT0                         // 0x07000100
	if (([9] CC_CTRL_CHIPLET_IS_ALIGNED_DC) == 1) break
	delay(100us)
	*/
	if (!wait_us(10 * 100, read_scom_for_chiplet(id, 0x07000100) & PPC_BIT(9)))
		die("Timeout while waiting for chiplet alignment\n");

	// Disable alignment
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_CLEAR)                // 0x07000020
	  [all]   0
	  [3]     CTRL_CC_FORCE_ALIGN_DC =  1
	*/
	write_scom_for_chiplet(id, 0x07000020, PPC_BIT(3));
}

static void p9_sbe_common_clock_start_stop(chiplet_id_t id)
{
	// Chiplet exit flush
	/*
	TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_OR)                   // 0x07000010
	  [all]   0
	  [2]     CTRL_CC_FLUSHMODE_INH_DC =  1
	*/
	write_scom_for_chiplet(id, 0x07000010, PPC_BIT(2));

	// Clear Scan region type register
	/*
	TP.TCMC01.MCSLOW.SCAN_REGION_TYPE                     // 0x07030005
	  [all]   0
	*/
	write_scom_for_chiplet(id, 0x07030005, 0);

	// Setup all Clock Domains and Clock Types
	/*
	TP.TCMC01.MCSLOW.CLK_REGION                           // 0x07030006
	  [0-1]   CLOCK_CMD =       1     // start
	  [2]     SLAVE_MODE =      0
	  [3]     MASTER_MODE =     0
	  [4-14]  CLOCK_REGION_* =  ~ATTR_PG[4-14]
	  [48]    SEL_THOLD_SL =    1
	  [49]    SEL_THOLD_NSL =   1
	  [50]    SEL_THOLD_ARY =   1
	*/
	//~ scom_and_or_for_chiplet(id, 0x07030006,
	                        //~ ~(PPC_BITMASK(0,14) | PPC_BITMASK(48,50)),	// and
	                        //~ PPC_BIT(1) | PPC_BIT(48) | PPC_BIT(49) | PPC_BIT(50)
	                        //~ | (~ATTR_PG & PPC_BITMASK(4,14)));		// or

	write_scom_for_chiplet(id, 0x07030006, PPC_BIT(1) | PPC_BIT(48) | PPC_BIT(49) | PPC_BIT(50)
	                        | (~ATTR_PG & PPC_BITMASK(4,14)));

	// Poll OPCG done bit to check for completeness
	/*
	timeout(10*100us):
	TP.TCMC01.MCSLOW.CPLT_STAT0                         // 0x07000100
	if (([8] CC_CTRL_OPCG_DONE_DC) == 1) break
	delay(100us)
	*/
	if (!wait_us(10 * 100, read_scom_for_chiplet(id, 0x07000100) & PPC_BIT(8)))
		die("Timeout while waiting for OPCG done bit\n");

	/*
	 * Here Hostboot calculates what is expected clock status, based on previous
	 * values and requested command. It is done by generic functions, but
	 * because we know exactly which clocks were to be started, we can test just
	 * for those.
	 */
	/*
	TP.TCMC01.MCSLOW.CLOCK_STAT_SL                        // 0x07030008
	TP.TCMC01.MCSLOW.CLOCK_STAT_NSL                       // 0x07030009
	TP.TCMC01.MCSLOW.CLOCK_STAT_ARY                       // 0x0703000A
	  assert(([4-14] & ATTR_PG[4-14]) == ATTR_PG[4-14])
	*/
	uint64_t mask = ATTR_PG & PPC_BITMASK(4,14);
	if ((read_scom_for_chiplet(id, 0x07030008) & PPC_BITMASK(4,14)) != mask ||
	    (read_scom_for_chiplet(id, 0x07030009) & PPC_BITMASK(4,14)) != mask ||
	    (read_scom_for_chiplet(id, 0x0703000A) & PPC_BITMASK(4,14)) != mask)
		die("Unexpected clock status\n");
}

static inline void p9_mem_startclocks_fence_setup_function(chiplet_id_t id)
{
	/*
	 * Hostboot does it based on pg_vector. It seems to check for Nest IDs to
	 * which MCs are connected, but I'm not sure if this is the case. I also
	 * don't know if it is possible to have a functional MCBIST for which we
	 * don't want to drop the fence (functional MCBIST with nonfunctional NEST?)
	 */

	/*
	 * if ((MC.ATTR_CHIP_UNIT_POS == 0x07 && pg_vector[5]) ||
	 *    (MC.ATTR_CHIP_UNIT_POS == 0x08 && pg_vector[3]))
	 *{
	 */

	// Drop chiplet fence
	/*
	TP.TPCHIP.NET.PCBSLMC01.NET_CTRL0 (WAND)            // 0x070F0041
	  [all] 1
	  [18]  FENCE_EN =  0
	*/
	write_scom_for_chiplet(id, 0x070F0041, ~PPC_BIT(18));

	/* }*/
}

static void p9_sbe_common_configure_chiplet_FIR(chiplet_id_t id)
{
	// reset pervasive FIR
	/*
	TP.TCMC01.MCSLOW.LOCAL_FIR                            // 0x0704000A
	  [all]   0
	*/
	write_scom_for_chiplet(id, 0x0704000A, 0);

	// configure pervasive FIR action/mask
	/*
	TP.TCMC01.MCSLOW.LOCAL_FIR_ACTION0                    // 0x07040010
	  [all]   0
	TP.TCMC01.MCSLOW.LOCAL_FIR_ACTION1                    // 0x07040011
	  [all]   0
	  [0-3]   0xF
	TP.TCMC01.MCSLOW.LOCAL_FIR_MASK                       // 0x0704000D
	  [all]   0
	  [4-41]  0x3FFFFFFFFF (every bit set)
	*/
	write_scom_for_chiplet(id, 0x07040010, 0);
	write_scom_for_chiplet(id, 0x07040011, PPC_BITMASK(0,3));
	write_scom_for_chiplet(id, 0x0704000D, PPC_BITMASK(4,41));

	// reset XFIR
	/*
	TP.TCMC01.MCSLOW.XFIR                                 // 0x07040000
	  [all]   0
	*/
	write_scom_for_chiplet(id, 0x07040000, 0);

	// configure XFIR mask
	/*
	TP.TCMC01.MCSLOW.FIR_MASK                             // 0x07040002
	  [all]   0
	*/

	/* FIXME: this results in checkstops from IOM23 FIR and IOM23 FIR unstaged.
	 * Temporarily leave these two bits (plus summary bit) masked. */
	// write_scom_for_chiplet(id, 0x07040002, 0);
	write_scom_for_chiplet(id, 0x07040002, PPC_BIT(0) | PPC_BIT(14) | PPC_BIT(16));

	printk(BIOS_EMERG, "Please FIXME: 0x07040002 = %#16.16llx\n",
	       read_scom_for_chiplet(id, 0x07040002));
}

/*
 * 13.6 mem_startclocks: Start clocks on MBA/MCAs
 *
 * a) p9_mem_startclocks.C (proc chip)
 *    - This step is a no-op on cumulus
 *    - This step is a no-op if memory is running in synchronous mode since the
 *      MCAs are using the nest PLL, HWP detect and exits
 *    - Drop fences and tholds on MBA/MCAs to start the functional clocks
 */
void istep_13_6(void)
{
	printk(BIOS_EMERG, "starting istep 13.6\n");
	int i;

	report_istep(13,6);

	/* Assuming MC doesn't run in sync mode with Fabric, otherwise this is no-op */

	for (i = 0; i < MCS_PER_PROC; i++) {
		if (!mem_data.mcs[i].functional)
			continue;

		// Call p9_mem_startclocks_cplt_ctrl_action_function for Mc chiplets
		p9_mem_startclocks_cplt_ctrl_action_function(mcs_ids[i]);

		// Call module align chiplets for Mc chiplets
		p9_sbe_common_align_chiplets(mcs_ids[i]);

		// Call module clock start stop for MC01, MC23
		p9_sbe_common_clock_start_stop(mcs_ids[i]);

		// Call p9_mem_startclocks_fence_setup_function for Mc chiplets
		p9_mem_startclocks_fence_setup_function(mcs_ids[i]);

		// Clear flush_inhibit to go in to flush mode
		/*
		TP.TCMC01.MCSLOW.CPLT_CTRL0 (WO_CLEAR)                // 0x07000020
		  [all]   0
		  [2]     CTRL_CC_FLUSHMODE_INH_DC =  1
		*/
		write_scom_for_chiplet(mcs_ids[i], 0x07000020, PPC_BIT(2));

		// Call p9_sbe_common_configure_chiplet_FIR for MC chiplets
		p9_sbe_common_configure_chiplet_FIR(mcs_ids[i]);

		// Reset FBC chiplet configuration
		/*
		TP.TCMC01.MCSLOW.CPLT_CONF0                             // 0x07000008
		  [48-51] TC_UNIT_GROUP_ID_DC = ATTR_PROC_FABRIC_GROUP_ID   // Where do these come from?
		  [52-54] TC_UNIT_CHIP_ID_DC =  ATTR_PROC_FABRIC_CHIP_ID
		  [56-60] TC_UNIT_SYS_ID_DC =   ATTR_PROC_FABRIC_SYSTEM_ID  // 0 in talos.xml
		*/
		/*
		 * Take 0 for all values - assuming ATTR_PROC_FABRIC_GROUP_ID is
		 * ATTR_FABRIC_GROUP_ID of parent PROC (same for CHIP_ID). Only
		 * SYSTEM_ID is present in talos.xml with full name.
		 */
		scom_and_for_chiplet(mcs_ids[i], 0x07000008,
		                     ~(PPC_BITMASK(48,54) | PPC_BITMASK(56,60)));

		// Add to Multicast Group
		/* Avoid setting if register is already set, i.e. [3-5] != 7 */
		/*
		TP.TPCHIP.NET.PCBSLMC01.MULTICAST_GROUP_1             // 0x070F0001
		  [3-5]   MULTICAST1_GROUP: if 7 then set to 0
		  [16-23] (not described):  if [3-5] == 7 then set to 0x1C    // No clue why Hostboot modifies these bits
		TP.TPCHIP.NET.PCBSLMC01.MULTICAST_GROUP_2             // 0x070F0002
		  [3-5]   MULTICAST1_GROUP: if 7 then set to 2
		  [16-23] (not described):  if [3-5] == 7 then set to 0x1C
		*/
		if ((read_scom_for_chiplet(mcs_ids[i], 0x070F0001) & PPC_BITMASK(3,5))
		    == PPC_BITMASK(3,5))
			scom_and_or_for_chiplet(mcs_ids[i], 0x070F0001,
			                        ~(PPC_BITMASK(3,5) | PPC_BITMASK(16,23)),
			                        PPC_BITMASK(19,21));

		if ((read_scom_for_chiplet(mcs_ids[i], 0x070F0002) & PPC_BITMASK(3,5))
		    == PPC_BITMASK(3,5))
			scom_and_or_for_chiplet(mcs_ids[i], 0x070F0002,
			                        ~(PPC_BITMASK(3,5) | PPC_BITMASK(16,23)),
			                        PPC_BIT(4) | PPC_BITMASK(19,21));
	}

	printk(BIOS_EMERG, "ending istep 13.6\n");
}
