/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <timer.h>
#include <endian.h>

static inline uint64_t reverse_bits(uint64_t x)
{
	x = swab64(x);		// reverse bytes
	x = (x & 0xF0F0F0F0F0F0F0F0) >> 4 |
	    (x & 0x0F0F0F0F0F0F0F0F) << 4;	// reverse nibbles in bytes
	x = (x & 0x1111111111111111) << 3 |
	    (x & 0x2222222222222222) << 1 |
	    (x & 0x4444444444444444) >> 1 |
	    (x & 0x8888888888888888) >> 3;	// reverse bits in nibbles

	return x;
}

/* 32 total, but last one is reserved for ccs_execute() */
#define MAX_CCS_INSTR	31

static unsigned instr;
static uint64_t total_cycles;

/* TODO: 4R, CID? */
void ccs_add_instruction(chiplet_id_t id, mrs_cmd_t mrs, uint8_t csn,
                         uint8_t cke, uint16_t idles)
{
	/*
	 * CSS_INST_ARR0_n layout (bits from MRS):
	 * [0-13]  A0-A13
	 * [14]    A17
	 * [15]    BG1
	 * [17-18] BA0-1
	 * [19]    BG0
	 * [21]    A16
	 * [22]    A15
	 * [23]    A14
	 */
	uint64_t mrs64 = (reverse_bits(mrs) & PPC_BITMASK(0, 13)) | /* A0-A13 */
	                 PPC_SHIFT(mrs & (1<<14), 23 + 14) |        /* A14 */
	                 PPC_SHIFT(mrs & (1<<15), 22 + 15) |        /* A15 */
	                 PPC_SHIFT(mrs & (1<<16), 21 + 16) |        /* A16 */
	                 PPC_SHIFT(mrs & (1<<17), 14 + 17) |        /* A17 */
	                 PPC_SHIFT(mrs & (1<<20), 17 + 20) |        /* BA0 */
	                 PPC_SHIFT(mrs & (1<<21), 18 + 21) |        /* BA1 */
	                 PPC_SHIFT(mrs & (1<<22), 19 + 22) |        /* BG0 */
	                 PPC_SHIFT(mrs & (1<<23), 15 + 23);         /* BA1 */

	/* MC01.MCBIST.CCS.CCS_INST_ARR0_n
	      [all]   0
	      // "ACT is high. It's a no-care in the spec but it seems to raise
	      // questions when people look at the trace, so lets set it high."
	      [20]    CCS_INST_ARR0_00_CCS_DDR_ACTN =     1
	      // "CKE is high Note: P8 set all 4 of these high - not sure if that's
	      // correct. BRS"
	      [24-27] CCS_INST_ARR0_00_CCS_DDR_CKE =      cke
	      [32-33] CCS_INST_ARR0_00_CCS_DDR_CSN_0_1 =  csn[0:1]
	      [36-37] CCS_INST_ARR0_00_CCS_DDR_CSN_2_3 =  csn[2:3]
	*/
	write_scom_for_chiplet(id, 0x07012315 + instr,
	                       mrs64 | PPC_BIT(20) | PPC_SHIFT(cke & 0xF, 27) |
	                       PPC_SHIFT((csn >> 2) & 3, 33) | PPC_SHIFT(csn & 3, 37));

	/* MC01.MCBIST.CCS.CCS_INST_ARR1_n
	      [all]   0
	      [0-15]  CCS_INST_ARR1_00_IDLES =    idles
	      [59-63] CCS_INST_ARR1_00_GOTO_CMD = instr + 1
	*/
	write_scom_for_chiplet(id, 0x07012335 + instr,
	                       PPC_SHIFT(idles, 15) | PPC_SHIFT(instr + 1, 63));

	/*
	 * For the last instruction in the stream we could decrease it by one (final
	 * DES added in ccs_execute()), but subtracting it would take longer than
	 * that one cycle, so leave it.
	 */
	total_cycles += idles;
	instr++;

	if (instr >= MAX_CCS_INSTR) {
		/* Maybe call ccs_execute() here? Would need mca_i... */
		die("CCS instructions overflowed\n");
	}
}

void ccs_execute(chiplet_id_t id, int mca_i)
{
	long time;
	/* Is it always as described below (CKE, CSN) or is it a copy of last instr? */
	/* Final DES - CCS does not wait for IDLES for the last command before
	 * clearing IP (in progress) bit, so we must use one separate DES
	 * instruction at the end.
	MC01.MCBIST.CCS.CCS_INST_ARR0_n
	      [all]   0
	      [20]    CCS_INST_ARR0_00_CCS_DDR_ACTN =     1
	      [24-27] CCS_INST_ARR0_00_CCS_DDR_CKE =      0xf
	      [32-33] CCS_INST_ARR0_00_CCS_DDR_CSN_0_1 =  3
	      [36-37] CCS_INST_ARR0_00_CCS_DDR_CSN_2_3 =  3
	MC01.MCBIST.CCS.CCS_INST_ARR1_n
	      [all]   0
	      [58]    CCS_INST_ARR1_00_CCS_END = 1
	*/
	write_scom_for_chiplet(id, 0x07012315 + instr, PPC_BIT(20) | PPC_SHIFT(0xF, 27) |
	                       PPC_SHIFT(3, 33) | PPC_SHIFT(3, 37));
	write_scom_for_chiplet(id, 0x07012335 + instr, PPC_BIT(58));

	/* Select ports
	MC01.MCBIST.MBA_SCOMFIR.MCB_CNTLQ
	      // Broadcast mode is not supported, set only one bit at a time
	      [2-5]   MCB_CNTLQ_MCBCNTL_PORT_SEL = bitmap with MCA index
	*/
	write_scom_for_chiplet(id, 0x070123DB, PPC_BIT(2 + mca_i));

	/* Lets go
	MC01.MCBIST.MBA_SCOMFIR.CCS_CNTLQ
	      [all] 0
	      [0]   CCS_CNTLQ_CCS_START = 1
	*/
	write_scom_for_chiplet(id, 0x070123A5, PPC_BIT(0));

	/* With microsecond resolution we are probably wasting a lot of time here. */
	delay_nck(total_cycles);

	/* timeout(50*10ns):
	  if MC01.MCBIST.MBA_SCOMFIR.CCS_STATQ[0] (CCS_STATQ_CCS_IP) != 1: break
	  delay(10ns)
	if MC01.MCBIST.MBA_SCOMFIR.CCS_STATQ != 0x40..00: report failure  // only [1] set, others 0
	*/
	time = wait_us(1, !(read_scom_for_chiplet(id, 0x070123A6) & PPC_BIT(0)));

	if (!time)
		die("CCS execution timeout\n");

	if (read_scom_for_chiplet(id, 0x070123A6) != PPC_BIT(1))
		die("(%#16.16llx) CCS execution error\n", read_scom_for_chiplet(id, 0x070123A6));

	instr = 0;
	total_cycles = 0;

	/* LRDIMM only */
	// cleanup_from_execute();
}

/*
 * Procedure for sending MRS through CCS
 *
 * We need to remember about two things here:
 * - RDIMM has A-side and B-side, some address bits are inverted for B-side;
 *   side is selected by DBG1 (when mirroring is enabled DBG0 is used for odd
 *   ranks to select side, instead of DBG1)
 * - odd ranks may or may not have mirrored lines, depending on SPD[136].
 *
 * Because of those two reasons we cannot simply repeat MRS data for all sides
 * and ranks, we have to do some juggling instead. Inverting is easy, we just
 * have to XOR with appropriate mask (special case for A17, it is not inverted
 * if it isn't used). Mirroring will require manual bit manipulations.
 *
 * There are no signals that are mirrored but not inverted, which means that
 * the order of those operations doesn't matter.
 */
/* TODO: add support for A17. For now it is blocked in initial SPD parsing. */
void ccs_add_mrs(chiplet_id_t id, mrs_cmd_t mrs, int ranks, int mirror,
                 uint16_t idles)
{
	if (ranks != 1 && ranks != 2)
		die("Too many ranks specified for MRS command\n");

	/* Rank 0, side A */
	/*
	 * "Not sure if we can get tricky here and only delay after the b-side MR.
	 * The question is whether the delay is needed/assumed by the register or is
	 * purely a DRAM mandated delay. We know we can't go wrong having both
	 * delays but if we can ever confirm that we only need one we can fix this.
	 * BRS"
	 */
	ccs_add_instruction(id, mrs, 0x7, 0xF, idles);

	/* Rank 0, side B - invert A3-A9, A11, A13, A17 (TODO), BA0-1, BG0-1 */
	mrs ^= 0xF02BF8;	/* This also changes BG1 to 1, which selects B-side */
	ccs_add_instruction(id, mrs, 0x7, 0xF, idles);

	if (ranks == 1)
		return;

	/* Rank 1, side A - revert MRS, mirror if needed */
	mrs ^= 0xF02BF8;
	if (mirror)
		mrs = ddr4_mrs_mirror_pins(mrs);

	ccs_add_instruction(id, mrs, 0xB, 0xF, idles);

	/* Rank 1, side B - MRS is already mirrored, just invert it */
	mrs ^= 0xF02BF8;
	ccs_add_instruction(id, mrs, 0xB, 0xF, idles);
}
