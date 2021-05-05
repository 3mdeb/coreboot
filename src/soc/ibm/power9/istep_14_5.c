/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <console/console.h>

/*
 * Reset memory controller configuration written by SBE.
 * Close the MCS acker before enabling the real memory bars.
 *
 * Some undocumented registers, again. The registers use a stride I haven't seen
 * before (0x80), not sure if those are MCSs (including those not present on P9),
 * magic MCAs or something totally different. Hostboot writes to all possible
 * registers, regardless of how many ports/slots are populated.
 *
 * All register and field names come from code and comments only, except for the
 * first one.
 */
static void revert_mc_hb_dcbz_config(void)
{
	int mcs_i, i;
	uint64_t val;
	const uint64_t mul = 0x80;

	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		chiplet_id_t nest = mcs_i == 0 ? N3_CHIPLET_ID : N1_CHIPLET_ID;

		/*
		 * Bit for MCS2/3 is documented, but for MCS0/1 it is "unused". Use what
		 * Hostboot uses - bit 10 for MCS0/1 and bit 9 for MCS2/3.
		 *
		 * FIXME: numbering again. I've been assuming that MCSs are numbered 0
		 * for MCS0/1 and 1 for MCS2/3, and unpopulated are just skipped. This
		 * may blow up when more DIMMs are connected.
		 */
		val = read_scom_for_chiplet(nest, 0x05000001); // TP.TCNx.Nx.CPLT_CTRL1, x = {1,3}
		if ((mcs_i == 0 && val & PPC_BIT(10)) ||
		    (mcs_i == 1 && val & PPC_BIT(9)))
			continue;

		for (i = 0; i < 2; i++) {
			/* MCFGP -- mark BAR invalid & reset grouping configuration fields
			MCS_n_MCFGP    // undocumented, 0x0501080A, 0x0501088A, 0x0301080A, 0x0301088A for MCS{0-3}
				[0]     VALID =                                 0
				[1-4]   MC_CHANNELS_PER_GROUP =                 0
				[5-7]   CHANNEL_0_GROUP_MEMBER_IDENTIFICATION = 0   // CHANNEL_1_GROUP_MEMBER_IDENTIFICATION not cleared?
				[13-23] GROUP_SIZE =                            0
			*/
			scom_and_or_for_chiplet(nest, 0x0501080A + i * mul,
			                        ~(PPC_BITMASK(0, 7) | PPC_BITMASK(13, 23)),
			                        0);

			/* MCMODE1 -- enable speculation, cmd bypass, fp command bypass
			MCS_n_MCMODE1  // undocumented, 0x05010812, 0x05010892, 0x03010812, 0x03010892
				[32]    DISABLE_ALL_SPEC_OPS =      0
				[33-51] DISABLE_SPEC_OP =           0x40    // bit 45 (called DCBF_BIT in code) set because of HW414958
				[54-60] DISABLE_COMMAND_BYPASS =    0
				[61]    DISABLE_FP_COMMAND_BYPASS = 0
			*/
			scom_and_or_for_chiplet(nest, 0x05010812 + i * mul,
			                        ~(PPC_BITMASK(32, 51) | PPC_BITMASK(54, 61)),
			                        PPC_SHIFT(0x40, 51));

			/* MCS_MCPERF1 -- enable fast path
			MCS_n_MCPERF1  // undocumented, 0x05010810, 0x05010890, 0x03010810, 0x03010890
				[0]     DISABLE_FASTPATH =  0
			*/
			scom_and_or_for_chiplet(nest, 0x05010810 + i * mul,
			                        ~PPC_BIT(0),
			                        0);

			/* Re-mask MCFIR. We want to ensure all MCSs are masked until the
			 * BARs are opened later during IPL.
			MCS_n_MCFIRMASK_OR  // undocumented, 0x05010805, 0x05010885, 0x03010805, 0x03010885
				[all]   1
			*/
			write_scom_for_chiplet(nest, 0x05010805 + i * mul, ~0);
		}
	}
}

/*
 * 14.5 proc_setup_bars: Setup Memory BARs
 *
 * a) p9_mss_setup_bars.C (proc chip) -- Nimbus
 * b) p9c_mss_setup_bars.C (proc chip) -- Cumulus
 *    - Same HWP interface for both Nimbus and Cumulus, input target is
 *      TARGET_TYPE_PROC_CHIP; HWP is to figure out if target is a Nimbus (MCS)
 *      or Cumulus (MI) internally.
 *    - Prior to setting the memory bars on each processor chip, this procedure
 *      needs to set the centaur security protection bit
 *      - TCM_CHIP_PROTECTION_EN_DC is SCOM Addr 0x03030000
 *      - TCN_CHIP_PROTECTION_EN_DC is SCOM Addr 0x02030000
 *      - Both must be set to protect Nest and Mem domains
 *    - Based on system memory map
 *      - Each MCS has its mirroring and non mirrored BARs
 *      - Set the correct checkerboard configs. Note that chip flushes to
 *        checkerboard
 *      - need to disable memory bar on slave otherwise base flush values will
 *        ack all memory accesses
 * c) p9_setup_bars.C
 *    - Sets up Powerbus/MCD, L3 BARs on running core
 *      - Other cores are setup via winkle images
 *    - Setup dSMP and PCIe Bars
 *      - Setup PCIe outbound BARS (doing stores/loads from host core)
 *        - Addresses that PCIE responds to on powerbus (PCI init 1-7)
 *      - Informing PCIe of the memory map (inbound)
 *        - PCI Init 8-15
 *    - Set up Powerbus Epsilon settings
 *      - Code is still running out of L3 cache
 *      - Use this procedure to setup runtime epsilon values
 *      - Must be done before memory is viable
 */
void istep_14_5(void)
{
	int mcs_i;
	printk(BIOS_EMERG, "starting istep 14.5\n");
	report_istep(14,5);

	/* Start MCS reset */
	revert_mc_hb_dcbz_config();

	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		if (!mem_data.mcs[mcs_i].functional)
			continue;

		write_scom_for_chiplet(5, 0x05010807, 0x8080000000000000);
		write_scom_for_chiplet(5, 0x05010804, 0x137FFFFFFFFFFFFF);
		write_scom_for_chiplet(5, 0x0501080A, 0);
		write_scom_for_chiplet(5, 0x0501080C, 0x8000010000000000);
		write_scom_for_chiplet(5, 0x0501080B, 0);
		write_scom_for_chiplet(5, 0x0501080D, 0);

		write_scom_for_chiplet(3, 0x03011403, ~0);
		write_scom_for_chiplet(3, 0x03011003, ~0);
	}

	printk(BIOS_EMERG, "ending istep 14.5\n");
}
