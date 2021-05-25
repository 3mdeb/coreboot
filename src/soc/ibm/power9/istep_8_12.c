#include <cpu/power/istep_8.h>

#define NUM_OP_POINTS      4
#define VPD_PV_POWERSAVE   1
#define VPD_PV_NOMINAL     0
#define VPD_PV_TURBO       2
#define VPD_PV_ULTRA       3
#define VPD_PV_POWERBUS    4


const uint32_t OCB_O2SCTRLF[2][2] =
{
	OCB_O2SCTRLF0A,
	OCB_O2SCTRLF0B,
	OCB_O2SCTRLF1A,
	OCB_O2SCTRLF1B
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

const uint32_t OCB_O2SST[2][2] =
{
	OCB_O2SST0A,
	#define OCB_O2SST0B (0x0006C716)
	OCB_O2SST1A,
	OCB_O2SST1B
};

const uint32_t OCB_O2SCMD[2][2] =
{
	#define OCB_O2SCMD0A (0x0006C707)
	#define OCB_O2SCMD0B (0x0006C717)
	#define OCB_O2SCMD1A (0x0006C727)
	#define OCB_O2SCMD1B (0x0006C737)
};

const uint32_t OCB_O2SWD[2][2] =
{
	OCB_O2SWD0A,
	#define OCB_O2SWD0B (0x0006C718)
	OCB_O2SWD1A,
	OCB_O2SWD1B
};

const uint32_t OCB_O2SRD[2][2] =
{
	OCB_O2SRD0A,
	#define OCB_O2SRD0B (0x0006C719)
	OCB_O2SRD1A,
	OCB_O2SRD1B
};

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
	16666(0), iv_proc_dpll_divider(0), iv_nest_freq_mhz(0),
	iv_wov_underv_enabled(0), iv_wov_overv_enabled(0)
	iv_attrs.attr_dpll_bias = 0;
	iv_attrs.attr_undervolting = 0;
}

void PlatPmPPB::attr_init(void)
{
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
	iv_attrs.attr_pm_safe_frequency_mhz = 4766;
	iv_attrs.attr_pm_safe_voltage_mv = 0;

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
	iv_nest_freq_mhz      = iv_attrs.attr_nest_frequency_mhz;
}

void p9_setup_evid(chiplet_id_t proc_chip_target)
{
	pm_pstate_parameter_block::AttributeList attrs;
	fapi2::voltageBucketData_t l_fstChlt_vpd_data;
	PlatPmPPB l_pmPPB(proc_chip_target);
	l_pmPPB.attr_init();

	iv_raw_vpd_pts = 0;
	iv_biased_vpd_pts = 0;
	iv_poundW_data = 0;
	iv_operating_points = 0;
	iv_attr_mvpd_poundV_raw = 0;
	iv_attr_mvpd_poundV_biased = 0;

	iv_bias[].frequency_hp    = {0};
	iv_bias[].vdd_ext_hp      = {0};
	iv_bias[].vdd_int_hp      = {0};
	iv_bias[].vdn_ext_hp      = {0};
	iv_bias[].vcs_ext_hp      = {0};

	iv_poundV_raw_data = {0};
	l_fstChlt_vpd_data = {0};
	iv_poundV_bucket_id = 0;

	if (CHIPE_EC == 0x20)
	{
		iv_vdm_enabled = false;
		iv_wov_underv_enabled = false;
		iv_wov_overv_enabled = false;
	}
	else
	{
		get_mvpd_poundW();
	}
	iv_wof_enabled = false;

	const uint8_t pv_op_order[NUM_OP_POINTS] = {VPD_PV_POWERSAVE, VPD_PV_NOMINAL, VPD_PV_TURBO, VPD_PV_ULTRA};

	Safe_mode_parameters l_safe_mode_values;
	uint32_t l_safe_mode_op_ps2freq_mhz = 4766;

	l_safe_mode_values.safe_vdm_jump_value = 0;
	// ps2v_mv()
	////
	uint32_t l_vdd = ((int16_t)(((float)0/(float)0) * (1 << 12)) * (-285)) >> 12;
	l_safe_mode_values.safe_mode_mv = ((l_vdd << 1) + 1) >> 1;
	////

	iv_attrs.vdd_voltage_mv = l_safe_mode_values.safe_mode_mv;

	memcpy(l_safe_mode_values, &l_safe_mode_values, sizeof(Safe_mode_parameters));
	iv_attrs.attr_pm_safe_voltage_mv = l_safe_mode_values.safe_mode_mv;

	FAPI_ATTR_SET(fapi2::ATTR_VDD_BOOT_VOLTAGE, iv_procChip, iv_attrs.vdd_voltage_mv);
	FAPI_ATTR_SET(fapi2::ATTR_SAFE_MODE_VOLTAGE_MV, iv_procChip, l_safe_mode_values.safe_mode_mv);
	FAPI_ATTR_SET(fapi2::ATTR_SAFE_MODE_FREQUENCY_MHZ, iv_procChip, 4766);

	memcpy(&attrs ,&iv_attrs, sizeof(iv_attrs));

	if (attrs.vdd_voltage_mv)
	{
		uint8_t     l_goodResponse;
		uint32_t    l_present_voltage_mv;
		uint32_t    l_target_mv;
		int32_t     l_delta_mv;
		avsIdleFrame(proc_chip_target);

		do
		{
			fapi2::buffer<uint64_t> l_data64;
			// read
			avsDriveCommand(proc_chip_target, 3, 0xFFFF);
			getScom(proc_chip_target, OCB_O2SRD0B, l_data64);
			l_present_voltage_mv = (l_data64 & 0x00FFFF0000000000) >> 40;
			l_goodResponse = avsValidateResponse(proc_chip_target, );
			if (!l_goodResponse)
			{
				avsIdleFrame(proc_chip_target);
			}
		}
		while (!l_goodResponse);

		l_delta_mv = l_present_voltage_mv - attrs.vdd_voltage_mv;

		while (l_delta_mv)
		{
			uint32_t l_abs_delta_mv = abs(l_delta_mv);
			if (l_abs_delta_mv > 50)
			{
				if (l_delta_mv > 0)
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
				l_target_mv = attrs.vdd_voltage_mv;
			}
			do
			{
				// write
				avsDriveCommand(proc_chip_target, 0, l_target_mv);
				l_goodResponse = avsValidateResponse(proc_chip_target);
				if (!l_goodResponse)
				{
					avsIdleFrame(proc_chip_target);
				}
			}
			while (!l_goodResponse);
		}
	}
}

uint32_t PlatPmPPB::ps2v_mv(void)
{

	uint32_t region_start, region_end;
	const char* pv_op_str[NUM_OP_POINTS] = PV_OP_ORDER_STR;

	uint32_t l_SlopeValue = (int16_t)(((float)0/(float)0) * (1 << 12));
	uint32_t l_vdd = (l_SlopeValue * (-285)) >> 12;
	return ((l_vdd << 1) + 1) >> 1;
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

static void PlatPmPPB::get_mvpd_poundW(void)
{
	std::vector<fapi2::Target<fapi2::TARGET_TYPE_EQ>> l_eqChiplets;
	l_eqChiplets = iv_procChip.getChildren<fapi2::TARGET_TYPE_EQ>(fapi2::TARGET_STATE_FUNCTIONAL);

	fapi2::vdmData_t l_vdmBuf = 0;
	PoundW_data_per_quad l_poundwPerQuad;
	PoundW_data l_poundw;
	l_poundw = 0;
	l_poundwPerQuad = 0;

	memcpy(&l_poundwPerQuad.resistance_data, &l_poundw.resistance_data, 10);
	l_poundwPerQuad.resistance_data.r_undervolt_allowed = l_poundw.undervolt_tested;
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
	if (CHIP_EC > 0x20)
	{
		for (auto p = 0; p < NUM_OP_POINTS; ++p)
		{
			if (!iv_poundW_data.poundw[p].ivdd_tdp_ac_current_10ma)
			{
				iv_wof_enabled = false;

			}
			if (!iv_poundW_data.poundw[p].ivdd_tdp_dc_current_10ma)
			{
				iv_wof_enabled = false;
			}
		}
	}

	validate_quad_spec_data();

	if((((iv_poundW_data.poundw[].vdm_overvolt_small_thresholds >> 4) & 0x0F) > 0x7
		&& (iv_poundW_data.poundw[p].vdm_overvolt_small_thresholds >> 4) & 0x0F != 0xC)
	|| ((iv_poundW_data.poundw[].vdm_overvolt_small_thresholds) & 0x0F) == 8
	|| ((iv_poundW_data.poundw[].vdm_overvolt_small_thresholds) & 0x0F) == 9
	|| ((iv_poundW_data.poundw[].vdm_overvolt_small_thresholds) & 0x0F) > 0xF
	|| ((iv_poundW_data.poundw[].vdm_large_extreme_thresholds >> 4) & 0x0F) == 8
	|| ((iv_poundW_data.poundw[].vdm_large_extreme_thresholds >> 4) & 0x0F) == 9
	|| ((iv_poundW_data.poundw[].vdm_large_extreme_thresholds >> 4) & 0x0F) > 0xF
	|| ((iv_poundW_data.poundw[].vdm_large_extreme_thresholds) & 0x0F) == 8
	|| ((iv_poundW_data.poundw[].vdm_large_extreme_thresholds) & 0x0F) == 9
	|| ((iv_poundW_data.poundw[].vdm_large_extreme_thresholds) & 0x0F) > 0xF
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

static void avsIdleFrame(chiplet_id_t proc_target)
{
	write_scom_for_chiplet(proc_target, OCB_O2SCMD0B, PPPC_BIT(1));
	write_scom_for_chiplet(proc_target, OCB_O2SWD0B, 0xFFFFFFFFFFFFFFFF);
	uint64_t ocb_o2sst;
	for(uint8_t l_count = 0; l_count < 0x1000; ++l_count)
	{
		ocb_o2sst = read_scom_for_chiplet(proc_target, OCB_O2SST0B);
		if (ocb_o2sst & PPC_BIT(0) == 0)
		{
			return;
		}
	}
}

static uint8_t avsValidateResponse(chiplet_id_t proc_target)
{
	uint64_t l_data64 = read_scom_for_target(proc_target, OCB_O2SRD0B);
	uint32_t l_rsp_data = ~PPC_BITMASK(0, 31) & l_data64;
	o_goodResponse =
		~PPC_BITMASK(0, 1) & l_data64 == 0
		&& (~PPC_BITMASK(29, 31) & l_data64) >> 32 == avsCRCcalc(l_rsp_data)
		&& l_rsp_data != 0
		&& l_rsp_data != 0xFFFFFFFF;
}

void avsDriveCommand(chiplet_id_t proc_target, const uint32_t i_CmdType, /* 0 or 3 */ enum avsBusOpType i_opType)
{
	write_scom_for_chiplet(proc_target, OCB_O2SCMD0B, PPC_BIT(1));
	uint64_t l_data64 = PPC_BIT(1);
	l_data64 |= (i_CmdType & 0x3) << 60;
	l_data64 &= ~(PPC_BIT(4) | PPC_BITMASK(9, 12));
	uint32_t l_data64WithoutCRC = l_data64 & PPC_BITMASK(0, 31);
	l_data64 |= (avsCRCcalc(l_data64WithoutCRC) & 0x7) << 32;
	write_scom_for_chiplet(proc_target, OCB_O2SWD0B, l_data64);
	avsPollVoltageTransDone(proc_target);
}

void avsPollVoltageTransDone(const chiplet_id_t proc_target)
{
	for(uint8_t l_count = 0;
		l_count < 0x1000
		&& read_scom_for_chiplet(proc_target, OCB_O2SST0B) & PPC_BIT(0);
		++l_count);
}

uint32_t avsCRCcalc(const uint32_t i_avs_cmd)
{
	//Polynomial = x^3 + x^1 + x^0 = 1*x^3 + 0*x^2 + 1*x^1 + 1*x^0
	//           = divisor(1011)
	uint32_t o_crc_value = 0;
	uint32_t l_polynomial = 0xB0000000;
	uint32_t l_msb =        0x80000000;

	o_crc_value = i_avs_cmd & 0xFFFFFFF8;

	while(o_crc_value & 0xFFFFFFF8)
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
