/* SPDX-License-Identifier: GPL-2.0-only */

#include "pci.h"

#include <commonlib/bsd/helpers.h>
#include <console/console.h>
#include <cpu/power/scom.h>
#include <stdint.h>
#include <string.h>
#include <timer.h>

#define MAX_PEC_PER_PROC 3
#define MAX_PHB_PER_PROC 6

#define MAX_LANE_GROUPS_PER_PEC 4

#define NUM_PCIE_LANES 16
#define NUM_PCS_CONFIG 4

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

/* Bit position of the PHB with the largest number a given PEC can use */
enum pec_phb_shift {
	PEC0_PHB_SHIFT = 7, // PHB0 only
	PEC1_PHB_SHIFT = 5, // PHB1 - PHB2
	PEC2_PHB_SHIFT = 2, // PHB3 - PHB5
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

static const uint64_t RX_VGA_CTRL3_REGISTER[NUM_PCIE_LANES] = {
	0x8000008D0D010C3F,
	0x800000CD0D010C3F,
	0x8000018D0D010C3F,
	0x800001CD0D010C3F,
	0x8000028D0D010C3F,
	0x800002CD0D010C3F,
	0x8000038D0D010C3F,
	0x800003CD0D010C3F,
	0x8000088D0D010C3F,
	0x800008CD0D010C3F,
	0x8000098D0D010C3F,
	0x800009CD0D010C3F,
	0x80000A8D0D010C3F,
	0x80000ACD0D010C3F,
	0x80000B8D0D010C3F,
	0x80000BCD0D010C3F,
};

static const uint64_t RX_LOFF_CNTL_REGISTER[NUM_PCIE_LANES] = {
	0x800000A60D010C3F,
	0x800000E60D010C3F,
	0x800001A60D010C3F,
	0x800001E60D010C3F,
	0x800002A60D010C3F,
	0x800002E60D010C3F,
	0x800003A60D010C3F,
	0x800003E60D010C3F,
	0x800008A60D010C3F,
	0x800008E60D010C3F,
	0x800009A60D010C3F,
	0x800009E60D010C3F,
	0x80000AA60D010C3F,
	0x80000AE60D010C3F,
	0x80000BA60D010C3F,
	0x80000BE60D010C3F,
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

static uint8_t determine_lane_configs(const struct lane_config_row **pec_cfgs)
{
	uint8_t pec = 0;
	uint8_t phb_active_mask = 0;

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

		// PEC[ATTR_PROC_PCIE_IOP_CONFIG] := pec_cfgs[pec]->lane_config 
		// PEC[ATTR_PROC_PCIE_REFCLOCK_ENABLE] := 1 
		// PEC[ATTR_PROC_PCIE_PCS_SYSTEM_CNTL] := pec_cfgs[pec]->phb_to_pcie_mac 
	}

	return phb_active_mask;
}

static uint64_t pec_val(int pec_id, uint8_t in,
			uint32_t pec0_s, uint32_t pec0_c,
			uint32_t pec1_s, uint32_t pec1_c,
			uint32_t pec2_s, uint32_t pec2_c)
{
	uint64_t out = 0;

	switch (pec_id) {
		case 0:
			out = PPC_SHIFT(in & ((1 << pec0_c) - 1), pec0_s);
			break;
		case 1:
			out = PPC_SHIFT(in & ((1 << pec1_c) - 1), pec1_s);
			break;
		case 2:
			out = PPC_SHIFT(in & ((1 << pec2_c) - 1), pec2_s);
			break;
		default:
			die("Unknown PEC ID: %d\n", pec_id);
	}

	return out;
}

static void phase1(const struct lane_config_row **pec_cfgs,
		   const uint8_t *iovalid_enable)
{
	enum {
		PEC_CPLT_CONF1_OR = 0x0D000019,
		PEC_CPLT_CTRL0_OR = 0x0D000010,
		PEC_CPLT_CONF1_CLEAR = 0x0D000029,

		PEC_PCS_RX_CONFIG_MODE_REG = 0x800004800D010C3F,
		PEC_PCS_RX_CDR_GAIN_REG = 0x800004B30D010C3F,
		PEC_PCS_RX_SIGDET_CONTROL_REG = 0x800004A70D010C3F,

		PCI_IOP_FIR_ACTION0_REG = 0x0000000000000000ULL,
		PCI_IOP_FIR_ACTION1_REG = 0xE000000000000000ULL,
		PCI_IOP_FIR_MASK_REG    = 0x1FFFFFFFF8000000ULL,

		PEC_FIR_ACTION0_REG = 0x0D010C06,
		PEC_FIR_ACTION1_REG = 0x0D010C07,
		PEC_FIR_MASK_REG = 0x0D010C03,

		PEC0_IOP_CONFIG_START_BIT = 13,
		PEC1_IOP_CONFIG_START_BIT = 14,
		PEC2_IOP_CONFIG_START_BIT = 10,
		PEC0_IOP_BIT_COUNT = 1,
		PEC1_IOP_BIT_COUNT = 2,
		PEC2_IOP_BIT_COUNT = 3,
		PEC0_IOP_SWAP_START_BIT = 12,
		PEC1_IOP_SWAP_START_BIT = 12,
		PEC2_IOP_SWAP_START_BIT = 7,
		PEC0_IOP_IOVALID_ENABLE_START_BIT = 4,
		PEC1_IOP_IOVALID_ENABLE_START_BIT = 4,
		PEC2_IOP_IOVALID_ENABLE_START_BIT = 4,
		PEC_IOP_IOVALID_ENABLE_STACK0_BIT = 4,
		PEC_IOP_IOVALID_ENABLE_STACK1_BIT = 5,
		PEC_IOP_IOVALID_ENABLE_STACK2_BIT = 6,
		PEC_IOP_REFCLOCK_ENABLE_START_BIT = 32,
		PEC_IOP_PMA_RESET_START_BIT = 29,
		PEC_IOP_PIPE_RESET_START_BIT = 28,

		PEC_PCS_PCLCK_CNTL_PLLA_REG = 0x8000050F0D010C3F,
		PEC_PCS_PCLCK_CNTL_PLLB_REG = 0x8000054F0D010C3F,
		PEC_PCS_TX_DCLCK_ROTATOR_REG = 0x800004450D010C3F,
		PEC_PCS_TX_PCIE_REC_DETECT_CNTL1_REG = 0x8000046C0D010C3F,
		PEC_PCS_TX_PCIE_REC_DETECT_CNTL2_REG = 0x8000046D0D010C3F,
		PEC_PCS_TX_POWER_SEQ_ENABLE_REG = 0x800004700D010C3F,

		PEC_SCOM0X0B_EDMOD = 52,

		PEC_PCS_RX_VGA_CONTROL1_REG = 0x8000048B0D010C3F,
		PEC_PCS_RX_VGA_CONTROL2_REG = 0x8000048C0D010C3F,
		PEC_PCS_SYS_CONTROL_REG = 0x80000C000D010C3F,
	};

	uint8_t pec = 0;

	for (pec = 0; pec < MAX_PEC_PER_PROC; ++pec) {
		long time;
		uint8_t i;
		uint64_t val;
		uint8_t proc_pcie_iop_swap;

		chiplet_id_t chiplet = PCI0_CHIPLET_ID + pec;

		/* Phase1 init step 1 (get VPD, no operation here) */

		/* Phase1 init step 2a */
		val = pec_val(pec, pec_cfgs[pec]->lane_config,
			      PEC0_IOP_CONFIG_START_BIT, PEC0_IOP_BIT_COUNT * 2,
			      PEC1_IOP_CONFIG_START_BIT, PEC1_IOP_BIT_COUNT * 2,
			      PEC2_IOP_CONFIG_START_BIT, PEC2_IOP_BIT_COUNT * 2);
		write_scom_for_chiplet(chiplet, PEC_CPLT_CONF1_OR, val);

		/* Phase1 init step 2b */

		/* ATTR_PROC_PCIE_IOP_SWAP, from talos.xml */
		proc_pcie_iop_swap = 0;

		val = pec_val(pec, proc_pcie_iop_swap,
			      PEC0_IOP_SWAP_START_BIT, PEC0_IOP_BIT_COUNT,
			      PEC1_IOP_SWAP_START_BIT, PEC1_IOP_BIT_COUNT,
			      PEC2_IOP_SWAP_START_BIT, PEC2_IOP_BIT_COUNT);
		write_scom_for_chiplet(chiplet, PEC_CPLT_CONF1_OR, val);

		/* Phase1 init step 3a */

		val = pec_val(pec, iovalid_enable[pec],
			      PEC0_IOP_IOVALID_ENABLE_START_BIT, PEC0_IOP_BIT_COUNT,
			      PEC1_IOP_IOVALID_ENABLE_START_BIT, PEC1_IOP_BIT_COUNT,
			      PEC2_IOP_IOVALID_ENABLE_START_BIT, PEC2_IOP_BIT_COUNT);

		/* Set IOVALID for base PHB if PHB2, or PHB4, or PHB5 are set (SW417485) */
		if ((val & PPC_BIT(PEC_IOP_IOVALID_ENABLE_STACK1_BIT)) ||
		    (val & PPC_BIT(PEC_IOP_IOVALID_ENABLE_STACK2_BIT))) {
			val |= PPC_BIT(PEC_IOP_IOVALID_ENABLE_STACK0_BIT);
			val |= PPC_BIT(PEC_IOP_IOVALID_ENABLE_STACK1_BIT);
		}

		write_scom_for_chiplet(chiplet, PEC_CPLT_CONF1_OR, val);

		/* Phase1 init step 3b (enable clock) */
		/* XXX: assume all PECs are enabled (due to hard-coded lanes),
		 *      ATTR_PROC_PCIE_REFCLOCK_ENABLE */
		write_scom_for_chiplet(chiplet, PEC_CPLT_CTRL0_OR,
				       PPC_BIT(PEC_IOP_REFCLOCK_ENABLE_START_BIT));

		/* Phase1 init step 4 (PMA reset) */

		write_scom_for_chiplet(chiplet, PEC_CPLT_CONF1_CLEAR,
				       PPC_BIT(PEC_IOP_PMA_RESET_START_BIT));
		(void)wait_us(1, false); /* at least 400ns */
		write_scom_for_chiplet(chiplet, PEC_CPLT_CONF1_OR,
				       PPC_BIT(PEC_IOP_PMA_RESET_START_BIT));
		(void)wait_us(1, false); /* at least 400ns */
		write_scom_for_chiplet(chiplet, PEC_CPLT_CONF1_CLEAR,
				       PPC_BIT(PEC_IOP_PMA_RESET_START_BIT));

		/*
		 * Poll for PRTREADY status on PLLA and PLLB:
		 * PEC_IOP_PLLA_VCO_COURSE_CAL_REGISTER1 = 0x800005010D010C3F
		 * PEC_IOP_PLLB_VCO_COURSE_CAL_REGISTER1 = 0x800005410D010C3F
		 * PEC_IOP_HSS_PORT_READY_START_BIT = 58
		 */
		time = wait_us(40,
				(read_scom_for_chiplet(chiplet, 0x800005010D010C3F) & PPC_BIT(58)) ||
				(read_scom_for_chiplet(chiplet, 0x800005410D010C3F) & PPC_BIT(58)));
		if (!time)
			die("IOP HSS Port Ready status is not set!");

		/* Phase1 init step 5 (Set IOP FIR action0) */
		write_scom_for_chiplet(chiplet, PEC_FIR_ACTION0_REG, PCI_IOP_FIR_ACTION0_REG);

		/* Phase1 init step 6 (Set IOP FIR action1) */
		write_scom_for_chiplet(chiplet, PEC_FIR_ACTION1_REG, PCI_IOP_FIR_ACTION1_REG);

		/* Phase1 init step 7 (Set IOP FIR mask) */
		write_scom_for_chiplet(chiplet, PEC_FIR_MASK_REG, PCI_IOP_FIR_MASK_REG);

		/* Phase1 init step 8-11 (Config 0 - 3) */

		/* ATTR_PROC_PCIE_PCS_RX_CDR_GAIN, from talos.xml */
		uint8_t pcs_cdr_gain[] = { 0x56, 0x47, 0x47, 0x47 };
		/* ATTR_PROC_PCIE_PCS_RX_INIT_GAIN, all zeroes by default */
		uint8_t pcs_init_gain = 0;
		/* ATTR_PROC_PCIE_PCS_RX_PK_INIT, all zeroes by default */
		uint8_t pcs_pk_init = 0;
		/* ATTR_PROC_PCIE_PCS_RX_SIGDET_LVL, defaults and talos.xml */
		uint8_t pcs_sigdet_lvl = 0x0B;

		uint32_t pcs_config_mode[NUM_PCS_CONFIG] = { 0xA006, 0xA805, 0xB071, 0xB870 };

		for (i = 0; i < NUM_PCS_CONFIG; ++i) {
			uint8_t lane;

			/* RX Config Mode */
			// 
			write_scom_for_chiplet(chiplet, PEC_PCS_RX_CONFIG_MODE_REG,
					       PPC_SHIFT(pcs_config_mode[i], 48));

			/* RX CDR GAIN */
			// 
			scom_and_or_for_chiplet(chiplet, PEC_PCS_RX_CDR_GAIN_REG,
						~PPC_BITMASK(56, 63),
						PPC_SHIFT(pcs_cdr_gain[i], 63));

			for (lane = 0; lane < NUM_PCIE_LANES; ++lane) {
				/* RX INITGAIN */
				// 
				scom_and_or_for_chiplet(chiplet, RX_VGA_CTRL3_REGISTER[lane],
							~PPC_BITMASK(48, 52),
							PPC_SHIFT(pcs_init_gain, 48));

				/* RX PKINIT */
				// 
				scom_and_or_for_chiplet(chiplet, RX_LOFF_CNTL_REGISTER[lane],
							~PPC_BITMASK(58, 63),
							PPC_SHIFT(pcs_pk_init, 63));
			}

			/* RX SIGDET LVL */
			// 
			scom_and_or_for_chiplet(chiplet, PEC_PCS_RX_SIGDET_CONTROL_REG,
						~PPC_BITMASK(59, 63),
						PPC_SHIFT(pcs_sigdet_lvl, 63));
		}

		/*
		 * Phase1 init step 12 (RX Rot Cntl CDR Lookahead Disabled,SSC Disabled)
                 *
		 * Skipping update of PEC_PCS_RX_ROT_CNTL_REG, because all these attributes are zero
		 * for Nimbus and there is nothing to update:
		 *  - ATTR_PROC_PCIE_PCS_RX_ROT_CDR_LOOKAHEAD
		 *  - ATTR_PROC_PCIE_PCS_RX_ROT_CDR_SSC
		 *  - ATTR_PROC_PCIE_PCS_RX_ROT_EXTEL
		 *  - ATTR_PROC_PCIE_PCS_RX_ROT_RST_FW
		 */

		/* Phase1 init step 13 (RX Config Mode Enable External Config Control) */
		// verify this and other shifts below  
		write_scom_for_chiplet(chiplet, PEC_PCS_RX_CONFIG_MODE_REG,
				       PPC_SHIFT(0x8600, 48));

		/* Phase1 init step 14 (PCLCK Control Register - PLLA) */
		/* ATTR_PROC_PCIE_PCS_PCLCK_CNTL_PLLA = 0xF8 */
		scom_and_or_for_chiplet(chiplet, PEC_PCS_PCLCK_CNTL_PLLA_REG,
					~PPC_BITMASK(56, 63),
					PPC_SHIFT(0xf8, 63));

		/* Phase1 init step 15 (PCLCK Control Register - PLLB) */
		/* ATTR_PROC_PCIE_PCS_PCLCK_CNTL_PLLB = 0xF8 */
		scom_and_or_for_chiplet(chiplet, PEC_PCS_PCLCK_CNTL_PLLB_REG,
					~PPC_BITMASK(56, 63),
					PPC_SHIFT(0xf8, 63));

		/* Phase1 init step 16 (TX DCLCK Rotator Override) */
		/* ATTR_PROC_PCIE_PCS_TX_DCLCK_ROT = 0x0022 */
		write_scom_for_chiplet(chiplet, PEC_PCS_TX_DCLCK_ROTATOR_REG,
				       PPC_SHIFT(0x0022, 48));

		/* Phase1 init step 17 (TX PCIe Receiver Detect Control Register 1) */
		/* ATTR_PROC_PCIE_PCS_TX_PCIE_RECV_DETECT_CNTL_REG1 = 0xAA7A */
		write_scom_for_chiplet(chiplet, PEC_PCS_TX_PCIE_REC_DETECT_CNTL1_REG,
				       PPC_SHIFT(0xaa7a, 48));

		/* Phase1 init step 18 (TX PCIe Receiver Detect Control Register 2) */
		/* ATTR_PROC_PCIE_PCS_TX_PCIE_RECV_DETECT_CNTL_REG2 = 0x2000 */
		write_scom_for_chiplet(chiplet, PEC_PCS_TX_PCIE_REC_DETECT_CNTL2_REG,
				       PPC_SHIFT(0x2000, 48));

		/* Phase1 init step 19 (TX Power Sequence Enable) */
		/* ATTR_PROC_PCIE_PCS_TX_POWER_SEQ_ENABLE = 0xFF */
		scom_and_or_for_chiplet(chiplet, PEC_PCS_TX_POWER_SEQ_ENABLE_REG,
					~PPC_BITMASK(56, 62),
					PPC_SHIFT(0xff, 56));

		/* Phase1 init step 20 (RX VGA Control Register 1) */

		/* ATTR_PROC_PCIE_PCS_RX_VGA_CNTL_REG1 = 0 */
		val = PPC_SHIFT(0, 48);

		/* Becase ATTR_CHIP_EC_FEATURE_HW414759 = 1 */
		val |= PPC_BIT(PEC_SCOM0X0B_EDMOD);
		val |= PPC_BIT(PEC_SCOM0X0B_EDMOD + 1);

		write_scom_for_chiplet(chiplet, PEC_PCS_RX_VGA_CONTROL1_REG, val);

		/* Phase1 init step 21 (RX VGA Control Register 2) */
		/* ATTR_PROC_PCIE_PCS_RX_VGA_CNTL_REG2 = 0 */
		write_scom_for_chiplet(chiplet, PEC_PCS_RX_VGA_CONTROL2_REG,
				       PPC_SHIFT(0, 48));

		/* Phase1 init step 22 (RX DFE Func Control Register 1) */
		/* ATTR_PROC_PCIE_PCS_RX_DFE_FDDC = 0, so not updating PEC_IOP_RX_DFE_FUNC_REGISTER1 */

		/* Phase1 init step 23 (PCS System Control) */
		/* ATTR_PROC_PCIE_PCS_SYSTEM_CNTL computed above */
		scom_and_or_for_chiplet(chiplet, PEC_PCS_SYS_CONTROL_REG,
					~PPC_BITMASK(55, 63),
					PPC_SHIFT(pec_cfgs[pec]->phb_to_pcie_mac, 63));

		/*
		 * All values in ATTR_PROC_PCIE_PCS_M_CNTL seem to be 0, which
		 * makes the next four steps no-op.  Hostboot has bugs here in
		 * that it updates PEC_PCS_M1_CONTROL_REG 4 times instead of
		 * updating 4 different registers (M1-M4), but no-op conceals this.
		 */

		/* Phase1 init step 24 (PCS M1 Control) */
		/* Phase1 init step 25 (PCS M2 Control) */
		/* Phase1 init step 26 (PCS M3 Control) */
		/* Phase1 init step 27 (PCS M4 Control) */

		/* Delay a minimum of 200ns to allow prior SCOM programming to take effect */
		(void)wait_us(1, false);

		// Phase1 init step 28
		write_scom_for_chiplet(chiplet, PEC_CPLT_CONF1_CLEAR,
				       PPC_BIT(PEC_IOP_PIPE_RESET_START_BIT));

		/*
		 * Delay a minimum of 300ns for reset to complete.
		 * Inherent delay before deasserting PCS PIPE Reset is enough here.
		 */
	}
}

static void enable_ridi(void)
{
	enum {
		PERV_NET_CTRL0 = 0x000F0040,
		PERV_NET_CTRL0_WOR = 0x000F0042,
	};

	uint8_t pec = 0;

	for (pec = 0; pec < MAX_PEC_PER_PROC; ++pec) {
		chiplet_id_t chiplet = PCI0_CHIPLET_ID + pec;

		/* Getting NET_CTRL0 register value and checking its CHIPLET_ENABLE bit */
		if (read_scom_for_chiplet(chiplet, PERV_NET_CTRL0) & PPC_BIT(0)) {
			/* Enable Recievers, Drivers DI1 & DI2 */
			uint64_t val = 0;
			val |= PPC_BIT(19); // NET_CTRL0.RI_N = 1
			val |= PPC_BIT(20); // NET_CTRL0.DI1_N = 1
			val |= PPC_BIT(21); // NET_CTRL0.DI2_N = 1
			write_scom_for_chiplet(chiplet, PERV_NET_CTRL0_WOR, val);
		}
	}
}

static void init_pecs(const uint8_t *iovalid_enable)
{
	enum {
		P9N2_PEC_ADDREXTMASK_REG = 0x4010C05,
		PEC_PBCQHWCFG_REG = 0x4010C00,
		PEC_NESTTRC_REG = 0x4010C03,
		PEC_PBAIBHWCFG_REG = 0xD010800,

		/* powerbus.c has these too */
		MBOX_SCRATCH_REG1 = 0x00050038,
		MBOX_SCRATCH_REG6_GROUP_PUMP_MODE = (1 << 23),
	};

	uint8_t dd = get_dd();

	uint8_t pec = 0;

	uint64_t scratch_reg6 = read_scom(MBOX_SCRATCH_REG1 + 5);

	/* ATTR_PROC_FABRIC_PUMP_MODE, it's either node or group pump mode */
	bool node_pump_mode = !(scratch_reg6 & MBOX_SCRATCH_REG6_GROUP_PUMP_MODE);

	for (pec = 0; pec < MAX_PEC_PER_PROC; ++pec) {
		uint64_t val = 0;
		chiplet_id_t chiplet = PCI0_CHIPLET_ID + pec;

		/*
		 * ATTR_FABRIC_ADDR_EXTENSION_GROUP_ID = 0
		 * ATTR_FABRIC_ADDR_EXTENSION_CHIP_ID = 0
		 */
		scom_and_or_for_chiplet(chiplet, P9N2_PEC_ADDREXTMASK_REG,
					~PPC_BITMASK(0, 6),
					PPC_SHIFT(0, 6));

		/*
		 * Phase2 init step 1
		 * NestBase+0x00
		 * Set bits 00:03 = 0b0001 Set hang poll scale
		 * Set bits 04:07 = 0b0001 Set data scale
		 * Set bits 08:11 = 0b0001 Set hang pe scale
		 * Set bit 22 = 0b1 Disable out­of­order store behavior
		 * Set bit 33 = 0b1 Enable Channel Tag streaming behavior
		 * Set bits 34:35 = 0b11 Set P9 Style cache-inject behavior
		 * Set bits 46:48 = 0b011 Set P9 Style cache-inject rate, 1/16 cycles
		 * Set bit 60 = 0b1 only if PEC is bifurcated or trifurcated.
		 * if HW423589_option1, set Disable Group Scope (r/w) and Use Vg(sys) at Vg scope
		 */

		val = read_scom_for_chiplet(chiplet, PEC_PBCQHWCFG_REG);
		/* Set hang poll scale */
		val &= ~PPC_BITMASK(0, 3);
		val |= PPC_SHIFT(1, 3);
		/* Set data scale */
		val &= ~PPC_BITMASK(4, 7);
		val |= PPC_SHIFT(1, 7);
		/* Set hang pe scale */
		val &= ~PPC_BITMASK(8, 11);
		val |= PPC_SHIFT(1, 11);
		/* Disable out­of­order store behavior */
		val |= PPC_BIT(22);
		/* Enable Channel Tag streaming behavior */
		val |= PPC_BIT(33);

		/* Set Disable Group Scope (r/w) and Use Vg(sys) at Vg scope */
		val |= PPC_BIT(41); // PEC_PBCQHWCFG_REG_PE_DISABLE_WR_VG
		val |= PPC_BIT(42); // PEC_PBCQHWCFG_REG_PE_DISABLE_WR_SCOPE_GROUP
		val |= PPC_BIT(43); // PEC_PBCQHWCFG_REG_PE_DISABLE_INTWR_VG
		val |= PPC_BIT(44); // PEC_PBCQHWCFG_REG_PE_DISABLE_INTWR_SCOPE_GROUP
		val |= PPC_BIT(54); // PEC_PBCQHWCFG_REG_PE_DISABLE_RD_VG
		val |= PPC_BIT(51); // PEC_PBCQHWCFG_REG_PE_DISABLE_RD_SCOPE_GROUP
		val |= PPC_BIT(56); // PEC_PBCQHWCFG_REG_PE_DISABLE_TCE_SCOPE_GROUP
		val |= PPC_BIT(59); // PEC_PBCQHWCFG_REG_PE_DISABLE_TCE_VG

		/* Disable P9 Style cache injects if chip is node */
		if (!node_pump_mode) {
			/*
			 * ATTR_PROC_PCIE_CACHE_INJ_MODE
			 * Attribute to control the cache inject mode.
			 *
			 * DISABLE_CI      = 0x0 - Disable cache inject completely. (Reset value default)
			 * P7_STYLE_CI     = 0x1 - Use cache inject design from Power7.
			 * PCITLP_STYLE_CI = 0x2 - Use PCI TLP Hint bits in packet to perform the cache inject.
			 * P9_STYLE_CI     = 0x3 - Initial attempt as cache inject. Power9 style. (Attribute default)
			 *
			 * Different cache inject modes will affect DMA write performance. The attribute default was
			 * selected based on various workloads and was to be the most optimal settings for Power9.
			 * fapi2::ATTR_PROC_PCIE_CACHE_INJ_MODE = 3 by default
			 */
			val &= ~PPC_BITMASK(34, 36);
			val |= PPC_SHIFT(0x3, 36);

			if (dd == 0x21 || dd == 0x22 || dd == 0x23) {
				/*
				 * ATTR_PROC_PCIE_CACHE_INJ_THROTTLE
				 * Attribute to control the cache inject throttling when cache inject is enable.
				 *
				 * DISABLE   = 0x0 - Disable cache inject throttling. (Reset value default)
				 * 16_CYCLES = 0x1 - Perform 1 cache inject every 16 clock cycles.
				 * 32_CYCLES = 0x3 - Perform 1 cache inject every 32 clock cycles. (Attribute default)
				 * 64_CYCLES = 0x7 - Perform 1 cache inject every 32 clock cycles.
				 *
				 * Different throttle rates will affect DMA write performance. The attribute default
				 * settings were optimal settings found across various workloads.
				 */
				val &= ~PPC_BITMASK(46, 48);
				val |= PPC_SHIFT(0x3, 48);
			}
		}

		if (pec == 1 || (pec == 2 && iovalid_enable[pec] != 0x4))
			val |= PPC_BIT(60); // PEC_PBCQHWCFG_REG_PE_DISABLE_TCE_ARBITRATION

		write_scom_for_chiplet(chiplet, PEC_PBCQHWCFG_REG, val);

		/*
		 * Phase2 init step 2
		 * NestBase + 0x01
		 * N/A Modify Drop Priority Control Register (DrPriCtl)
		 */

		/*
		 * Phase2 init step 3
		 * NestBase + 0x03
		 * Set bits 00:03 = 0b1001 Enable trace, and select
		 *                         inbound operations with addr information
		 */
		scom_and_or_for_chiplet(chiplet, PEC_NESTTRC_REG,
					~PPC_BITMASK(0, 3),
					PPC_SHIFT(9, 3));

		/*
		 * Phase2 init step 4
		 * NestBase+0x05
		 * N/A For use of atomics/asb_notify
		 */

		/*
		 * Phase2 init step 5
		 * NestBase+0x06
		 * N/A To override scope prediction
		 */

		/*
		 * Phase2 init step 6
		 * PCIBase +0x00
		 * Set bits 30 = 0b1 Enable Trace
		 */
		val = 0;
		val |= PPC_BIT(0x1E); // PEC_PBAIBHWCFG_REG_PE_PCIE_CLK_TRACE_EN
		val |= PPC_SHIFT(7, 0x2A); // PEC_AIB_HWCFG_OSBM_HOL_BLK_CNT
		write_scom_for_chiplet(chiplet, PEC_PBAIBHWCFG_REG, val);
	}
}

static uint64_t phb_addr(uint8_t phb, uint64_t addr)
{
	chiplet_id_t chiplet;
	uint8_t sat_id = (addr >> 6) & 0xF;

	if (phb == 0) {
		chiplet = PCI0_CHIPLET_ID;
		sat_id = (sat_id < 4 ? 1 : 4);
	} else {
		chiplet = PCI0_CHIPLET_ID + (phb / 3) + 1;
		sat_id = (sat_id < 4 ? 1 : 4)
		       + ((phb % 2) ? 0 : 1)
		       + (2 * (phb / 5));
	}

	addr &= ~PPC_BITMASK(34, 39);
	addr |= PPC_SHIFT(chiplet & 0x3F, 39);

	addr &= ~PPC_BITMASK(54, 57);
	addr |= PPC_SHIFT(sat_id & 0xF, 57);

	return addr;
}

static void init_phbs(uint8_t phb_active_mask, const uint8_t *iovalid_enable)
{
	enum {
		PHB_CERR_RPT0_REG = 0x4010C4A,
		PHB_CERR_RPT1_REG = 0x4010C4B,
		PHB_NFIR_REG = 0x4010C40,
		PHB_NFIRWOF_REG = 0x4010C48,

		PHB_NFIRACTION0_REG = 0x4010C46,
		PCI_NFIR_ACTION0_REG = 0x5B0F81E000000000,

		PHB_NFIRACTION1_REG = 0x4010C47,
		PCI_NFIR_ACTION1_REG = 0x7F0F81E000000000,

		PHB_NFIRMASK_REG = 0x4010C43,
		PCI_NFIR_MASK_REG = 0x30001C00000000,

		PHB_PE_DFREEZE_REG = 0x4010C55,
		PHB_PBAIB_CERR_RPT_REG = 0xD01084B,
		PHB_PFIR_REG = 0xD010840,
		PHB_PFIRWOF_REG = 0xD010848,

		PHB_PFIRACTION0_REG = 0xD010846,
		PCI_PFIR_ACTION0_REG = 0xB000000000000000,

		PHB_PFIRACTION1_REG = 0xD010847,
		PCI_PFIR_ACTION1_REG = 0xB000000000000000,

		PHB_PFIRMASK_REG = 0xD010843,
		PCI_PFIR_MASK_REG = 0xE00000000000000,

		P9_PCIE_CONFIG_BAR_SHIFT = 8,

		PHB_MMIOBAR0_REG = 0x4010C4E,
		PHB_MMIOBAR0_MASK_REG = 0x4010C4F,
		PHB_MMIOBAR1_REG = 0x4010C50,
		PHB_MMIOBAR1_MASK_REG = 0x04010C51,
		PHB_PHBBAR_REG = 0x4010C52,
		PHB_BARE_REG = 0x4010C54,

		PHB_PHBRESET_REG = 0xD01084A,
		PHB_ACT0_REG = 0xD01090E,
		PHB_ACTION1_REG = 0xD01090F,
		PHB_MASK_REG = 0xD01090B,
	};

	/* ATTR_PROC_PCIE_MMIO_BAR0_BASE_ADDR_OFFSET */
	uint64_t mmio_bar0_offsets[MAX_PHB_PER_PROC] = { 0 };
	/* ATTR_PROC_PCIE_MMIO_BAR1_BASE_ADDR_OFFSET */
	uint64_t mmio_bar1_offsets[MAX_PHB_PER_PROC] = { 0 };
	/* ATTR_PROC_PCIE_REGISTER_BAR_BASE_ADDR_OFFSET */
	uint64_t register_bar_offsets[MAX_PHB_PER_PROC] = { 0 };
	/* ATTR_PROC_PCIE_BAR_SIZE */
	uint64_t bar_sizes[3] = { 0 };

	/* Determine base address of chip MMIO range */
	uint64_t base_addr_mmio = 0;
	base_addr_mmio |= PPC_SHIFT(0, 12); // 5 bits, ATTR_PROC_FABRIC_SYSTEM_ID
	base_addr_mmio |= PPC_SHIFT(0, 18); // 4 bits, ATTR_PROC_EFF_FABRIC_GROUP_ID
	base_addr_mmio |= PPC_SHIFT(0, 21); // 3 bits, ATTR_PROC_EFF_FABRIC_CHIP_ID
	base_addr_mmio |= PPC_SHIFT(3, 14); // 2 bits, FABRIC_ADDR_MSEL,
	                                    // nm = 0b00/01, m = 0b10, mmio = 0b11

	uint8_t phb = 0;
	for (phb = 0; phb < MAX_PHB_PER_PROC; ++phb) {
		/* BAR enable attribute (ATTR_PROC_PCIE_BAR_ENABLE) */
		uint8_t l_bar_enables[3] = { 0 };

		uint64_t val = 0;
		uint64_t mmio0_bar = base_addr_mmio;
		uint64_t mmio1_bar = base_addr_mmio;
		uint64_t register_bar = base_addr_mmio;

		if (!(phb_active_mask & (PHB0_MASK >> phb)))
			continue;

		/*
		 * Phase2 init step 12_a (yes, out of order)
		 * NestBase + StackBase + 0xA
		 * 0xFFFFFFFF_FFFFFFFF
		 * Clear any spurious cerr_rpt0 bits (cerr_rpt0)
		 */
		write_scom(phb_addr(phb, PHB_CERR_RPT0_REG), PPC_BITMASK(0, 63));

		/*
		 * Phase2 init step 12_b (yes, out of order)
		 * NestBase + StackBase + 0xB
		 * 0xFFFFFFFF_FFFFFFFF
		 * Clear any spurious cerr_rpt1 bits (cerr_rpt1)
		 */
		write_scom(phb_addr(phb, PHB_CERR_RPT1_REG), PPC_BITMASK(0, 63));

		/*
		 * Phase2 init step 7_c
		 * NestBase + StackBase + 0x0
		 * 0x00000000_00000000
		 * Clear any spurious FIR
		 * bits (NFIR)NFIR
		 */
		write_scom(phb_addr(phb, PHB_NFIR_REG), 0);

		/*
		 * Phase2 init step 8
		 * NestBase + StackBase + 0x8
		 * 0x00000000_00000000
		 * Clear any spurious WOF bits (NFIRWOF)
		 */
		write_scom(phb_addr(phb, PHB_NFIRWOF_REG), 0);

		/*
		 * Phase2 init step 9
		 * NestBase + StackBase + 0x6
		 * Set the per FIR Bit Action 0 register
		 */
		write_scom(phb_addr(phb, PHB_NFIRACTION0_REG), PCI_NFIR_ACTION0_REG);

		/*
		 * Phase2 init step 10
		 * NestBase + StackBase + 0x7
		 * Set the per FIR Bit Action 1 register
		 */
		write_scom(phb_addr(phb, PHB_NFIRACTION1_REG), PCI_NFIR_ACTION1_REG);

		/*
		 * Phase2 init step 11
		 * NestBase + StackBase + 0x3
		 * Set FIR Mask Bits to allow errors (NFIRMask)
		 */
		write_scom(phb_addr(phb, PHB_NFIRMASK_REG), PCI_NFIR_MASK_REG);

		/*
		 * Phase2 init step 12
		 * NestBase + StackBase + 0x15
		 * 0x00000000_00000000
		 * Set Data Freeze Type Register for SUE handling (DFREEZE)
		 */
		write_scom(phb_addr(phb, PHB_PE_DFREEZE_REG), 0);

		/*
		 * Phase2 init step 13_a
		 * PCIBase + StackBase + 0xB
		 * 0x00000000_00000000
		 * Clear any spurious pbaib_cerr_rpt bits
		 */
		write_scom(phb_addr(phb, PHB_PBAIB_CERR_RPT_REG), 0);

		/*
		 * Phase2 init step 13_b
		 * PCIBase + StackBase + 0x0
		 * 0x00000000_00000000
		 * Clear any spurious FIR
		 * bits (PFIR)PFIR
		 */
		write_scom(phb_addr(phb, PHB_PFIR_REG), 0);

		/*
		 * Phase2 init step 14
		 * PCIBase + StackBase + 0x8
		 * 0x00000000_00000000
		 * Clear any spurious WOF bits (PFIRWOF)
		 */
		write_scom(phb_addr(phb, PHB_PFIRWOF_REG), 0);

		/*
		 * Phase2 init step 15
		 * PCIBase + StackBase + 0x6
		 * Set the per FIR Bit Action 0 register
		 */
		write_scom(phb_addr(phb, PHB_PFIRACTION0_REG), PCI_PFIR_ACTION0_REG);

		/*
		 * Phase2 init step 16
		 * PCIBase + StackBase + 0x7
		 * Set the per FIR Bit Action 1 register
		 */
		write_scom(phb_addr(phb, PHB_PFIRACTION1_REG), PCI_PFIR_ACTION1_REG);

		/*
		 * Phase2 init step 17
		 * PCIBase + StackBase + 0x3
		 * Set FIR Mask Bits to allow errors (PFIRMask)
		 */
		write_scom(phb_addr(phb, PHB_PFIRMASK_REG), PCI_PFIR_MASK_REG);

		/*
		 * Phase2 init step 18
		 * NestBase + StackBase + 0xE
		 * Set MMIO Base Address Register 0 (MMIOBAR0)
		 */
		mmio0_bar += mmio_bar0_offsets[phb];
		mmio0_bar <<= P9_PCIE_CONFIG_BAR_SHIFT;
		write_scom(phb_addr(phb, PHB_MMIOBAR0_REG), mmio0_bar);

		/*
		 * Phase2 init step 19
		 * NestBase + StackBase + 0xF
		 * Set MMIO BASE Address Register Mask 0 (MMIOBAR0_MASK)
		 */
		write_scom(phb_addr(phb, PHB_MMIOBAR0_MASK_REG), bar_sizes[0]);

		/*
		 * Phase2 init step 20
		 * NestBase + StackBase + 0x10
		 * Set MMIO Base
		 * Address Register 1 (MMIOBAR1)
		 */
		mmio1_bar += mmio_bar1_offsets[phb];
		mmio1_bar <<= P9_PCIE_CONFIG_BAR_SHIFT;
		write_scom(phb_addr(phb, PHB_MMIOBAR1_REG), mmio1_bar);

		/*
		 * Phase2 init step 21
		 * NestBase + StackBase + 0x11
		 * Set MMIO Base Address Register Mask 1 (MMIOBAR1_MASK)
		 */
		write_scom(phb_addr(phb, PHB_MMIOBAR1_MASK_REG), bar_sizes[1]);

		/*
		 * Phase2 init step 22
		 * NestBase + StackBase + 0x12
		 * Set PHB Register Base address Register (PHBBAR)
		 */
		register_bar += register_bar_offsets[phb];
		register_bar <<= P9_PCIE_CONFIG_BAR_SHIFT;
		write_scom(phb_addr(phb, PHB_PHBBAR_REG), register_bar);

		/*
		 * Phase2 init step 23
		 * NestBase + StackBase + 0x14
		 * Set Base address Enable Register (BARE)
		 */

		val = 0;

		if (l_bar_enables[0])
			val |= PPC_BIT(0); // PHB_BARE_REG_PE_MMIO_BAR0_EN, bit 0 for BAR0
		if (l_bar_enables[1])
			val |= PPC_BIT(1); // PHB_BARE_REG_PE_MMIO_BAR1_EN, bit 1 for BAR1
		if (l_bar_enables[2])
			val |= PPC_BIT(1); // PHB_BARE_REG_PE_PHB_BAR_EN, bit 2 for PHB

		write_scom(phb_addr(phb, PHB_BARE_REG), val);

		/*
		 * Phase2 init step 24
		 * PCIBase + StackBase +0x0A
		 * 0x00000000_00000000
		 * Remove ETU/AIB bus from reset (PHBReset)
		 */
		write_scom(phb_addr(phb, PHB_PHBRESET_REG), 0);
		/* Configure ETU FIR (all masked) */
		write_scom(phb_addr(phb, PHB_ACT0_REG), 0);
		write_scom(phb_addr(phb, PHB_ACTION1_REG), 0);
		write_scom(phb_addr(phb, PHB_MASK_REG), PPC_BITMASK(0, 63));
	}
}

static void phase2(uint8_t phb_active_mask, const uint8_t *iovalid_enable)
{
	init_pecs(iovalid_enable);
	init_phbs(phb_active_mask, iovalid_enable);
}

void pci_init(void)
{
	const struct lane_config_row *pec_cfgs[MAX_PEC_PER_PROC] = { NULL };
	uint8_t iovalid_enable[MAX_PEC_PER_PROC] = { 0 };

	uint8_t phb_active_mask = determine_lane_configs(pec_cfgs);

	/*
	 * Mask of functional PHBs for each PEC, ATTR_PROC_PCIE_IOVALID_ENABLE in Hostboot.
	 * LSB is the PHB with the highest number for the given PEC.
	 */
	iovalid_enable[0] = pec_cfgs[0]->phb_active >> PEC0_PHB_SHIFT;
	iovalid_enable[1] = pec_cfgs[1]->phb_active >> PEC1_PHB_SHIFT;
	iovalid_enable[2] = pec_cfgs[2]->phb_active >> PEC2_PHB_SHIFT;

	phase1(pec_cfgs, iovalid_enable);
	enable_ridi();
	phase2(phb_active_mask, iovalid_enable);
}
