/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <console/console.h>
#include <cpu/power/scom.h>
#include <cpu/power/scom_registers.h>
#include <delay.h>

#define MCS_PER_PROC		2
#define DIMMS_PER_MCS		4

/*
 * 13.4 mem_pll_setup: Setup PLL for MBAs
 *
 * a) p9_mem_pll_setup.C (proc chip)
 *    - This step is a no-op on cumulus
 *    - This step is a no-op if memory is running in synchronous mode since the
 *      MCAs are using the nest PLL, HWP detect and exits
 *    - MCA PLL setup
 *      - Moved PLL out of bypass (just DDR)
 *    - Performs PLL checking
 */
void istep_13_4(dimm_attr dimms[CONFIG_DIMM_MAX])
{
	printk(BIOS_EMERG, "starting istep 13.4\n");
	int i;
	bool mcs_functional[MCS_PER_PROC] = {false, false};
	chiplet_id_t mcs_ids[MCS_PER_PROC] = {MC01_CHIPLET_ID, MC23_CHIPLET_ID};

	report_istep(13,4);

	/* We already did it in 13.2. Maybe this should be done on a higher level? */
	for (i = 0; i < CONFIG_DIMM_MAX; i++) {
		if (dimms[i].dram_type == SPD_MEMORY_TYPE_DDR4_SDRAM &&
		    dimms[i].dimm_type == SPD_DIMM_TYPE_RDIMM &&
		    dimms[i].ecc_extension == true) {
			mcs_functional[i/DIMMS_PER_MCS] = true;
			/* Skip to next MCS */
			i = (i/DIMMS_PER_MCS + 1) * DIMMS_PER_MCS;
		}
	}

	/* Assuming MC doesn't run in sync mode with Fabric, otherwise this is no-op */

	for (i = 0; i < MCS_PER_PROC; i++) {
		if (!mcs_functional[i])
			continue;

		// Drop PLDY bypass of Progdelay logic
		/*
		TP.TPCHIP.NET.PCBSLMC01.NET_CTRL1 (WAND)                  // 0x070F0045
		  [all] 1
		  [2]   CLK_PDLY_BYPASS_EN =  0
		*/
		write_scom_for_chiplet(mcs_ids[i], 0x070F0045, ~PPC_BIT(2));

		// Drop DCC bypass of DCC logic
		/*
		TP.TPCHIP.NET.PCBSLMC01.NET_CTRL1 (WAND)                  // 0x070F0045
		  [all] 1
		  [1]   CLK_DCC_BYPASS_EN =   0
		*/
		write_scom_for_chiplet(mcs_ids[i], 0x070F0045, ~PPC_BIT(1));

		// ATTR_NEST_MEM_X_O_PCI_BYPASS is set to 0 in talos.xml.
		// > if (ATTR_NEST_MEM_X_O_PCI_BYPASS == 0)

		// Drop PLL test enable
		/*
		TP.TPCHIP.NET.PCBSLMC01.NET_CTRL0 (WAND)                  // 0x070F0041
		  [all] 1
		  [3]   PLL_TEST_EN = 0
		*/
		write_scom_for_chiplet(mcs_ids[i], 0x070F0041, ~PPC_BIT(3));

		// Drop PLL reset
		/*
		TP.TPCHIP.NET.PCBSLMC01.NET_CTRL0 (WAND)                  // 0x070F0041
		  [all] 1
		  [4]   PLL_RESET =   0
		*/
		write_scom_for_chiplet(mcs_ids[i], 0x070F0041, ~PPC_BIT(4));

		/*
		 * TODO: This is how Hosboot does it, maybe it would be better to use
		 * wait_ms and a separate loop to have only one timeout. On the other
		 * hand, it is possible that MCS will stop responding to SCOM accesses
		 * after PLL reset so we wouldn't be able to read the status.
		 */
		mdelay(5);

		// Check PLL lock
		/*
		TP.TPCHIP.NET.PCBSLMC01.PLL_LOCK_REG                      // 0x070F0019
		assert([0] (reserved) == 1)
		*/
		if (!(read_scom_for_chiplet(mcs_ids[i], 0x070F0019) & PPC_BIT(0)))
			die("MCS%d PLL not locked\n", i);

		// Drop PLL Bypass
		/*
		TP.TPCHIP.NET.PCBSLMC01.NET_CTRL0 (WAND)                  // 0x070F0041
		  [all] 1
		  [5]   PLL_BYPASS =  0
		*/
		write_scom_for_chiplet(mcs_ids[i], 0x070F0041, ~PPC_BIT(5));

		// Set scan ratio to 4:1
		/*
		TP.TCMC01.MCSLOW.OPCG_ALIGN                               // 0x07030001
		  [47-51] SCAN_RATIO =  3           // 4:1
		*/
		scom_and_or_for_chiplet(mcs_ids[i], 0x07030001, ~PPC_BITMASK(47,51),
		                        PPC_BIT(50) | PPC_BIT(51));

		// > end if

		// Reset PCB Slave error register
		/*
		TP.TPCHIP.NET.PCBSLMC01.ERROR_REG                         // 0x070F001F
		  [all] 1                 // Write 1 to clear
		*/
		write_scom_for_chiplet(mcs_ids[i], 0x070F001F, ~0);

		// Unmask PLL unlock error in PCB slave
		/*
		TP.TPCHIP.NET.PCBSLMC01.SLAVE_CONFIG_REG                  // 0x070F001E
		  [12]  (part of) ERROR_MASK =  0
		*/
		scom_and_for_chiplet(mcs_ids[i], 0x070F001E, ~PPC_BIT(12));
	}

	printk(BIOS_EMERG, "ending istep 13.4\n");
}
