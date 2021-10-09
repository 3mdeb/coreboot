/* SPDX-License-Identifier: GPL-2.0-only */

#include "pci.h"

#include <commonlib/bsd/helpers.h>
#include <console/console.h>
#include <stdint.h>
#include <string.h>

#define MAX_PEC_PER_PROC 3
#define MAX_PHB_PER_PROC 6

#define MAX_LANE_GROUPS_PER_PEC 4

/* Enum indicating lane width (units = "number of lanes") */
enum lane_width {
	LANE_WIDTH_NC  = 0,
	LANE_WIDTH_4X  = 4,
	LANE_WIDTH_8X  = 8,
	LANE_WIDTH_16X = 16
};

enum lane_mask {
	LANE_MASK_X16     = 0xFFFF,
	LANE_MASK_X8_GRP0 = 0xFF00,
	LANE_MASK_X8_GRP1 = 0x00FF,
	LANE_MASK_X4_GRP0 = 0x00F0,
	LANE_MASK_X4_GRP1 = 0x000F,
};

/* Enumeration of PHB to PCI MAC mappings */
enum phb_to_mac {
	PHB_X16_MAC_MAP      = 0x0000,
	PHB_X8_X8_MAC_MAP    = 0x0050,
	PHB_X8_X4_X4_MAC_MAP = 0x0090,
};

/* Enum giving bitmask values for enabled PHBs */
enum phb_active_mask {
	PHB_MASK_NA = 0x00, // Sentinel mask (loop terminations)
	PHB0_MASK   = 0x80, // PHB0 enabled
	PHB1_MASK   = 0x40, // PHB1 enabled
	PHB2_MASK   = 0x20, // PHB2 enabled
	PHB3_MASK   = 0x10, // PHB3 enabled
	PHB4_MASK   = 0x08, // PHB4 enabled
	PHB5_MASK   = 0x04, // PHB5 enabled
};

/*
 * Struct for each row in PCIE IOP configuration table.
 * Used by code to compute the IOP config and PHBs active mask.
 */
struct lane_config_row {
	/*
	 * Grouping of lanes under one IOP.
	 * Value signifies width of each PCIE lane set (0, 4, 8, or 16).
	 */
	// enum lane_width  
	uint8_t lane_set[MAX_LANE_GROUPS_PER_PEC];

	/* IOP config value from PCIE IOP configuration table */
	uint8_t lane_config;

	/*
	 * PHB active mask (see phb_active_mask enum)
	 * PHB0 = 0x80
	 * PHB1 = 0x40
	 * PHB2 = 0x20
	 * PHB3 = 0x10
	 * PHB4 = 0x08
	 * PHB5 = 0x04
	 */
	// enum phb_active_mask 
	uint8_t phb_active;

	// enum phb_to_mac   
	uint16_t phb_to_pcie_mac;
};

/*
 * Currently there are three PEC config tables for procs with 48 usable PCIE
 * lanes. In general, the code accumulates the current configuration of
 * the PECs from the MRW and other dynamic information(such as bifurcation)
 * then matches that config to one of the rows in the table.  Once a match
 * is discovered, the PEC config value is  pulled from the matching row and
 * set in the attributes.
 *
 * Each PEC can control up to 16 lanes:
 * - PEC0 can give 16 lanes to PHB0
 * - PEC1 can split 16 lanes between PHB1 & PHB2
 * - PEC2 can split 16 lanes between PHB3, PHB4 & PHB5
 */
static const struct lane_config_row pec0_lane_cfg[] = {
	{
		{ LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC },
		0x00,
		PHB_MASK_NA,
		PHB_X16_MAC_MAP
	},
	{
		{ LANE_WIDTH_16X, LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC },
		0x00,
		PHB0_MASK,
		PHB_X16_MAC_MAP
	},
};
static const struct lane_config_row pec1_lane_cfg[] = {
	{
		{ LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC },
		0x00,
		PHB_MASK_NA,
		PHB_X8_X8_MAC_MAP
	},
	{
		{ LANE_WIDTH_8X, LANE_WIDTH_NC, LANE_WIDTH_8X, LANE_WIDTH_NC },
		0x00,
		PHB1_MASK | PHB2_MASK,
		PHB_X8_X8_MAC_MAP
	},
	{
		{ LANE_WIDTH_8X, LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC },
		0x00,
		PHB1_MASK,
		PHB_X8_X8_MAC_MAP
	},
	{
		{ LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_8X, LANE_WIDTH_NC },
		0x00,
		PHB2_MASK,
		PHB_X8_X8_MAC_MAP
	},
};
static const struct lane_config_row pec2_lane_cfg[] = {
	{
		{ LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC },
		0x00,
		PHB_MASK_NA,
		PHB_X16_MAC_MAP
	},
	{
		{ LANE_WIDTH_16X, LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC },
		0x00,
		PHB3_MASK,
		PHB_X16_MAC_MAP
	},
	{
		{ LANE_WIDTH_8X, LANE_WIDTH_NC, LANE_WIDTH_8X, LANE_WIDTH_NC },
		0x10,
		PHB3_MASK | PHB4_MASK,
		PHB_X8_X8_MAC_MAP
	},
	{
		{ LANE_WIDTH_8X, LANE_WIDTH_NC, LANE_WIDTH_4X, LANE_WIDTH_4X },
		0x20,
		PHB3_MASK | PHB4_MASK | PHB5_MASK,
		PHB_X8_X4_X4_MAC_MAP
	},
};

static const struct lane_config_row *pec_lane_cfgs[] = {
	pec0_lane_cfg,
	pec1_lane_cfg,
	pec2_lane_cfg
};
static const size_t pec_lane_cfg_sizes[] = {
	ARRAY_SIZE(pec0_lane_cfg),
	ARRAY_SIZE(pec1_lane_cfg),
	ARRAY_SIZE(pec2_lane_cfg)
};

// TODO: find (possibly dynamic) source of these values 
// enum lane_width  
static uint16_t lane_masks[MAX_PEC_PER_PROC][MAX_LANE_GROUPS_PER_PEC] = {
	{ LANE_MASK_X16,     0x0,               0x0,               0x0 },
	{ LANE_MASK_X8_GRP0, 0x0, LANE_MASK_X8_GRP1,               0x0 },
	{ LANE_MASK_X8_GRP0, 0x0, LANE_MASK_X4_GRP0, LANE_MASK_X4_GRP1 },
};

static enum lane_width lane_mask_to_width(uint16_t mask)
{
	enum lane_width width = LANE_WIDTH_NC;

	if (mask == LANE_MASK_X16)
		width = LANE_WIDTH_16X;
	else if (mask == LANE_MASK_X8_GRP0 || mask == LANE_MASK_X8_GRP1)
		width = LANE_WIDTH_8X;
	else if (mask == LANE_MASK_X4_GRP0 || mask == LANE_MASK_X4_GRP1)
		width = LANE_WIDTH_4X;
	else
		die("Invalid value for lane mask: 0x%04x\n", mask);

	return width;
}

void pci_init(void)
{
	uint8_t pec;

	uint8_t phb_active_mask = 0;

	const struct lane_config_row *pec_cfgs[3] = { NULL };

	for (pec = 0; pec < MAX_PEC_PER_PROC; ++pec) {
		uint8_t i;
		uint8_t lane_group;

		// enum lane_width  
		uint16_t lane_mask[MAX_LANE_GROUPS_PER_PEC];
		memcpy(&lane_mask, &lane_masks[pec], sizeof(lane_mask));

		struct lane_config_row config = {
			{ LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC, LANE_WIDTH_NC },
			0x00,
			PHB_MASK_NA,
			PHB_X16_MAC_MAP,
		};

		/* Transform effective config to match lane config table format */
		for (lane_group = 0; lane_group < MAX_LANE_GROUPS_PER_PEC; ++lane_group)
			config.lane_set[lane_group] = lane_mask_to_width(lane_mask[lane_group]);

		for (i = 0; i < pec_lane_cfg_sizes[pec]; ++i) {
			if (memcmp(pec_lane_cfgs[pec][i].lane_set, &config.lane_set,
				   sizeof(config.lane_set)) == 0)
				break;
		}

		if (i == pec_lane_cfg_sizes[pec])
			die("Failed to find PCIE IOP configuration for PEC%d\n", pec);

		phb_active_mask |= pec_lane_cfgs[pec][i].phb_active;

		pec_cfgs[pec] = &pec_lane_cfgs[pec][i];

		// PEC[ATTR_PROC_PCIE_IOP_CONFIG] := pec_lane_cfgs[pec][i].lane_config 
		// PEC[ATTR_PROC_PCIE_REFCLOCK_ENABLE] := 1 
		// PEC[ATTR_PROC_PCIE_PCS_SYSTEM_CNTL] := pec_lane_cfgs[pec][i].phb_to_pcie_mac 
	}

	// foreach PEC[ATTR_PROC_PCIE_IOVALID_ENABLE] := mask of functional PHBs 

	// ATTR_PROC_PCIE_PHB_ACTIVE := phb_active_mask 
}
