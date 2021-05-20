#include <cpu/power/istep_8.h>

void* call_host_set_voltages(void *io_pArgs)
{
	TargetHandleList l_procList;
	getAllChips(l_procList, TYPE_PROC, true); // true: return functional procs
	for(const auto & l_procTarget : l_procList)
	{
		const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP> l_fapiProcTarget(l_procTarget);
		p9_setup_evid(l_fapiProcTarget);
	}
}

PlatPmPPB {
	iv_procChip (i_target), iv_pstates_enabled(false), iv_resclk_enabled(0),
	iv_vdm_enabled(0), iv_ivrm_enabled(0), iv_wof_enabled(0), iv_safe_voltage (0),
	iv_safe_frequency(0), iv_reference_frequency_mhz(0), iv_reference_frequency_khz(0),
	iv_frequency_step_khz(0), iv_proc_dpll_divider(0), iv_nest_freq_mhz(0),
	iv_wov_underv_enabled(0), iv_wov_overv_enabled(0)
	iv_attrs.attr_dpll_bias = 0;
	iv_attrs.attr_undervolting = 0;
}

uint32_t revle32(const uint32_t i_x)
{
	uint32_t rx;
#ifndef _BIG_ENDIAN
	uint8_t* pix = (uint8_t*)(&i_x);
	uint8_t* prx = (uint8_t*)(&rx);
	prx[0] = pix[3];
	prx[1] = pix[2];
	prx[2] = pix[1];
	prx[3] = pix[0];
#else
	rx = i_x;
#endif

	return rx;
}

void PlatPmPPB::attr_init(void)
{
	const fapi2::Target<fapi2::TARGET_TYPE_SYSTEM> FAPI_SYSTEM;
	iv_attrs.attr_tdp_rdp_current_factor = 0;
	iv_attrs.attr_voltage_ext_vcs_bias = 0;
	iv_attrs.attr_voltage_ext_vdn_bias = 0;
	iv_attrs.attr_freq_bias_turbo = 0;
	iv_attrs.attr_voltage_ext_vdd_bias_turbo = 0;
	iv_attrs.attr_voltage_int_vdd_bias_turbo = 0;
	iv_attrs.attr_dpll_vdm_response = 0;
	iv_attrs.attr_system_wof_disable = 0;
	iv_attrs.attr_freq_bias_nominal = 0;
	iv_attrs.attr_voltage_ext_vdd_bias_nominal = 0;
	iv_attrs.attr_voltage_int_vdd_bias_nominal = 0;
	iv_attrs.vdd_rail_select = 0;
	iv_attrs.vdn_rail_select = 0;
	iv_attrs.attr_freq_bias_powersave = 0;
	iv_attrs.attr_proc_vrm_voffset_vcs_uv = 0;
	iv_attrs.attr_proc_vrm_voffset_vdd_uv = 0;
	iv_attrs.attr_proc_vrm_voffset_vdn_uv = 0;
	iv_attrs.attr_voltage_ext_vdd_bias_powersave = 0;
	iv_attrs.attr_voltage_int_vdd_bias_powersave = 0;
	iv_attrs.vrm_voffset_vcs_uv = 0;
	iv_attrs.vrm_voffset_vdd_uv = 0;
	iv_attrs.vrm_voffset_vdn_uv = 0;
	iv_attrs.attr_freq_bias_ultraturbo = 0;
	iv_attrs.attr_proc_r_distloss_vdd_uohm = 0;
	iv_attrs.attr_proc_r_loadline_vcs_uohm = 0;
	iv_attrs.attr_proc_r_loadline_vdn_uohm = 0;
	iv_attrs.attr_voltage_ext_vdd_bias_ultraturbo = 0;
	iv_attrs.attr_voltage_int_vdd_bias_ultraturbo = 0;
	iv_attrs.r_distloss_vdd_uohm = 0;
	iv_attrs.r_loadline_vcs_uohm = 0;
	iv_attrs.r_loadline_vdn_uohm = 0;
	iv_attrs.vcs_bus_num = 0;
	iv_attrs.vdd_bus_num = 0;
	iv_attrs.vcs_rail_select = 1;
	iv_attrs.vdn_bus_num = 1;
	iv_attrs.attr_proc_dpll_divider = 8;
	iv_attrs.attr_proc_r_distloss_vdn_uohm = 50;
	iv_attrs.r_distloss_vdn_uohm = 50;
	iv_attrs.attr_proc_r_distloss_vcs_uohm = 64;
	iv_attrs.r_distloss_vcs_uohm = 64;
	iv_attrs.attr_proc_r_loadline_vdd_uohm = 254;
	iv_attrs.r_loadline_vdd_uohm = 254;
	iv_attrs.attr_ext_vrm_stabilization_time_us = 0;
	iv_attrs.attr_ext_vrm_step_size_mv = 0;
	iv_attrs.attr_ext_vrm_transition_rate_dec_uv_per_us = 0;
	iv_attrs.attr_ext_vrm_transition_rate_inc_uv_per_us = 0;
	iv_attrs.attr_ext_vrm_transition_start_ns = 0;
	iv_attrs.attr_pstate_mode = 0;
	iv_attrs.attr_resclk_disable = 0;
	iv_attrs.attr_system_ivrm_disable = 0;
	iv_attrs.attr_system_vdm_disable = 0;
	iv_attrs.attr_wov_overv_disable = 0; // OFF
	iv_attrs.attr_wov_underv_disable = 0; // OFF
	iv_attrs.attr_wov_max_droop_pct = 0;
	iv_attrs.attr_wov_overv_max_pct = 0;
	iv_attrs.attr_wov_overv_step_decr_pct = 0; // default 5 is used
	iv_attrs.attr_wov_overv_step_incr_pct = 0; // default 5 is used
	iv_attrs.attr_wov_overv_vmax_mv = 0; // default 1150(1.15V) is used
	iv_attrs.attr_wov_sample_125us = 0; // default 2(~250us) is used
	iv_attrs.attr_wov_underv_force = 0; // OFF
	iv_attrs.attr_wov_underv_max_pct = 0; // default 10%(100) is used
	iv_attrs.attr_wov_underv_perf_loss_thresh_pct = 0; // default 0.5%(5) is used
	iv_attrs.attr_wov_underv_step_decr_pct = 0; // default 0.5%(5) is used
	iv_attrs.attr_wov_underv_step_incr_pct = 0; // default 0.5%(5) is used
	iv_attrs.attr_wov_underv_vmin_mv = 0;
	iv_attrs.attr_nest_leakage_percent = 0x3C;
	iv_attrs.freq_proc_refclock_khz = 133333;
	iv_attrs.attr_freq_dpll_refclock_khz = 133333
	iv_attrs.proc_dpll_divider = 8
	iv_attrs.vcs_voltage_mv = 0;
	iv_attrs.vdd_voltage_mv = 0;
	iv_attrs.vdn_voltage_mv = 0;
	iv_attrs.attr_dd_vdm_not_supported = CHIP_EC == 0x20;
	iv_attrs.attr_dd_wof_not_supported = CHIP_EC == 0x20;
	FAPI_ATTR_GET(fapi2::FREQ_CORE_FLOOR_MHZ, FAPI_SYSTEM, iv_attrs.attr_freq_core_floor_mhz);
	FAPI_ATTR_GET(fapi2::ATTR_FREQ_PB_MHZ, FAPI_SYSTEM, iv_attrs.attr_nest_frequency_mhz);
	FAPI_ATTR_GET(fapi2::ATTR_FREQ_CORE_CEILING_MHZ, FAPI_SYSTEM, iv_attrs.attr_freq_core_ceiling_mhz);
	FAPI_ATTR_GET(fapi2::ATTR_SAFE_MODE_FREQUENCY_MHZ,iv_procChip, iv_attrs.attr_pm_safe_frequency_mhz);
	FAPI_ATTR_GET(fapi2::ATTR_SAFE_MODE_VOLTAGE_MV, iv_procChip, iv_attrs.attr_pm_safe_voltage_mv);

	#define SET_DEFAULT(_attr_name, _attr_default) \
	if (!(iv_attrs._attr_name)) \
	{ \
	   iv_attrs._attr_name = _attr_default; \
	}

	SET_DEFAULT(attr_ext_vrm_transition_start_ns, 8000)
	SET_DEFAULT(attr_ext_vrm_transition_rate_inc_uv_per_us, 10000)
	SET_DEFAULT(attr_ext_vrm_transition_rate_dec_uv_per_us, 10000)
	SET_DEFAULT(attr_ext_vrm_stabilization_time_us, 5)
	SET_DEFAULT(attr_ext_vrm_step_size_mv, 50)
	SET_DEFAULT(attr_wov_sample_125us, 2);
	SET_DEFAULT(attr_wov_max_droop_pct, 125);
	SET_DEFAULT(attr_wov_overv_step_incr_pct, 5);
	SET_DEFAULT(attr_wov_overv_step_decr_pct, 5);
	SET_DEFAULT(attr_wov_overv_max_pct, 0);
	SET_DEFAULT(attr_wov_overv_vmax_mv, 1150);
	SET_DEFAULT(attr_wov_underv_step_incr_pct, 5);
	SET_DEFAULT(attr_wov_underv_step_decr_pct, 5);
	SET_DEFAULT(attr_wov_underv_max_pct, 100);
	SET_DEFAULT(attr_wov_underv_perf_loss_thresh_pct, 5);

	iv_vcs_sysparam.distloss_uohm = revle32(64);
	iv_vcs_sysparam.distoffset_uv = 0;
	iv_vcs_sysparam.loadline_uohm = 0;

	iv_vdd_sysparam.distloss_uohm = 0;
	iv_vdd_sysparam.distoffset_uv = 0;
	iv_vdd_sysparam.loadline_uohm = revle32(254);

	iv_vdn_sysparam.distloss_uohm = revle32(50);
	iv_vdn_sysparam.distoffset_uv = 0;
	iv_vdn_sysparam.loadline_uohm = 0;

	iv_vdmpb.droop_small_override = 0;
	iv_vdmpb.droop_large_override = 0;
	iv_vdmpb.droop_extreme_override = 0;
	iv_vdmpb.overvolt_override = 0;
	iv_vdmpb.fmin_override_khz = 0;
	iv_vdmpb.fmax_override_khz = 0;
	iv_vdmpb.vid_compare_override_mv = 0;
	iv_vdmpb.vdm_response = 0;

	iv_resclk_enabled  = true;
	iv_vdm_enabled     = true;
	iv_ivrm_enabled    = true;
	iv_wof_enabled     = true;
	iv_wov_underv_enabled = true;
	iv_wov_overv_enabled = true;
	iv_frequency_step_khz = 16666;
	iv_nest_freq_mhz      = iv_attrs.attr_nest_frequency_mhz;
}

uint32_t sysparm_uplift(const uint32_t i_vpd_mv,
						const uint32_t i_vpd_ma,
						const uint32_t i_loadline_uohm,
						const uint32_t i_distloss_uohm,
						const uint32_t i_distoffset_uohm)
{
	return (uint32_t)((double)i_vpd_mv + (((double)(i_vpd_ma * (i_loadline_uohm + i_distloss_uohm)) / 1000 + (double)i_distoffset_uohm)) / 1000);
}

double calc_bias(const int8_t i_value)
{
	return 1.0 + ((0.5 / 100) * (double)i_value);
}

uint32_t bias_adjust_mv(
	const uint32_t i_value,
	const int32_t i_bias_0p5pct)
{
	return (uint32_t)ceil((double)i_value * calc_bias(i_bias_0p5pct));
}

uint32_t bias_adjust_mhz(
	const uint32_t i_value,
	const int32_t i_bias_0p5pct)
{
	return ((uint32_t)floor((double)i_value * calc_bias(i_bias_0p5pct)));
}

static void PlatPmPPB::compute_vpd_pts(void)
{
	for (int p = 0; p < NUM_OP_POINTS; p++)
	{
		iv_operating_points[VPD_PT_SET_RAW][p].vdd_mv = iv_raw_vpd_pts[p].vdd_mv;
		iv_operating_points[VPD_PT_SET_RAW][p].vcs_mv = iv_raw_vpd_pts[p].vcs_mv;
		iv_operating_points[VPD_PT_SET_RAW][p].idd_100ma = iv_raw_vpd_pts[p].idd_100ma;
		iv_operating_points[VPD_PT_SET_RAW][p].ics_100ma = iv_raw_vpd_pts[p].ics_100ma;
		iv_operating_points[VPD_PT_SET_RAW][p].frequency_mhz = iv_raw_vpd_pts[p].frequency_mhz;
		iv_operating_points[VPD_PT_SET_RAW][p].pstate = iv_raw_vpd_pts[p].pstate;

		iv_operating_points[VPD_PT_SET_SYSP][p].vdd_mv = (uint32_t)((double)iv_raw_vpd_pts[p].vdd_mv + (((double)(iv_raw_vpd_pts[p].idd_100ma * 100 * (revle32(iv_vdd_sysparam.loadline_uohm) + revle32(iv_vdd_sysparam.distloss_uohm))) / 1000 + (double)revle32(iv_vdd_sysparam.distoffset_uv))) / 1000);
		iv_operating_points[VPD_PT_SET_SYSP][p].vcs_mv = (uint32_t)((double)iv_raw_vpd_pts[p].vcs_mv + (((double)(iv_raw_vpd_pts[p].ics_100ma * 100 * (revle32(iv_vcs_sysparam.loadline_uohm) + revle32(iv_vcs_sysparam.distloss_uohm))) / 1000 + (double)revle32(iv_vcs_sysparam.distoffset_uv))) / 1000);
		iv_operating_points[VPD_PT_SET_SYSP][p].idd_100ma = iv_raw_vpd_pts[p].idd_100ma;
		iv_operating_points[VPD_PT_SET_SYSP][p].ics_100ma = iv_raw_vpd_pts[p].ics_100ma;
		iv_operating_points[VPD_PT_SET_SYSP][p].frequency_mhz = iv_raw_vpd_pts[p].frequency_mhz;
		iv_operating_points[VPD_PT_SET_SYSP][p].pstate = iv_raw_vpd_pts[p].pstate;

		iv_operating_points[VPD_PT_SET_BIASED][p].vdd_mv = bias_adjust_mv(iv_raw_vpd_pts[p].vdd_mv, iv_bias[p].vdd_ext_hp);
		iv_operating_points[VPD_PT_SET_BIASED][p].vcs_mv = bias_adjust_mv(iv_raw_vpd_pts[p].vcs_mv, iv_bias[p].vcs_ext_hp);
		iv_operating_points[VPD_PT_SET_BIASED][p].frequency_mhz = bias_adjust_mhz(iv_raw_vpd_pts[p].frequency_mhz, iv_bias[p].frequency_hp);
		iv_operating_points[VPD_PT_SET_BIASED][p].idd_100ma = iv_biased_vpd_pts[p].idd_100ma;
		iv_operating_points[VPD_PT_SET_BIASED][p].ics_100ma = iv_biased_vpd_pts[p].ics_100ma;
	}

	iv_reference_frequency_khz = iv_operating_points[VPD_PT_SET_BIASED][ULTRA].frequency_mhz * 1000;

	for (int p = 0; p < NUM_OP_POINTS; p++)
	{
		iv_operating_points[VPD_PT_SET_BIASED][p].pstate = (((iv_operating_points[VPD_PT_SET_BIASED][ULTRA].frequency_mhz - iv_operating_points[VPD_PT_SET_BIASED][p].frequency_mhz) * 1000) / iv_frequency_step_khz);

		iv_operating_points[VPD_PT_SET_BIASED_SYSP][p].vdd_mv = sysparm_uplift(iv_operating_points[VPD_PT_SET_BIASED][p].vdd_mv, iv_operating_points[VPD_PT_SET_BIASED][p].idd_100ma * 100, revle32(iv_vdd_sysparam.loadline_uohm), revle32(iv_vdd_sysparam.distloss_uohm), revle32(iv_vdd_sysparam.distoffset_uv));
		iv_operating_points[VPD_PT_SET_BIASED_SYSP][p].vcs_mv = sysparm_uplift(iv_operating_points[VPD_PT_SET_BIASED][p].vcs_mv, iv_operating_points[VPD_PT_SET_BIASED][p].ics_100ma * 100, revle32(iv_vcs_sysparam.loadline_uohm), revle32(iv_vcs_sysparam.distloss_uohm), revle32(iv_vcs_sysparam.distoffset_uv));
		iv_operating_points[VPD_PT_SET_BIASED_SYSP][p].idd_100ma = iv_operating_points[VPD_PT_SET_BIASED][p].idd_100ma;
		iv_operating_points[VPD_PT_SET_BIASED_SYSP][p].ics_100ma = iv_operating_points[VPD_PT_SET_BIASED][p].ics_100ma;
		iv_operating_points[VPD_PT_SET_BIASED_SYSP][p].frequency_mhz = iv_operating_points[VPD_PT_SET_BIASED][p].frequency_mhz;
		iv_operating_points[VPD_PT_SET_BIASED_SYSP][p].pstate = iv_operating_points[VPD_PT_SET_BIASED][p].pstate;
	}
}

static void PlatPmPPB::safe_mode_computation(
	const Pstate i_ps_pstate,
	Safe_mode_parameters *o_safe_mode_values)
{
	const fapi2::Target<fapi2::TARGET_TYPE_SYSTEM> FAPI_SYSTEM;
	Safe_mode_parameters                     l_safe_mode_values;
	fapi2::ATTR_SAFE_MODE_FREQUENCY_MHZ_Type l_safe_mode_freq_mhz;
	fapi2::ATTR_SAFE_MODE_VOLTAGE_MV_Type    l_safe_mode_mv;
	fapi2::ATTR_VDD_BOOT_VOLTAGE_Type        l_boot_mv;
	uint32_t                                 l_safe_mode_op_ps2freq_mhz;
	uint32_t                                 l_core_floor_mhz;
	uint32_t                                 l_op_pt;


	FAPI_ATTR_GET(fapi2::ATTR_FREQ_CORE_FLOOR_MHZ, FAPI_SYSTEM, l_core_floor_mhz);
	FAPI_ATTR_GET(fapi2::ATTR_SAFE_MODE_FREQUENCY_MHZ, iv_procChip, l_safe_mode_freq_mhz);
	FAPI_ATTR_GET(fapi2::ATTR_SAFE_MODE_VOLTAGE_MV, iv_procChip, l_safe_mode_mv);
	FAPI_ATTR_GET(fapi2::ATTR_VDD_BOOT_VOLTAGE, iv_procChip, l_boot_mv);

	l_op_pt = (iv_operating_points[VPD_PT_SET_BIASED_SYSP][POWERSAVE].frequency_mhz);
	// Safe operational frequency is the MAX(core floor, VPD Powersave).
	// PowerSave is the lowest operational frequency that the part was tested at
	if (l_core_floor_mhz > l_op_pt)
	{
		l_safe_mode_values.safe_op_freq_mhz = l_core_floor_mhz;
	}
	else
	{
		l_safe_mode_values.safe_op_freq_mhz = l_op_pt;
	}

	// Calculate safe operational pstate.  This must be rounded down to create
	// a faster Pstate than the floor
	l_safe_mode_values.safe_op_ps = ((float)(iv_reference_frequency_khz) -
									 (float)(l_safe_mode_values.safe_op_freq_mhz * 1000)) /
									 (float)iv_frequency_step_khz;

	l_safe_mode_op_ps2freq_mhz    = (iv_reference_frequency_khz -
									(l_safe_mode_values.safe_op_ps * iv_frequency_step_khz)) / 1000;

	while (l_safe_mode_op_ps2freq_mhz < l_safe_mode_values.safe_op_freq_mhz)
	{
		l_safe_mode_values.safe_op_ps--;

		l_safe_mode_op_ps2freq_mhz =
			(iv_reference_frequency_khz - (l_safe_mode_values.safe_op_ps * iv_frequency_step_khz)) / 1000;
	}

	// Calculate safe jump value for large frequency
	l_safe_mode_values.safe_vdm_jump_value =
			large_jump_interpolate(l_safe_mode_values.safe_op_ps,
								   i_ps_pstate);

	// Calculate safe mode frequency - Round up to nearest MHz
	// The uplifted frequency is based on the fact that the DPLL percentage is a
	// "down" value. Hence:
	//     X (uplifted safe) = Y (safe operating) / (1 - droop percentage)
	l_safe_mode_values.safe_mode_freq_mhz = (uint32_t)
		(((float)l_safe_mode_op_ps2freq_mhz * 1000 /
		  (1 - (float)l_safe_mode_values.safe_vdm_jump_value/32) + 500) / 1000);

	if (l_safe_mode_freq_mhz)
	{
		l_safe_mode_values.safe_mode_freq_mhz = l_safe_mode_freq_mhz;
	}
	else
	{
		FAPI_ATTR_SET(fapi2::ATTR_SAFE_MODE_FREQUENCY_MHZ, iv_procChip, l_safe_mode_values.safe_mode_freq_mhz);
	}


	l_safe_mode_values.safe_mode_ps = ((float)(iv_reference_frequency_khz) -
									   (float)(l_safe_mode_values.safe_mode_freq_mhz * 1000)) /
									   (float)iv_frequency_step_khz;


	l_safe_mode_values.safe_mode_mv = ps2v_mv(l_safe_mode_values.safe_mode_ps);

	if (l_safe_mode_mv)
	{
		l_safe_mode_values.safe_mode_mv = l_safe_mode_mv;
	}
	else
	{
		FAPI_ATTR_SET(fapi2::ATTR_SAFE_MODE_VOLTAGE_MV, iv_procChip, l_safe_mode_values.safe_mode_mv);
	}


	fapi2::ATTR_SAFE_MODE_NOVDM_UPLIFT_MV_Type l_uplift_mv;
	FAPI_ATTR_GET(fapi2::ATTR_SAFE_MODE_NOVDM_UPLIFT_MV, iv_procChip, l_uplift_mv);

	l_safe_mode_values.boot_mode_mv = l_safe_mode_values.safe_mode_mv + l_uplift_mv;

	if (!iv_attrs.vdd_voltage_mv)
	{
		iv_attrs.vdd_voltage_mv = l_safe_mode_values.boot_mode_mv;
	}

	memcpy (o_safe_mode_values,&l_safe_mode_values, sizeof(Safe_mode_parameters));
}

uint32_t PlatPmPPB::large_jump_interpolate(const Pstate i_pstate,
			const Pstate i_ps_pstate)
{

	uint8_t l_jump_value_set_ps  = iv_poundW_data.poundw[POWERSAVE].vdm_normal_freq_drop & 0x0F;
	uint8_t l_jump_value_set_nom = iv_poundW_data.poundw[NOMINAL].vdm_normal_freq_drop & 0x0F;

	uint32_t l_slope_value = compute_slope_thresh(
		l_jump_value_set_nom,
		l_jump_value_set_ps,
		iv_operating_points[VPD_PT_SET_BIASED][POWERSAVE].pstate,
		iv_operating_points[VPD_PT_SET_BIASED][NOMINAL].pstate);

	uint32_t l_vdm_jump_value =
		(uint32_t)((int32_t)l_jump_value_set_ps + (((int32_t)l_slope_value *
						(i_ps_pstate - i_pstate)) >> THRESH_SLOPE_FP_SHIFT));
	return l_vdm_jump_value;
}

int16_t compute_slope_thresh(int32_t y1, int32_t y0, int32_t x1, int32_t x0)
{
	return (int16_t)
			(
				// Perform division using double for maximum precision
				// Store resulting slope in 4.12 Fixed-Pt format
				((double)(y1 - y0) / (double)(x1 - x0)) * (1 << THRESH_SLOPE_FP_SHIFT)
			);
}

uint32_t PlatPmPPB::ps2v_mv(const Pstate i_pstate)
{

	uint32_t region_start, region_end;
	const char* pv_op_str[NUM_OP_POINTS] = PV_OP_ORDER_STR;

	if(i_pstate > iv_operating_points[VPD_PT_SET_BIASED_SYSP][NOMINAL].pstate)
	{
		region_start = POWERSAVE;
		region_end = NOMINAL;
	}
	else if(i_pstate > iv_operating_points[VPD_PT_SET_BIASED_SYSP][TURBO].pstate)
	{
		region_start = NOMINAL;
		region_end = TURBO;
	}
	else
	{
		region_start = TURBO;
		region_end = ULTRA;
	}

	uint32_t l_SlopeValue =
		compute_slope_4_12((iv_operating_points[VPD_PT_SET_BIASED_SYSP][region_end].vdd_mv),
						   (iv_operating_points[VPD_PT_SET_BIASED_SYSP][region_start].vdd_mv),
						   iv_operating_points[VPD_PT_SET_BIASED_SYSP][region_start].pstate,
						   iv_operating_points[VPD_PT_SET_BIASED_SYSP][region_end].pstate);

	uint32_t x = l_SlopeValue * (-i_pstate + iv_operating_points[VPD_PT_SET_BIASED_SYSP][region_start].pstate);
	uint32_t y = x >> VID_SLOPE_FP_SHIFT_12;

	uint32_t l_vdd =
		(((l_SlopeValue * (-i_pstate + iv_operating_points[VPD_PT_SET_BIASED_SYSP][region_start].pstate)) >> VID_SLOPE_FP_SHIFT_12)
		   + (iv_operating_points[VPD_PT_SET_BIASED_SYSP][region_start].vdd_mv));

	// Round up
	l_vdd = (l_vdd << 1) + 1;
	l_vdd = l_vdd >> 1;

	return l_vdd;
}

static void PlatPmPPB::safe_mode_init(void)
{
	uint8_t l_ps_pstate = 0;
	Safe_mode_parameters l_safe_mode_values;
	uint32_t l_ps_freq_khz =
	iv_operating_points[VPD_PT_SET_BIASED][POWERSAVE].frequency_mhz * 1000;

	if (!iv_attrs.attr_pm_safe_voltage_mv &&
			!iv_attrs.attr_pm_safe_frequency_mhz)
	{
		freq2pState(l_ps_freq_khz, &l_ps_pstate);

		safe_mode_computation(l_ps_pstate, &l_safe_mode_values);

		FAPI_ATTR_GET(fapi2::ATTR_SAFE_MODE_FREQUENCY_MHZ, iv_procChip,iv_attrs.attr_pm_safe_frequency_mhz );
		FAPI_ATTR_GET(fapi2::ATTR_SAFE_MODE_VOLTAGE_MV, iv_procChip, iv_attrs.attr_pm_safe_voltage_mv);

	}
}

int PlatPmPPB::freq2pState (
	const uint32_t i_freq_khz,
	Pstate* o_pstate)
{
	float pstate32 = ((float)(iv_reference_frequency_khz - (float)i_freq_khz)) / (float)iv_frequency_step_khz;
	// @todo Bug fix from Characterization team to deal with VPD not being
	// exactly in step increments
	//       - not yet included to separate changes
	// As higher Pstate numbers represent lower frequencies, the pstate must be
	// snapped to the nearest *higher* integer value for safety.  (e.g. slower
	// frequencies are safer).
	if (i_freq_khz)
	{
		*o_pstate = (Pstate)ceil(pstate32);
	}
	else
	{
		*o_pstate = (Pstate)pstate32;
	}
	if (pstate32 < 0)
	{
		*o_pstate = 0;
	}
	else if (pstate32 > 255)
	{
		*o_pstate = 255;
	}
}

void PlatPmPPB::compute_boot_safe()
{
	iv_raw_vpd_pts = 0;
	iv_biased_vpd_pts = 0;
	iv_poundW_data = 0;
	iv_iddqt = 0;
	iv_operating_points = 0;
	iv_attr_mvpd_poundV_raw = 0;
	iv_attr_mvpd_poundV_biased = 0;

	iv_attr_mvpd_poundV_raw[].frequency_mhz = {0};
	iv_attr_mvpd_poundV_raw[].vdd_mv        = {0};
	iv_attr_mvpd_poundV_raw[].idd_100ma     = {0};
	iv_attr_mvpd_poundV_raw[].vcs_mv        = {0};
	iv_attr_mvpd_poundV_raw[].ics_100ma     = {0};

	iv_bias[].frequency_hp    = {0};
	iv_bias[].vdd_ext_hp      = {0};
	iv_bias[].vdd_int_hp      = {0};
	iv_bias[].vdn_ext_hp      = {0};
	iv_bias[].vcs_ext_hp      = {0};

	vpd_init();
	compute_vpd_pts();
	safe_mode_init();

	iv_attrs.vcs_voltage_mv = (uint32_t)((double)iv_attr_mvpd_poundV_biased[POWERSAVE].vcs_mv
		+ (double)(iv_attr_mvpd_poundV_biased[POWERSAVE].ics_100ma * revle32(64)) / 10000);
	iv_attrs.vdn_voltage_mv = (uint32_t)((double)iv_attr_mvpd_poundV_biased[POWERBUS].vdd_mv
		+ (double)(iv_attr_mvpd_poundV_biased[POWERBUS].idd_100ma * revle32(50)) / 10000)

	FAPI_ATTR_SET(fapi2::ATTR_VDD_BOOT_VOLTAGE, iv_procChip, iv_attrs.vdd_voltage_mv);
	FAPI_ATTR_SET(fapi2::ATTR_VCS_BOOT_VOLTAGE, iv_procChip, iv_attrs.vcs_voltage_mv);
	FAPI_ATTR_SET(fapi2::ATTR_VDN_BOOT_VOLTAGE, iv_procChip, iv_attrs.vdn_voltage_mv);
}

static void p9_setup_dpll_values(chiplet_id_t i_target)
{
	std::vector<fapi2::Target<fapi2::TARGET_TYPE_EQ>> l_eqChiplets;
	fapi2::Target<fapi2::TARGET_TYPE_EQ> l_firstEqChiplet;
	l_eqChiplets = i_target.getChildren<fapi2::TARGET_TYPE_EQ>(fapi2::TARGET_STATE_FUNCTIONAL);
	fapi2::buffer<uint64_t> l_data64;
	fapi2::ATTR_SAFE_MODE_FREQUENCY_MHZ_Type l_attr_safe_mode_freq;
	fapi2::ATTR_SAFE_MODE_VOLTAGE_MV_Type l_attr_safe_mode_mv;
	FAPI_ATTR_GET(fapi2::ATTR_SAFE_MODE_FREQUENCY_MHZ, i_target, l_attr_safe_mode_freq);
	FAPI_ATTR_GET(fapi2::ATTR_SAFE_MODE_VOLTAGE_MV, i_target, l_attr_safe_mode_mv);
	if (!l_attr_safe_mode_freq || !l_attr_safe_mode_mv)
	{
		return;
	}
	for (auto l_itr = l_eqChiplets.begin(); l_itr != l_eqChiplets.end(); ++l_itr)
	{
		fapi2::getScom(*l_itr, EQ_QPPM_DPLL_FREQ , l_data64);
		uint32_t l_safe_mode_dpll_value = l_attr_safe_mode_freq * 8000 / 133333;
		l_data64.insertFromRight<1, 11>(l_safe_mode_dpll_value);
		l_data64.insertFromRight<17, 11>(l_safe_mode_dpll_value);
		l_data64.insertFromRight<33, 11>(l_safe_mode_dpll_value);
		fapi2::putScom(*l_itr, EQ_QPPM_DPLL_FREQ, l_data64);

		fapi2::getScom(*l_itr, EQ_QPPM_VDMCFGR, l_data64);
		l_data64.insertFromRight<0, 8>((l_attr_safe_mode_mv - 512) / 4);
		fapi2::putScom(*l_itr, EQ_QPPM_VDMCFGR, l_data64);
	}
}

void p9_setup_evid(chiplet_id_t proc_chip_target)
{
	pm_pstate_parameter_block::AttributeList attrs;
	PlatPmPPB l_pmPPB(proc_chip_target);
	l_pmPPB.attr_init();
	l_pmPPB.compute_boot_safe();
	memcpy(&attrs ,&iv_attrs, sizeof(iv_attrs));
	p9_setup_dpll_values(proc_chip_target);

	if (attrs.vdd_voltage_mv)
	{
		p9_setup_evid_voltageWrite(proc_chip_target, 0, 0, attrs.vdd_voltage_mv, VDD_SETUP); // VDD_SETUP = 6
	}
	if (attrs.vdn_voltage_mv)
	{
		p9_setup_evid_voltageWrite(proc_chip_target, 1, 0, attrs.vdn_voltage_mv, VDN_SETUP); // VDN_SETUP = 7
	}
	if (attrs.vcs_voltage_mv)
	{
		p9_setup_evid_voltageWrite(proc_chip_target, 0, 1, attrs.vcs_voltage_mv, VCS_SETUP); // VCS_SETUP = 8
	}
}

void p9_setup_evid_voltageWrite(
	chiplet_id_t proc_chip_target,
	const uint8_t i_bus_num,
	const uint8_t i_rail_select,
	const uint32_t i_voltage_mv,
	const P9_SETUP_EVID_CONSTANTS i_evid_value)

{
	uint8_t     l_goodResponse = 0;
	uint8_t     l_throwAssert = true;
	uint32_t    l_present_voltage_mv;
	uint32_t    l_target_mv;
	uint32_t    l_count;
	int32_t     l_delta_mv = 0;
	// VCS_SETUP = 8
	if (i_evid_value != VCS_SETUP)
	{
		avsInitExtVoltageControl(proc_chip_target, i_bus_num, BRIDGE_NUMBER);
	}

	avsIdleFrame(proc_chip_target, i_bus_num, BRIDGE_NUMBER);

	l_count = 0;
	do
	{
		fapi2::buffer<uint64_t> l_data64;
		// Drive a Read Command
		avsDriveCommand(
			proc_chip_target,
			i_bus_num,
			BRIDGE_NUMBER,
			i_rail_select,
			3,
			0xFFFF);
		// Read returned voltage value from Read frame
		getScom(proc_chip_target, p9avslib::OCB_O2SRD[i_bus_num][BRIDGE_NUMBER], l_data64);
		// Extracting bits 8:23 , which contains voltage read data
		l_present_voltage_mv = (l_data64 & 0x00FFFF0000000000) >> 40;
		// Throw an assertion if we don't get a good response.
		l_throwAssert = l_count >= AVSBUS_RETRY_COUNT;
		avsValidateResponse(proc_chip_target, i_bus_num, BRIDGE_NUMBER, l_throwAssert, l_goodResponse);
		if (!l_goodResponse)
		{
			avsIdleFrame(proc_chip_target, i_bus_num, BRIDGE_NUMBER);
		}
		l_count++;
	}
	while (!l_goodResponse);

	// Compute the delta
	l_delta_mv = l_present_voltage_mv - i_voltage_mv;

	// Break into steps limited by attr.attr_ext_vrm_step_size_mv
	while (l_delta_mv)
	{
		uint32_t l_abs_delta_mv = abs(l_delta_mv);
		if (50 > 0 && l_abs_delta_mv > 50)
		{
			if (l_delta_mv > 0)  // Decreasing
			{
				l_target_mv = l_present_voltage_mv - 50;
			}
			else
			{
				l_target_mv = l_present_voltage_mv + 50;
			}
		}
		else
		{
			l_target_mv = i_voltage_mv;
		}
		l_count = 0;

		do
		{
			// Set Boot voltage
			avsDriveCommand(
			proc_chip_target,
			i_bus_num,
			BRIDGE_NUMBER,
			i_rail_select,
			0,
			l_target_mv);

			// Throw an assertion if we don't get a good response.
			l_throwAssert =  l_count >= AVSBUS_RETRY_COUNT;
			avsValidateResponse(
				proc_chip_target,
				i_bus_num,
				BRIDGE_NUMBER,
				l_throwAssert,
				l_goodResponse);
			if (!l_goodResponse)
			{
				avsIdleFrame(proc_chip_target, i_bus_num, BRIDGE_NUMBER);
			}
			l_count++;
		}
		while (!l_goodResponse);
	}
}

#define NUM_OP_POINTS      4
#define VPD_PV_POWERSAVE   1
#define VPD_PV_NOMINAL     0
#define VPD_PV_TURBO       2
#define VPD_PV_ULTRA       3
#define VPD_PV_POWERBUS    4

void PlatPmPPB::get_extint_bias()
{
	memcpy(iv_attr_mvpd_poundV_biased, iv_attr_mvpd_poundV_raw, sizeof(iv_attr_mvpd_poundV_raw));

	for (auto p = 0; p < NUM_OP_POINTS; p++)
	{
		iv_attr_mvpd_poundV_raw[p].frequency_mhz = (uint16_t)floor(iv_attr_mvpd_poundV_raw[p].frequency_mhz);
		iv_attr_mvpd_poundV_raw[p].vdd_mv        = (uint16_t)ceil(iv_attr_mvpd_poundV_raw[p].vdd_mv);
		iv_attr_mvpd_poundV_raw[p].vcs_mv        = (uint16_t)iv_attr_mvpd_poundV_raw[p].vcs_mv;
	}
	iv_attr_mvpd_poundV_biased[VPD_PV_POWERBUS].vdd_mv = (uint32_t)ceil((double)iv_attr_mvpd_poundV_raw[VPD_PV_POWERBUS].vdd_mv);
}

static void PlatPmPPB::validate_quad_spec_data(void)
{
	uint32_t l_vdm_compare_raw_mv[NUM_OP_POINTS];
	uint32_t l_vdm_compare_biased_mv[NUM_OP_POINTS];

	for(size_t eq_chiplet_unit_pos = 0; eq_chiplet_unit_pos < 5; ++eq_chiplet_unit_pos)
	{
		if (!(iv_poundW_data.poundw[NOMINAL].vdm_vid_compare_per_quad[eq_chiplet_unit_pos])
		&& !(iv_poundW_data.poundw[POWERSAVE].vdm_vid_compare_per_quad[eq_chiplet_unit_pos])
		&& !(iv_poundW_data.poundw[TURBO].vdm_vid_compare_per_quad[eq_chiplet_unit_pos])
		&& !(iv_poundW_data.poundw[ULTRA].vdm_vid_compare_per_quad[eq_chiplet_unit_pos]))
		{
			iv_poundW_data.poundw[NOMINAL].vdm_vid_compare_per_quad[eq_chiplet_unit_pos] = (iv_poundV_raw_data.VddNomVltg - 512) / 4;
			iv_poundW_data.poundw[POWERSAVE].vdm_vid_compare_per_quad[eq_chiplet_unit_pos] = (iv_poundV_raw_data.VddPSVltg - 512) / 4;
			iv_poundW_data.poundw[TURBO].vdm_vid_compare_per_quad[eq_chiplet_unit_pos] = (iv_poundV_raw_data.VddTurboVltg - 512) / 4;
			iv_poundW_data.poundw[ULTRA].vdm_vid_compare_per_quad[eq_chiplet_unit_pos] = (iv_poundV_raw_data.VddUTurboVltg - 512) / 4;
		}
		else if (!(iv_poundW_data.poundw[NOMINAL].vdm_vid_compare_per_quad[eq_chiplet_unit_pos]) ||
				 !(iv_poundW_data.poundw[POWERSAVE].vdm_vid_compare_per_quad[eq_chiplet_unit_pos]) ||
				 !(iv_poundW_data.poundw[TURBO].vdm_vid_compare_per_quad[eq_chiplet_unit_pos]) ||
				 !(iv_poundW_data.poundw[ULTRA].vdm_vid_compare_per_quad[eq_chiplet_unit_pos]))
		{
			iv_vdm_enabled = false;
			return;
		}

		iv_vdm_enabled = !(
			iv_poundW_data.poundw[POWERSAVE].vdm_vid_compare_per_quad[eq_chiplet_unit_pos] <= iv_poundW_data.poundw[NOMINAL].vdm_vid_compare_per_quad[eq_chiplet_unit_pos]
			&& iv_poundW_data.poundw[NOMINAL].vdm_vid_compare_per_quad[eq_chiplet_unit_pos] <= iv_poundW_data.poundw[TURBO].vdm_vid_compare_per_quad[eq_chiplet_unit_pos]
			&& iv_poundW_data.poundw[TURBO].vdm_vid_compare_per_quad[eq_chiplet_unit_pos] <= iv_poundW_data.poundw[ULTRA].vdm_vid_compare_per_quad[eq_chiplet_unit_pos]);
		if(!iv_vdm_enabled)
		{
			return;
		}

		for (uint8_t i = 0; i < NUM_OP_POINTS; i++)
		{
			l_vdm_compare_biased_mv[i] = 512 + (iv_poundW_data.poundw[i].vdm_vid_compare_per_quad[eq_chiplet_unit_pos] << 2);
			if (l_vdm_compare_biased_mv[i] < 576)
			{
				l_vdm_compare_biased_mv[i] = 576;
			}
			iv_poundW_data.poundw[i].vdm_vid_compare_per_quad[eq_chiplet_unit_pos] = (l_vdm_compare_biased_mv[i] - 512) >> 2;
		}
	}
}

double ceil(double x)
{
	if ((x - (int)(x)) > 0)
	{
		return (int)x + 1;
	}
	return ((int)x);
}

double floor(double x)
{
	if(x >= 0)
	{
		return (int)x;
	}
	return (int)(x - 0.9999999999999999);
}

void PlatPmPPB::get_mvpd_poundW (void)
{
	// Exit if both VDM and WOF is disabled
	if (!(CHIPE_EC > 0x20 && iv_vdm_enabled) && !(CHIP_EC > 0x20 && iv_wof_enabled))
	{
		iv_vdm_enabled = false;
		iv_wof_enabled = false;
		iv_wov_underv_enabled = false;
		iv_wov_overv_enabled = false;
		break;
	}
	std::vector<fapi2::Target<fapi2::TARGET_TYPE_EQ>> l_eqChiplets;
	l_eqChiplets = iv_procChip.getChildren<fapi2::TARGET_TYPE_EQ>(fapi2::TARGET_STATE_FUNCTIONAL);


	fapi2::vdmData_t l_vdmBuf;
	memset(&l_vdmBuf, 0, sizeof(l_vdmBuf));
	memset(&l_vdmBuf, 0, sizeof(l_vdmBuf));
	PoundW_data_per_quad l_poundwPerQuad;
	PoundW_data          l_poundw;
	memset(&l_poundw, 0, sizeof(PoundW_data));
	memset(&l_poundwPerQuad, 0, sizeof(PoundW_data_per_quad));

	l_poundwPerQuad.poundw[].ivdd_tdp_ac_current_10ma      = 0;
	l_poundwPerQuad.poundw[].ivdd_tdp_dc_current_10ma      = 0;
	l_poundwPerQuad.poundw[].vdm_overvolt_small_thresholds = 0;
	l_poundwPerQuad.poundw[].vdm_large_extreme_thresholds  = 0;
	l_poundwPerQuad.poundw[].vdm_normal_freq_drop          = 0;
	l_poundwPerQuad.poundw[].vdm_normal_freq_return        = 0;
	l_poundwPerQuad.poundw[].vdm_vid_compare_per_quad[0]   = 0;
	l_poundwPerQuad.poundw[].vdm_vid_compare_per_quad[1]   = 0;
	l_poundwPerQuad.poundw[].vdm_vid_compare_per_quad[2]   = 0;
	l_poundwPerQuad.poundw[].vdm_vid_compare_per_quad[3]   = 0;
	l_poundwPerQuad.poundw[].vdm_vid_compare_per_quad[4]   = 0;
	l_poundwPerQuad.poundw[].vdm_vid_compare_per_quad[5]   = 0;

	const uint32_t LEGACY_RESISTANCE_ENTRY_SIZE =  10;
	memcpy(&l_poundwPerQuad.resistance_data, &l_poundw.resistance_data , LEGACY_RESISTANCE_ENTRY_SIZE);
	l_poundwPerQuad.resistance_data.r_undervolt_allowed = l_poundw.undervolt_tested;
	memset(&l_vdmBuf.vdmData, 0, sizeof(l_vdmBuf.vdmData));
	memcpy(&l_vdmBuf.vdmData, &l_poundwPerQuad, sizeof(PoundW_data_per_quad));

	if (iv_poundV_bucket_id == l_vdmBuf.bucketId)
	{
		break;
	}

	memcpy(&iv_poundW_data, l_vdmBuf.vdmData, sizeof(l_vdmBuf.vdmData));
	memcpy(&(iv_poundW_data.poundw[VPD_PV_NOMINAL]), &iv_poundW_data.poundw[VPD_PV_POWERSAVE], sizeof(poundw_entry_per_quad_t));
	memcpy(&(iv_poundW_data.poundw[VPD_PV_POWERSAVE]), &iv_poundW_data.poundw[VPD_PV_NOMINAL], sizeof(poundw_entry_per_quad_t));

	// If the #W version is less than 3, validate Turbo VDM large threshold
	// not larger than -32mV. This filters out parts that have bad VPD.  If
	// this check fails, log a recovered error, mark the VDMs disabled and
	// break out of the reset of the checks.
	uint32_t turbo_vdm_large_threshhold = (iv_poundW_data.poundw[TURBO].vdm_large_extreme_thresholds >> 4) & 0x0F;
	if (l_vdmBuf.version < FULLY_VALID_POUNDW_VERSION && g_GreyCodeIndexMapping[turbo_vdm_large_threshhold] > GREYCODE_INDEX_M32MV)
	{
		iv_vdm_enabled = false;
		break;
	}

	// Validate the WOF content is non-zero if WOF is enabled
	if (CHIP_EC > 0x20 && iv_wof_enabled)
	{
		bool b_tdp_ac = true;
		bool b_tdp_dc = true;
		// Check that the WOF currents are non-zero
		for (auto p = 0; p < NUM_OP_POINTS; ++p)
		{
			if (!iv_poundW_data.poundw[p].ivdd_tdp_ac_current_10ma)
			{
				b_tdp_ac = false;
				iv_wof_enabled = false;

			}
			if (!iv_poundW_data.poundw[p].ivdd_tdp_dc_current_10ma)
			{
				b_tdp_dc = false;
				iv_wof_enabled = false;
			}
		}

		// Set the return code to success to keep going.
		fapi2::current_err = fapi2::FAPI2_RC_SUCCESS;
	}

	// The rest of the processing here is all checking of the VDM content
	// within #W.  If VDMs are not enabled (or supported), skip all of it
	if (CHIPE_EC < 0x20)
	{
		iv_vdm_enabled = false;
		return;
	}

	validate_quad_spec_data();

	if((((iv_poundW_data.poundw[].vdm_overvolt_small_thresholds >> 4) & 0x0F) > 0x7 && (iv_poundW_data.poundw[p].vdm_overvolt_small_thresholds >> 4) & 0x0F != 0xC)
	|| (((iv_poundW_data.poundw[].vdm_overvolt_small_thresholds) & 0x0F) == 8)
	|| (((iv_poundW_data.poundw[].vdm_overvolt_small_thresholds) & 0x0F) == 9)
	|| (((iv_poundW_data.poundw[].vdm_overvolt_small_thresholds) & 0x0F) > 0xF)
	|| (((iv_poundW_data.poundw[].vdm_large_extreme_thresholds >> 4) & 0x0F) == 8)
	|| (((iv_poundW_data.poundw[].vdm_large_extreme_thresholds >> 4) & 0x0F) == 9)
	|| (((iv_poundW_data.poundw[].vdm_large_extreme_thresholds >> 4) & 0x0F) > 0xF)
	|| (((iv_poundW_data.poundw[].vdm_large_extreme_thresholds) & 0x0F) == 8)
	|| (((iv_poundW_data.poundw[].vdm_large_extreme_thresholds) & 0x0F) == 9)
	|| (((iv_poundW_data.poundw[].vdm_large_extreme_thresholds) & 0x0F) > 0xF)
	|| (iv_poundW_data.poundw[].vdm_normal_freq_drop) & 0x0F > 7
	|| (iv_poundW_data.poundw[].vdm_normal_freq_drop >> 4) & 0x0F > iv_poundW_data.poundw[p].vdm_normal_freq_drop & 0x0F
	|| (iv_poundW_data.poundw[].vdm_normal_freq_return >> 4) & 0x0F
		> iv_poundW_data.poundw[].vdm_normal_freq_drop & 0x0F - iv_poundW_data.poundw[p].vdm_normal_freq_return & 0x0F
	|| (iv_poundW_data.poundw[].vdm_normal_freq_return & 0x0F > (iv_poundW_data.poundw[p].vdm_normal_freq_drop >> 4) & 0x0F)
	|| (((iv_poundW_data.poundw[].vdm_normal_freq_drop & 0x0F)
		| (iv_poundW_data.poundw[].vdm_normal_freq_drop >> 4) & 0x0F
		| (iv_poundW_data.poundw[].vdm_normal_freq_return >> 4) & 0x0F
		| iv_poundW_data.poundw[].vdm_normal_freq_return & 0x0F) == 0))
	{
		iv_vdm_enabled = false;
	}

	iv_wov_underv_enabled =
		(iv_poundW_data.resistance_data.r_undervolt_allowed || iv_attrs.attr_wov_underv_force)
		&& !(iv_attrs.attr_wov_underv_disable)
		&& iv_wov_underv_enabled;

	if (!iv_vdm_enabled)
	{
		iv_poundW_data.poundw[POWERSAVE].vdm_normal_freq_drop = 4;
		iv_poundW_data.poundw[NOMINAL].vdm_normal_freq_drop = 4;
	}
}

void PlatPmPPB::vpd_init(void)
{
	fapi2::voltageBucketData_t l_fstChlt_vpd_data;
	iv_poundV_bucket_id = iv_poundV_raw_data.bucketId;
	memcpy(&l_fstChlt_vpd_data, &iv_poundV_raw_data, sizeof(iv_poundV_raw_data));

	if ((iv_poundV_raw_data.VddNomVltg    > l_fstChlt_vpd_data.VddNomVltg)
	||	(iv_poundV_raw_data.VddPSVltg     > l_fstChlt_vpd_data.VddPSVltg)
	||	(iv_poundV_raw_data.VddTurboVltg  > l_fstChlt_vpd_data.VddTurboVltg)
	||	(iv_poundV_raw_data.VddUTurboVltg > l_fstChlt_vpd_data.VddUTurboVltg)
	||	(iv_poundV_raw_data.VdnPbVltg     > l_fstChlt_vpd_data.VdnPbVltg) )
	{
		memcpy(&l_fstChlt_vpd_data, &iv_poundV_raw_data, sizeof(iv_poundV_raw_data));
		iv_poundV_bucket_id = iv_poundV_raw_data.bucketId;
	}

	if(!iv_procChip.getChildren<fapi2::TARGET_TYPE_EQ>(fapi2::TARGET_STATE_FUNCTIONAL).size())
	{
		break;
	}

	get_extint_bias();
	get_mvpd_poundW();
	iv_wof_enabled = false;
	load_mvpd_operating_point();
}

#define NUM_OP_POINTS      4
#define VPD_PV_POWERSAVE   1
#define VPD_PV_NOMINAL     0
#define VPD_PV_TURBO       2
#define VPD_PV_ULTRA       3
#define VPD_PV_POWERBUS    4

void PlatPmPPB::load_mvpd_operating_point(void)
{
	const uint8_t pv_op_order[NUM_OP_POINTS] = {VPD_PV_POWERSAVE, VPD_PV_NOMINAL, VPD_PV_TURBO, VPD_PV_ULTRA};
	for (uint32_t i = 0; i < NUM_OP_POINTS; i++)
	{
		iv_raw_vpd_pts[i].frequency_mhz  = iv_attr_mvpd_poundV_raw[pv_op_order[i]].frequency_mhz;
		iv_raw_vpd_pts[i].vdd_mv         = iv_attr_mvpd_poundV_raw[pv_op_order[i]].vdd_mv;
		iv_raw_vpd_pts[i].idd_100ma      = iv_attr_mvpd_poundV_raw[pv_op_order[i]].idd_100ma;
		iv_raw_vpd_pts[i].vcs_mv         = iv_attr_mvpd_poundV_raw[pv_op_order[i]].vcs_mv;
		iv_raw_vpd_pts[i].ics_100ma      = iv_attr_mvpd_poundV_raw[pv_op_order[i]].ics_100ma;
		iv_raw_vpd_pts[i].pstate         = iv_attr_mvpd_poundV_raw[ULTRA].frequency_mhz
			- iv_attr_mvpd_poundV_raw[pv_op_order[i]].frequency_mhz * 1000 / 16666;

		iv_biased_vpd_pts[i].frequency_mhz  = iv_attr_mvpd_poundV_biased[pv_op_order[i]].frequency_mhz;
		iv_biased_vpd_pts[i].vdd_mv         = iv_attr_mvpd_poundV_biased[pv_op_order[i]].vdd_mv;
		iv_biased_vpd_pts[i].idd_100ma      = iv_attr_mvpd_poundV_biased[pv_op_order[i]].idd_100ma;
		iv_biased_vpd_pts[i].vcs_mv         = iv_attr_mvpd_poundV_biased[pv_op_order[i]].vcs_mv;
		iv_biased_vpd_pts[i].ics_100ma      = iv_attr_mvpd_poundV_biased[pv_op_order[i]].ics_100ma;
		iv_biased_vpd_pts[i].pstate         = iv_attr_mvpd_poundV_biased[ULTRA].frequency_mhz
			- iv_attr_mvpd_poundV_biased[pv_op_order[i]].frequency_mhz * 1000 / 16666;
	}
}

enum avslibconstants
{
	// AVSBUS_FREQUENCY specified in Khz, Default value 10 MHz
	MAX_POLL_COUNT_AVS = 0x1000,
	AVS_CRC_DATA_MASK = 0xfffffff8,
	O2S_FRAME_SIZE = 0x20,
	O2S_IN_DELAY1 = 0x3F,
	AVSBUS_FREQUENCY = 0x2710
};

const uint32_t OCB_O2SCTRLS[2][2] =
{
	OCB_O2SCTRLS0A,
	OCB_O2SCTRLS0B,
	OCB_O2SCTRLS1A,
	OCB_O2SCTRLS1B
};

const uint32_t OCB_O2SCTRL1[2][2] =
{
	OCB_O2SCTRL10A,
	OCB_O2SCTRL10B,
	OCB_O2SCTRL11A,
	OCB_O2SCTRL11B
};


const uint32_t OCB_O2SCTRL2[2][2] =
{
	OCB_O2SCTRL20A,
	OCB_O2SCTRL20B,
	OCB_O2SCTRL21A,
	OCB_O2SCTRL21B
};

void avsInitExtVoltageControl(
	chiplet_id_t proc_target,
	const uint8_t i_avsBusNum,
	const uint8_t i_o2sBridgeNum)
{
	write_scom_for_chiplet(
		proc_target,
		p9avslib::OCB_O2SCTRLF[i_avsBusNum][i_o2sBridgeNum],
		PPC_BIT(0)
		| PPC_BIT(6)
		| PPC_BITMASK(12, 17));
	write_scom_for_chiplet(
		proc_target,
		OCB_O2SCTRLS[i_avsBusNum][i_o2sBridgeNum],
		PPC_BIT(16));
	write_scom_for_chiplet(
		proc_target,
		p9avslib::OCB_O2SCTRL1[i_avsBusNum][i_o2sBridgeNum],
		PPC_BIT(0)
		| PPC_BIT(3)
		| ((ATTR_FREQ_PB_MHZ / 80 - 1) & 0x3FF) << 50
		| PPC_BIT(17));
	write_scom_for_chiplet(
		proc_target,
		p9avslib::OCB_O2SCTRL2[i_avsBusNum][i_o2sBridgeNum],
		0);
}

static void avsIdleFrame(
	chiplet_id_t proc_target,
	const uint8_t i_avsBusNum,
	const uint8_t i_o2sBridgeNum)
{
	// clear sticky bits in o2s_status_reg
	write_scom_for_chiplet(proc_target, p9avslib::OCB_O2SCMD[i_avsBusNum][i_o2sBridgeNum], PPPC_BIT(1));
	// Send the idle frame
	write_scom_for_chiplet(proc_target, p9avslib::OCB_O2SWD[i_avsBusNum][i_o2sBridgeNum], 0xFFFFFFFFFFFFFFFF);
	// Wait on o2s_ongoing = 0
	avsPollVoltageTransDone(proc_target, i_avsBusNum, i_o2sBridgeNum);
}

static void
avsPollVoltageTransDone(
	chiplet_id_t proc_target,
	const uint8_t i_avsBusNum,
	const uint8_t i_o2sBridgeNum)
{
	uint64_t ocb_o2sst;

	for(uint8_t l_count = 0; l_count < 0x1000; ++l_count)
	{
		ocb_o2sst = read_scom_for_chiplet(proc_target, p9avslib::OCB_O2SST[i_avsBusNum][i_o2sBridgeNum]);
		if (!(ocb_o2sst & PPC_BIT(0)))
		{
			return;
		}
	}
}

static void avsVoltageRead(
	chiplet_id_t proc_target,
	const uint8_t i_bus_num,
	const uint8_t BRIDGE_NUMBER,
	const uint32_t i_rail_select,
	uint32_t& l_present_voltage_mv)
{
	fapi2::buffer<uint64_t> l_data64;
	// Drive a Read Command
	avsDriveCommand(
		proc_target,
		i_bus_num,
		BRIDGE_NUMBER,
		i_rail_select,
		3,
		0xFFFF);

	// Read returned voltage value from Read frame
	getScom(proc_target, p9avslib::OCB_O2SRD[i_avsBusNum][i_o2sBridgeNum], l_data64);
	// Extracting bits 8:23 , which contains voltage read data
	l_present_voltage_mv = (l_data64 & 0x00FFFF0000000000) >> 40;
}

static void avsValidateResponse(
	chiplet_id_t proc_target,
	const uint8_t i_avsBusNum,
	const uint8_t i_o2sBridgeNum,
	const uint8_t i_throw_assert,
	uint8_t& o_goodResponse)
{
	fapi2::buffer<uint64_t> l_data64;
	fapi2::buffer<uint32_t> l_rsp_rcvd_crc;
	fapi2::buffer<uint8_t>  l_data_status_code;
	fapi2::buffer<uint32_t> l_rsp_data;

	uint32_t l_rsp_computed_crc;
	o_goodResponse = false;

	// Read the data response register
	getScom(proc_target, p9avslib::OCB_O2SRD[i_avsBusNum][i_o2sBridgeNum], l_data64);

	// Status Return Code and Received CRC
	// out SS len
	l_data_status_code = ~PPC_BITMASK(0, 1) & l_data64;
	l_rsp_rcvd_crc = (~PPC_BITMASK(29, 31) & l_data64) >> 32;
	l_rsp_data = ~PPC_BITMASK(0, 31) & l_data_64;

	// Compute CRC on Response frame
	l_rsp_computed_crc = avsCRCcalc(l_rsp_data);

	if (l_data_status_code == 0                 // no error code
	&& l_rsp_rcvd_crc == l_rsp_computed_crc     // good crc
	&& l_rsp_data != 0                          // valid response
	&& l_rsp_data != 0xFFFFFFFF)
	{
		o_goodResponse = true;
	}
}

static void p9_fbc_eff_config_links_query_link_en(
	chiplet_id_t xbus_target,
	uint8_t& o_link_is_enabled)
{
	fapi2::ATTR_LINK_TRAIN_Type l_link_train;
	FAPI_ATTR_GET(fapi2::ATTR_LINK_TRAIN, xbus_target, l_link_train);
	if (l_link_train == fapi2::ENUM_ATTR_LINK_TRAIN_BOTH)
	{
		o_link_is_enabled = fapi2::ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_TRUE;
	}
	else if (l_link_train == fapi2::ENUM_ATTR_LINK_TRAIN_EVEN_ONLY)
	{
		o_link_is_enabled = fapi2::ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_EVEN_ONLY;
	}
	else if (l_link_train == fapi2::ENUM_ATTR_LINK_TRAIN_ODD_ONLY)
	{
		o_link_is_enabled = fapi2::ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_ODD_ONLY;
	}
	else
	{
		o_link_is_enabled = fapi2::ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_FALSE;
	}
}

template<fapi2::TargetType T>
static void p9_fbc_eff_config_links_query_endp(
	const fapi2::Target<T>& i_loc_target,
	const uint8_t i_loc_fbc_chip_id,
	const uint8_t i_loc_fbc_group_id,
	const p9_fbc_link_ctl_t i_link_ctl_arr[],
	const uint8_t i_link_ctl_arr_size,
	bool i_rem_fbc_id_is_chip,
	uint8_t o_loc_link_en[],
	uint8_t o_rem_link_id[],
	uint8_t o_rem_fbc_id[])
{
	// A/X link ID for local end
	uint8_t l_loc_link_id = 0;
	// remote end target
	fapi2::Target<T> l_rem_target;
	// determine link ID/enable state for local end
	p9_fbc_eff_config_links_map_endp<T>(i_loc_target, i_link_ctl_arr, i_link_ctl_arr_size, l_loc_link_id);
	p9_fbc_eff_config_links_query_link_en<T>(i_loc_target, o_loc_link_en[l_loc_link_id]);
	// local end link target is enabled, query remote end
	if (o_loc_link_en[l_loc_link_id])
	{
		// obtain endpoint target associated with remote end of link
		i_loc_target.getOtherEnd(l_rem_target);
		// endpoint target is configured, qualify local link enable with remote endpoint state
		p9_fbc_eff_config_links_query_link_en<T>(l_rem_target, o_loc_link_en[l_loc_link_id]);
	}
	// link is enabled, gather remaining remote end parameters
	if (o_loc_link_en[l_loc_link_id])
	{
		uint8_t l_rem_fbc_group_id;
		uint8_t l_rem_fbc_chip_id;
		p9_fbc_eff_config_links_map_endp<T>(l_rem_target, i_link_ctl_arr, i_link_ctl_arr_size, o_rem_link_id[l_loc_link_id]);
		// return either chip or group ID of remote chip
		FAPI_ATTR_GET(fapi2::ATTR_PROC_FABRIC_CHIP_ID, l_rem_target, l_rem_fbc_chip_id);
		FAPI_ATTR_GET(fapi2::ATTR_PROC_FABRIC_GROUP_ID, l_rem_target, l_rem_fbc_group_id);
		if (i_rem_fbc_id_is_chip)
		{
			o_rem_fbc_id[l_loc_link_id] = l_rem_fbc_chip_id;
		}
		else
		{
			o_rem_fbc_id[l_loc_link_id] = l_rem_fbc_group_id;
		}
	}
	FAPI_ATTR_SET(
		fapi2::ATTR_PROC_FABRIC_LINK_ACTIVE,
		i_loc_target,
		o_loc_link_en[l_loc_link_id] ? fapi2::ENUM_ATTR_PROC_FABRIC_LINK_ACTIVE_TRUE : fapi2::ENUM_ATTR_PROC_FABRIC_LINK_ACTIVE_FALSE);
}

template<fapi2::TargetType T>
static void p9_fbc_eff_config_links_map_endp(
	const fapi2::Target<T>& i_target,
	const p9_fbc_link_ctl_t i_link_ctl_arr[],
	const uint8_t i_link_ctl_arr_size,
	uint8_t& o_link_id)
{
	uint8_t l_loc_unit_id;
	bool l_found = false;

	FAPI_ATTR_GET(fapi2::ATTR_CHIP_UNIT_POS, i_target, l_loc_unit_id);
	for (uint8_t l_link_id = 0; l_link_id < i_link_ctl_arr_size && !l_found; l_link_id++)
	{
		if(static_cast<fapi2::TargetType>(i_link_ctl_arr[l_link_id].endp_type) == T) && (i_link_ctl_arr[l_link_id].endp_unit_id == l_loc_unit_id)
		{
			o_link_id = l_link_id;
			l_found = true;
		}
	}
}

void avsDriveCommand(
	const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& proc_target,
	const uint8_t  i_avsBusNum,
	const uint8_t  i_o2sBridgeNum,
	const uint32_t i_RailSelect,
	const uint32_t i_CmdType,
	const uint32_t i_CmdGroup,
	enum avsBusOpType i_opType)
{
	fapi2::buffer<uint64_t> l_data64;
	fapi2::buffer<uint32_t> l_data64WithoutCRC;
	putScom(proc_target, p9avslib::OCB_O2SCMD[i_avsBusNum][i_o2sBridgeNum], PPC_BIT(1));
	l_data64 = 0;
	l_data64 |= PPC_BIT(1);
	l_data64.insertFromRight<2, 2>(i_CmdType);
	l_data64.insertFromRight<4, 1>(i_CmdGroup);
	l_data64.insertFromRight<9, 4>(i_RailSelect);
	l_data64.extract(l_data64WithoutCRC, 0, 32);
	l_data64.insertFromRight<29, 3>(avsCRCcalc(l_data64WithoutCRC));
	putScom(proc_target, p9avslib::OCB_O2SWD[i_avsBusNum][i_o2sBridgeNum], l_data64);
	// Wait on o2s_ongoing = 0
	avsPollVoltageTransDone(proc_target, i_avsBusNum, i_o2sBridgeNum);
}

uint32_t avsCRCcalc(const uint32_t i_avs_cmd)
{
	//Polynomial = x^3 + x^1 + x^0 = 1*x^3 + 0*x^2 + 1*x^1 + 1*x^0
	//           = divisor(1011)
	uint32_t o_crc_value = 0;
	uint32_t l_polynomial = 0xB0000000;
	uint32_t l_msb =        0x80000000;

	o_crc_value = i_avs_cmd & AVS_CRC_DATA_MASK;

	while (o_crc_value & AVS_CRC_DATA_MASK)
	{
		if (o_crc_value & l_msb)
		{
			o_crc_value = o_crc_value ^ l_polynomial;
			l_polynomial = l_polynomial >> 1;
		}
		else
		{
			l_polynomial = l_polynomial >> 1;
		}
		l_msb = l_msb >> 1;
	}
	return o_crc_value;
}
