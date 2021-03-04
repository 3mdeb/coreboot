/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/dram/ddr4.h>
#include <spd_bin.h>

#define MCS_PER_PROC		2
#define MCA_PER_MCS		2
#define MCA_PER_PROC		(MCA_PER_MCS * MCS_PER_PROC)
#define DIMMS_PER_MCA		2
#define DIMMS_PER_MCS		(DIMMS_PER_MCA * MCA_PER_MCS)

typedef struct {
	bool present;
	uint8_t mranks;
	uint8_t log_ranks;
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
	uint8_t cl;
	uint8_t nccd_l;
	uint8_t nwtr_s;
	uint8_t nwtr_l;
	uint16_t nfaw;
	uint8_t nrcd;
	uint8_t nrp;
	uint16_t nras;
	uint8_t nwr;
	uint8_t nrrd_s;
	uint8_t nrrd_l;
	uint16_t nrfc;
	uint16_t nrfc_dlr;	// nRFC for Different Logical Rank (3DS only)
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

void istep_13_2(void);
void istep_13_3(void);
void istep_13_4(void);
void istep_13_6(void);
void istep_13_8(void);	// TODO: takes epsilon values from 8.6 and MSS data from 7.4
