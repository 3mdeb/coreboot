/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/dram/ddr4.h>
#include <spd_bin.h>

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
static inline uint64_t ps_to_nck(uint64_t ps)
{
	/*
	 * Speed is in MT/s, we need to divide it by 2 to get MHz.
	 * tCK(avg) should be rounded down to the next valid speed bin, which
	 * corresponds to value obtained by using standardized MT/s values.
	 */
	uint64_t tck_in_ps = 1000000 / (mem_data.speed / 2);

	/* Algorithm taken from JEDEC Standard No. 21-C */
	return ((ps * 1000 / tck_in_ps) + 974) / 1000;
}

static inline uint64_t mtb_ftb_to_nck(uint64_t mtb, int8_t ftb)
{
	/* ftb is signed (always byte?) */
	return ps_to_nck(mtb * 125 + ftb);
}

static inline uint64_t ns_to_nck(uint64_t ns)
{
	return ps_to_nck(ns * 1000);
}

void istep_13_2(void);
void istep_13_3(void);
void istep_13_4(void);
void istep_13_6(void);
void istep_13_8(void);	// TODO: takes epsilon values from 8.6 and MSS data from 7.4
