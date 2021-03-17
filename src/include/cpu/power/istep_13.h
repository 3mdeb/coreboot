/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/dram/ddr4.h>
#include <spd_bin.h>
#include <delay.h>
#include <cpu/power/scom.h>

#define MCS_PER_PROC		2
#define MCA_PER_MCS		2
#define MCA_PER_PROC		(MCA_PER_MCS * MCS_PER_PROC)
#define DIMMS_PER_MCA		2
#define DIMMS_PER_MCS		(DIMMS_PER_MCA * MCA_PER_MCS)

/* These should be in one of the SPD headers. */
#define WIDTH_x4		0
#define WIDTH_x8		1

#define DENSITY_256Mb		0
#define DENSITY_512Mb		1
#define DENSITY_1Gb		2
#define DENSITY_2Gb		3
#define DENSITY_4Gb		4
#define DENSITY_8Gb		5
#define DENSITY_16Gb		6
#define DENSITY_32Gb		7

#define PSEC_PER_NSEC		1000
#define PSEC_PER_USEC		1000000

typedef struct {
	bool present;
	uint8_t mranks;
	uint8_t log_ranks;
	uint8_t width;
	uint8_t density;
	uint8_t *spd;
} rdimm_data_t;

typedef struct {
	bool functional;
	rdimm_data_t dimm[DIMMS_PER_MCA];

	/*
	 * The following fields are read and/or calculated from SPD obtained
	 * from DIMMs, but they are here because we can only set them per
	 * MCA/port/channel and not per DIMM. All units are clock cycles,
	 * absolute time values are rarely used.
	 */
	uint16_t nfaw;
	uint16_t nras;
	uint16_t nrfc;
	uint16_t nrfc_dlr;	// nRFC for Different Logical Rank (3DS only)
	uint8_t cl;
	uint8_t nccd_l;
	uint8_t nwtr_s;
	uint8_t nwtr_l;
	uint8_t nrcd;
	uint8_t nrp;
	uint8_t nwr;
	uint8_t nrrd_s;
	uint8_t nrrd_l;
} mca_data_t;

typedef struct {
	bool functional;
	mca_data_t mca[MCA_PER_MCS];
} mcs_data_t;

typedef struct {
	/* Do we need 'bool functional' here as well? */
	mcs_data_t mcs[MCS_PER_PROC];

	/*
	 * Unclear whether we can have different speeds between MCSs.
	 * Documentation says we can, but ring ID in 13.3 is sent per MCBIST.
	 * ATTR_MSS_FREQ is defined for SYSTEM target type, implying only one
	 * speed for whole platform.
	 *
	 * FIXME: maybe these should be in mcs_data_t and 13.3 should send
	 * a second Ring ID for the second MCS. How to test it?
	 */
	uint16_t speed;	// MT/s
	/*
	 * These depend just on memory frequency (and specification), and even
	 * though they describe DRAM/DIMM/MCA settings, there is no need to have
	 * multiple copies of identical data.
	 */
	uint16_t nrefi;	// 7.8 us in normal temperature range (0-85 deg Celsius)
	uint8_t cwl;
	uint8_t nrtp;	// max(4 nCK, 7.5 ns) = 7.5 ns for every supported speed
} mcbist_data_t;

extern mcbist_data_t mem_data;

/*
 * All time conversion functions assume that both MCSs have the same frequency.
 * Change it if proven otherwise by adding a second argument - memory speed or
 * MCS index.
 *
 * These functions should not be used before setting mem_data.speed to a valid
 * non-0 value.
 */
static inline uint64_t tck_in_ps(void)
{
	/*
	 * Speed is in MT/s, we need to divide it by 2 to get MHz.
	 * tCK(avg) should be rounded down to the next valid speed bin, which
	 * corresponds to value obtained by using standardized MT/s values.
	 */
	return 1000000 / (mem_data.speed / 2);
}

static inline uint64_t ps_to_nck(uint64_t ps)
{
	/* Algorithm taken from JEDEC Standard No. 21-C */
	return ((ps * 1000 / tck_in_ps()) + 974) / 1000;
}

static inline uint64_t mtb_ftb_to_nck(uint64_t mtb, int8_t ftb)
{
	/* ftb is signed (always byte?) */
	return ps_to_nck(mtb * 125 + ftb);
}

static inline uint64_t ns_to_nck(uint64_t ns)
{
	return ps_to_nck(ns * PSEC_PER_NSEC);
}

static inline uint64_t nck_to_ps(uint64_t nck)
{
	return nck * tck_in_ps();
}

/*
 * To be used in delays, so always round up.
 *
 * Microsecond is the best precision exposed by coreboot API. tCK is somewhere
 * around 1 ns, so most smaller delays will be rounded up to 1 us. For better
 * resolution we would have to read TBR (Time Base Register) directly.
 */
static inline uint64_t nck_to_us(uint64_t nck)
{
	return (nck_to_ps(nck) + PSEC_PER_USEC - 1) / PSEC_PER_USEC;
}

static inline void delay_nck(uint64_t nck)
{
	udelay(nck_to_us(nck));
}

#define PPC_SHIFT(val, lsb)	(((uint64_t)(val)) << (63 - lsb))

/* TODO: discover how MCAs are numbered (0,1,2,3? 0,1,6,7? 0,1,4,5?) */
/* TODO: consider non-RMW variants */
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

static inline uint64_t mca_read(chiplet_id_t mcs, int mca, uint64_t scom)
{
	/* Indirect registers have different stride than the direct ones. */
	unsigned mul = (scom & PPC_BIT(0)) ? 0x400 : 0x40;
	return read_scom_for_chiplet(mcs, scom + mca * mul);
}

static inline uint64_t dp_mca_read(chiplet_id_t mcs, int dp, int mca, uint64_t scom)
{
	return mca_read(mcs, mca, scom + dp * 0x40000000000);
}

void istep_13_2(void);
void istep_13_3(void);
void istep_13_4(void);
void istep_13_6(void);
void istep_13_8(void);	// TODO: takes epsilon values from 8.6 and MSS data from 7.4
