/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <console/console.h>
#include <cpu/power/scom.h>
#include <cpu/power/scom_registers.h>
#include <timer.h>

#define RING_ID_1866	0x6B
#define RING_ID_2133	0x6C
#define RING_ID_2400	0x6D
#define RING_ID_2666	0x6E

/*
 * 13.3 mem_pll_initf: PLL Initfile for MBAs
 *
 * a) p9_mem_pll_initf.C (proc chip)
 *    - This step is a no-op on cumulus
 *    - This step is a no-op if memory is running in synchronous mode since the
 *      MCAs are using the nest PLL, HWP detect and exits
 *    - MCA PLL setup
 *      - Note that Hostboot doesn't support twiddling bits, Looks up which
 *        "bucket" (ring) to use from attributes set during mss_freq
 *      - Then request the SBE to scan ringId with setPulse
 *        - SBE needs to support 5 RS4 images
 *        - Data is stored as a ring image in the SBE that is frequency specific
 *        - 5 different frequencies (1866, 2133, 2400, 2667, EXP)
 */
void istep_13_3(struct spd_block *blk)
{
	printk(BIOS_EMERG, "starting istep 13.3\n");
	int dimm;
	int tckmin = 0x06;		// Platform limit
	uint64_t ring_id;

	report_istep(13,3);

	/* Assuming MC doesn't run in sync mode with Fabric, otherwise this is no-op */

	/*
	 * We need to find the highest common (for all DIMMs and the platform)
	 * supported frequency, meaning we need to compare minimum clock cycle times
	 * and choose the highest value. For the range supported by the platform we
	 * can check MTB only.
	 *
	 * TODO: maximum for 2 DIMMs on one port (channel) is 2400 MT/s, this loop
	 * doesn't check for that.
	 */
	for (dimm = 0; dimm < CONFIG_DIMM_MAX; dimm++) {
		if (blk->spd_array[dimm] != NULL && blk->spd_array[dimm][18] > tckmin)
			tckmin = blk->spd_array[dimm][18];
	}

	switch (tckmin) {
		case 0x06:
			ring_id = RING_ID_2666;
			break;
		case 0x07:
			ring_id = RING_ID_2400;
			break;
		case 0x08:
			ring_id = RING_ID_2133;
			break;
		case 0x09:
			ring_id = RING_ID_1866;
			break;
		default:
			die("Unsupported tCKmin: %d ps (+/- 125)\n", tckmin * 125);
	}

	/*
	 * This is the only place where Hostboot does `putRing()` on Nimbus, but
	 * because Hostboot tries to be as generic as possible, there are many tests
	 * and safeties in place. We do not have to worry about another threads or
	 * out of order command/response pair. Just fill a buffer, send it and make
	 * sure the receiver (SBE) gets it. If you still want to know the details,
	 * start digging here: https://github.com/open-power/hostboot/blob/master/src/usr/scan/scandd.C#L169
	 */
	// TP.TPCHIP.PIB.PSU.PSU_SBE_DOORBELL_REG
	if (read_scom(0x000D0060) & PPC_BIT(0))
		die("MBOX to SBE busy, this should not happen\n");

	/* https://github.com/open-power/hostboot/blob/master/src/include/usr/sbeio/sbe_psudd.H#L418 */
	// TP.TPCHIP.PIB.PSU.PSU_HOST_SBE_MBOX0_REG
	/* REQUIRE_RESPONSE, PSU_PUT_RING_FROM_IMAGE_CMD, CMD_CONTROL_PUTRING */
	write_scom(0x000D0050, 0x000001000000D301);

	// TP.TPCHIP.PIB.PSU.PSU_HOST_SBE_MBOX0_REG
	/* TARGET_TYPE_PERV, chiplet ID = 0x07, ring ID, RING_MODE_SET_PULSE_NSL */
	write_scom(0x000D0051, 0x0002000700000004 | ring_id << 16);

	// Ring the host->SBE doorbell
	// TP.TPCHIP.PIB.PSU.PSU_SBE_DOORBELL_REG_OR
	write_scom(0x000D0062, PPC_BIT(0));

	// Wait for response
	/*
	 * Hostboot registers an interrupt handler in a thread that is demonized. We
	 * do not want nor need to implement a whole OS just for this purpose, we
	 * can just busy-wait here, there isn't anything better to do anyway.
	 *
	 * The original timeout is 90 seconds, but that seems like eternity. After
	 * thorough testing we probably should trim it.
	 */
	long time;

	// TP.TPCHIP.PIB.PSU.PSU_HOST_DOORBELL_REG
	time = wait_ms(90 * MSECS_PER_SEC, read_scom(0x000D0063) & PPC_BIT(0));

	if (!time)
		die("Timed out while waiting for SBE response\n");

	printk(BIOS_EMERG, "putRing took %ld ms\n", time);

	// Clear SBE->host doorbell
	// TP.TPCHIP.PIB.PSU.PSU_HOST_DOORBELL_REG_AND
	write_scom(0x000D0064, ~PPC_BIT(0));

	printk(BIOS_EMERG, "ending istep 13.3\n");
}
