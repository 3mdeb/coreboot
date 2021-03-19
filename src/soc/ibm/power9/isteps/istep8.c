/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <console/console.h>
#include <soc/istep8.h>

/* From src/import/chips/p9/procedures/hwp/nest/p9_fbc_eff_config.C */
/* LE epsilon (2 chips per-group) */
static const uint32_t EPSILON_R_T0_LE[] = {    7,    7,    8,    8,   10,   22 };
static const uint32_t EPSILON_R_T1_LE[] = {    7,    7,    8,    8,   10,   22 };
static const uint32_t EPSILON_R_T2_LE[] = {   67,   69,   71,   74,   79,  103 };
static const uint32_t EPSILON_W_T0_LE[] = {    0,    0,    0,    0,    0,    5 };
static const uint32_t EPSILON_W_T1_LE[] = {   15,   16,   17,   19,   21,   33 };

/* HE epsilon (4 chips per-group) */
static const uint32_t EPSILON_R_T0_HE[] = {    7,    7,    8,    8,   10,   22 };
static const uint32_t EPSILON_R_T1_HE[] = {   92,   94,   96,   99,  104,  128 };
static const uint32_t EPSILON_R_T2_HE[] = {  209,  210,  213,  216,  221,  245 };
static const uint32_t EPSILON_W_T0_HE[] = {   28,   29,   30,   32,   34,   46 };
static const uint32_t EPSILON_W_T1_HE[] = {  117,  118,  119,  121,  123,  135 };

/* HE epsilon (flat 4 Zeppelin) */
static const uint32_t EPSILON_R_T0_F4[] = {    7,    7,    8,    8,   10,   22 };
static const uint32_t EPSILON_R_T1_F4[] = {    7,    7,    8,    8,   10,   22 };
static const uint32_t EPSILON_R_T2_F4[] = {   83,   84,   87,   90,   95,  119 };
static const uint32_t EPSILON_W_T0_F4[] = {    0,    0,    0,    0,    0,    5 };
static const uint32_t EPSILON_W_T1_F4[] = {   18,   19,   20,   22,   24,   36 };

/* HE epsilon (flat 8 configuration) */
static const uint32_t EPSILON_R_T0_F8[] = {    7,    7,    8,    8,   10,   22 };
static const uint32_t EPSILON_R_T1_F8[] = {    7,    7,    8,    8,   10,   22 };
static const uint32_t EPSILON_R_T2_F8[] = {  148,  149,  152,  155,  160,  184 };
static const uint32_t EPSILON_W_T0_F8[] = {    0,    0,    0,    0,    0,    5 };
static const uint32_t EPSILON_W_T1_F8[] = {   76,   77,   78,   80,   82,   94 };

/* Keep them static and later we can use function to retrieve tghe values set here */
static uint32_t eps_r[NUM_EPSILON_READ_TIERS];
static uint32_t eps_w[NUM_EPSILON_WRITE_TIERS];
static int8_t eps_guardband;
static uint8_t core_floor_ratio;
static uint8_t core_ceiling_ratio;
static uint32_t freq_fbc;
static uint32_t freq_core_ceiling;

static int p9_calculate_frequencies(uint8_t *floor_ratio, uint8_t *ceiling_ratio,
				    uint32_t *fbc_freq, uint32_t *freq_ceiling)
{
	/* Default 4800 from src/usr/targeting/common/xmltohb/hb_customized_attrs.xml */
	uint32_t freq_core_floor = 4800; 
	/* According to src/usr/targeting/common/xmltohb/attribute_types.xml
	   ATTR_FREQ_CORE_NOMINAL_MHZ maps to NOMINAL_FREQ_MHZ present in talos.xml */
	uint32_t freq_core_nom = 4800;
	uint8_t async_safe_mode = FABRIC_ASYNC_PERFORMANCE_MODE;
	*freq_ceiling = 4800;

	/* breakpoint ratio: core floor 4.0, pb 2.0 (cache floor :: pb = 8/8) */
	if ((freq_core_floor) >= (2 * (*fbc_freq))) {
		*floor_ratio = FABRIC_CORE_FLOOR_RATIO_RATIO_8_8;
	}
	/* breakpoint ratio: core floor 3.5, pb 2.0 (cache floor :: pb = 7/8) */
	else if ((4 * freq_core_floor) >= (7 * (*fbc_freq))) {
		*floor_ratio = FABRIC_CORE_FLOOR_RATIO_RATIO_7_8;
	}
	/* breakpoint ratio: core floor 3.0, pb 2.0 (cache floor :: pb = 6/8) */
	else if ((2 * freq_core_floor) >= (3 * (*fbc_freq))) {
		*floor_ratio = FABRIC_CORE_FLOOR_RATIO_RATIO_6_8;
	}
	/* breakpoint ratio: core floor 2.5, pb 2.0 (cache floor :: pb = 5/8) */
	else if ((4 * freq_core_floor) >= (5 * (*fbc_freq))) {
		*floor_ratio = FABRIC_CORE_FLOOR_RATIO_RATIO_5_8;
	}
	/* breakpoint ratio: core floor 2.0, pb 2.0 (cache floor :: pb = 4/8) */
	else if (freq_core_floor >= (*fbc_freq)) {
		*floor_ratio = FABRIC_CORE_FLOOR_RATIO_RATIO_4_8;
	}
	/* breakpoint ratio: core floor 1.0, pb 2.0 (cache floor :: pb = 2/8) */
	else if ((2 * freq_core_floor) >= (*fbc_freq)) {
		*floor_ratio = FABRIC_CORE_FLOOR_RATIO_RATIO_2_8;
	}
	else {
		printk (BIOS_ERR, "Unsupported core ceiling/PB frequency ratio = (%d/%d)",
			freq_core_floor, *fbc_freq);
		return -1;
	}

	/* breakpoint ratio: core ceiling 4.0, pb 2.0 (cache ceiling :: pb = 8/8) */
	if (*freq_ceiling >= (2 * (*fbc_freq))) {
		*ceiling_ratio = FABRIC_CORE_CEILING_RATIO_RATIO_8_8;
	}
	/* breakpoint ratio: core ceiling 3.5, pb 2.0 (cache ceiling :: pb = 7/8) */
	else if ((4 * (*freq_ceiling)) >= (7 * (*fbc_freq))) {
		*ceiling_ratio = FABRIC_CORE_CEILING_RATIO_RATIO_7_8;
	}
	/* breakpoint ratio: core ceiling 3.0, pb 2.0 (cache ceiling :: pb = 6/8) */
	else if ((2 * (*freq_ceiling)) >= (3 * (*fbc_freq))) {
		*ceiling_ratio = FABRIC_CORE_CEILING_RATIO_RATIO_6_8;
	}
	/* breakpoint ratio: core ceiling 2.5, pb 2.0 (cache ceiling :: pb = 5/8) */
	else if ((4 * (*freq_ceiling)) >= (5 * (*fbc_freq))) {
		*ceiling_ratio = FABRIC_CORE_CEILING_RATIO_RATIO_5_8;
	}
	/* breakpoint ratio: core ceiling 2.0, pb 2.0 (cache ceiling :: pb = 4/8) */
	else if ((*freq_ceiling) >= (*fbc_freq)) {
		*ceiling_ratio = FABRIC_CORE_CEILING_RATIO_RATIO_4_8;
	}
	/* breakpoint ratio: core ceiling 1.0, pb 2.0 (cache ceiling :: pb = 2/8) */
	else if ((2 * (*freq_ceiling)) >= (*fbc_freq)) {
		*ceiling_ratio = FABRIC_CORE_CEILING_RATIO_RATIO_2_8;
	}
	else {
		printk (BIOS_ERR, "Unsupported core ceiling/PB frequency ratio = (%d/%d)",
			*freq_ceiling, *fbc_freq);
		return -1;
	}

	return 0;
}

static void config_guardband_epsilon(const uint8_t gb_percentage, uint32_t *target_value)
{
	uint32_t delta =  (*target_value * gb_percentage) / 100;
	delta += ((*target_value * gb_percentage) % 100) ? 1 : 0;

	printk(BIOS_DEBUG, "%s\n", __func__);

	/* Apply guardband */
	if (gb_percentage >= 0)
	{
		/* Clamp to maximum value if necessary */
		if (delta > (EPSILON_MAX_VALUE - target_value)) {
			printk(BIOS_DEBUG, "Guardband application generated out-of-range target value,"
					   " clamping to maximum value!\n");
			*target_value = EPSILON_MAX_VALUE;
		} else {
			*target_value += delta;
		}
	}
	else
	{
		/* Clamp to minimum value if necessary */
		if (delta >= *target_value) {
			printk(BIOS_DEBUG, "Guardband application generated out-of-range target value,"
					   " clamping to minimum value!\n");
			*target_value = EPSILON_MIN_VALUE;
		} else {
			*target_value -= delta;
		}
	}

}

static void dump_epsilons(void)
{
	uint32_t i;

	for (i = 0; i < NUM_EPSILON_READ_TIERS; i++)
	{
		printk(BIOS_DEBUG, " R_T[%d] = %d\n", i, eps_r[i]);
	}

	for (i = 0; i < NUM_EPSILON_WRITE_TIERS; i++)
	{
		printk(BIOS_DEBUG, " W_T[%d] = %d\n", i, eps_w[i]);
	}
}

static int p9_calculate_epsilons(uint8_t *floor_ratio, uint8_t *ceiling_ratio,
				 uint32_t *fbc_freq, uint32_t *freq_ceiling)
{
	uint8_t eps_table_type = EPS_TYPE_LE; /* ATTR_PROC_EPS_TABLE_TYPE from talos.xml */
	uint32_t i;

	/* Processor SMP Fabric broadcast scope configuration.
	 *    CHIP_IS_NODE = MODE1 = default
	 *    CHIP_IS_GROUP = MODE2
	 */
	uint8_t pump_mode = CHIP_IS_GROUP; /* ATTR_PROC_FABRIC_PUMP_MODE MODE2 from talos.xml */

	switch(eps_table_type)
	{
	case EPS_TYPE_HE:
		if (pump_mode == CHIP_IS_NODE) {
			eps_r[0] = EPSILON_R_T0_HE[*floor_ratio];
			eps_r[1] = EPSILON_R_T1_HE[*floor_ratio];
			eps_r[2] = EPSILON_R_T2_HE[*floor_ratio];

			eps_w[0] = EPSILON_W_T0_HE[*floor_ratio];
			eps_w[1] = EPSILON_W_T1_HE[*floor_ratio];
		} else {
			eps_r[0] = EPSILON_R_T0_F4[*floor_ratio];
			eps_r[1] = EPSILON_R_T1_F4[*floor_ratio];
			eps_r[2] = EPSILON_R_T2_F4[*floor_ratio];

			eps_w[0] = EPSILON_W_T0_F4[*floor_ratio];
			eps_w[1] = EPSILON_W_T1_F4[*floor_ratio];
		}

		break;
	case EPS_TYPE_HE_F8:
		eps_r[0] = EPSILON_R_T0_F8[*floor_ratio];

		if (pump_mode == CHIP_IS_NODE)
			eps_r[1] = EPSILON_R_T1_F8[*floor_ratio];
		else
			eps_r[1] = EPSILON_R_T0_F8[*floor_ratio];

		eps_r[2] = EPSILON_R_T2_F8[*floor_ratio];

		eps_w[0] = EPSILON_W_T0_F8[*floor_ratio];
		eps_w[1] = EPSILON_W_T1_F8[*floor_ratio];
		break;
	case EPS_TYPE_LE:
		eps_r[0] = EPSILON_R_T0_LE[*floor_ratio];

		if (pump_mode == CHIP_IS_NODE)
			eps_r[1] = EPSILON_R_T1_LE[*floor_ratio];
		else
			eps_r[1] = EPSILON_R_T0_LE[*floor_ratio];

		eps_r[2] = EPSILON_R_T2_LE[*floor_ratio];

		eps_w[0] = EPSILON_W_T0_LE[*floor_ratio];
		eps_w[1] = EPSILON_W_T1_LE[*floor_ratio];
		break;
	default:
		printk(BIOS_WARNING, "Invalid epsilon table type 0x%.8X", eps_table_type);
		break;
	}

	/* dump base epsilon values */
	printk(BIOS_DEBUG, "Base epsilon values read from table:\n");
	dump_eplisons();

	eps_guardband = 20;

	/* scale base epsilon values if core is running 2x nest frequency */
	if (*ceiling_ratio == FABRIC_CORE_CEILING_RATIO_RATIO_8_8)
	{
		printk(BIOS_DEBUG, "Scaling based on ceiling frequency");
		uint8_t scale_percentage = 100 * (*freq_ceiling) / (2 * (*fbc_freq));
		scale_percentage -= 100;

		/* scale/apply guardband read epsilons */
		for (i = 0; i < NUM_EPSILON_READ_TIERS; i++)
			config_guardband_epsilon(scale_percentage, &eps_r[i]);

		/* Scale write epsilons */
		for (i = 0; i < NUM_EPSILON_WRITE_TIERS; i++)
			config_guardband_epsilon(scale_percentage, &eps_w[i]);
	}

	printk(BIOS_DEBUG, "Scaled epsilon values based on %s%d percent guardband:\n",
			   (eps_guardband >= 0) ? ("+") : ("-"), eps_guardband);
	dump_eplisons();

	/* 
	 * check relationship of epsilon counters:
	 *   read tier values are strictly increasing
	 *   write tier values are strictly increaing
	 */
	if ((eps_r[0] > eps_r[1]) || (eps_r[1] > eps_r[2]) || (eps_w[0] > eps_w[1]))
		printk(BIOS_WARNING, "Invalid relationship between base epsilon values\n");
}

void configure_powerbus_link (void)
{

	if (!p9_calculate_frequencies(&core_floor_ratio, &core_ceiling_ratio,
					&freq_fbc, &freq_core_ceiling)) {
		die("Incorrect core or PowerBus frequency");
	}

	p9_calculate_epsilons(&core_floor_ratio, &core_ceiling_ratio,
				&freq_fbc, &freq_core_ceiling);

	/* Do we need to create variables for the fabric link activity and
	   disable them as in p9_fbc_eff_config_reset_attrs? */
}
