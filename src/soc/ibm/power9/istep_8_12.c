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
	iv_procChip (i_target), iv_pstates_enabled(0), iv_resclk_enabled(0),
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
	FAPI_ATTR_GET(fapi2::CHIP_EC_FEATURE_VDM_NOT_SUPPORTED, iv_procChip, iv_attrs.attr_dd_vdm_not_supported); // 1 if EC == 0x20
	FAPI_ATTR_GET(fapi2::CHIP_EC_FEATURE_WOF_NOT_SUPPORTED, iv_procChip, iv_attrs.attr_dd_wof_not_supported); // 1 if EC == 0x20
	FAPI_ATTR_GET(fapi2::FREQ_CORE_FLOOR_MHZ, FAPI_SYSTEM, iv_attrs.attr_freq_core_floor_mhz);
	FAPI_ATTR_GET(fapi2::ATTR_FREQ_PB_MHZ, FAPI_SYSTEM, iv_attrs.attr_nest_frequency_mhz);
	FAPI_ATTR_GET(fapi2::ATTR_FREQ_CORE_CEILING_MHZ, FAPI_SYSTEM, iv_attrs.attr_freq_core_ceiling_mhz);
	FAPI_ATTR_GET(fapi2::ATTR_SAFE_MODE_FREQUENCY_MHZ,iv_procChip, iv_attrs.attr_pm_safe_frequency_mhz);
	FAPI_ATTR_GET(fapi2::ATTR_SAFE_MODE_VOLTAGE_MV, iv_procChip, iv_attrs.attr_pm_safe_voltage_mv);
	FAPI_ATTR_GET(fapi2::ATTR_VCS_BOOT_VOLTAGE, iv_procChip, iv_attrs.vcs_voltage_mv);
	FAPI_ATTR_GET(fapi2::ATTR_VDD_BOOT_VOLTAGE, iv_procChip, iv_attrs.vdd_voltage_mv);
	FAPI_ATTR_GET(fapi2::ATTR_VDN_BOOT_VOLTAGE, iv_procChip, iv_attrs.vdn_voltage_mv);

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

	iv_pstates_enabled = true;
	iv_resclk_enabled  = true;
	iv_vdm_enabled     = true;
	iv_ivrm_enabled    = true;
	iv_wof_enabled     = true;
	iv_wov_underv_enabled = true;
	iv_wov_overv_enabled = true;
	iv_frequency_step_khz = 16666;
	iv_nest_freq_mhz      = iv_attrs.attr_nest_frequency_mhz;
}

void p9_setup_evid(chiplet_id_t proc_chip_target)
{
	pm_pstate_parameter_block::AttributeList attrs;
	// Instantiate PPB object
	PlatPmPPB l_pmPPB(proc_chip_target);
	l_pmPPB.attr_init();
	// Compute the boot/safe values
	l_pmPPB.compute_boot_safe();
	memcpy(&attrs ,&iv_attrs, sizeof(iv_attrs));
	// Set the DPLL frequency values to safe mode values
	p9_setup_dpll_values(proc_chip_target);

	if (attrs.vdd_voltage_mv)
	{
		p9_setup_evid_voltageWrite(
			proc_chip_target,
			0,
			0,
			attrs.vdd_voltage_mv,
			VDD_SETUP); // VDD_SETUP = 6
	}

	if (attrs.vdn_voltage_mv)
	{
		p9_setup_evid_voltageWrite(
			proc_chip_target,
			1,
			0,
			attrs.vdn_voltage_mv,
			VDN_SETUP); // VDN_SETUP = 7
	}

	if (attrs.vcs_voltage_mv)
	{
		p9_setup_evid_voltageWrite(
			proc_chip_target,
			0,
			1,
			attrs.vcs_voltage_mv,
			VCS_SETUP); // VCS_SETUP = 8
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
		// Initialize the buses
		avsInitExtVoltageControl(proc_chip_target, i_bus_num, BRIDGE_NUMBER);
	}

	// Drive AVS Bus with a frame value 0xFFFFFFFF (idle frame) to
	// initialize the AVS slave
	avsIdleFrame(proc_chip_target, i_bus_num, BRIDGE_NUMBER);

	// Read the present voltage

	// This loop is to ensrue AVSBus Master and Slave are in sync
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
		// Convert frequency value to a format that needs to be written to the
		// register
		uint32_t l_safe_mode_dpll_value = l_attr_safe_mode_freq * 8000 / 133333;
		l_data64.insertFromRight<1, 11>(l_safe_mode_dpll_value);
		l_data64.insertFromRight<17, 11>(l_safe_mode_dpll_value);
		l_data64.insertFromRight<33, 11>(l_safe_mode_dpll_value);
		//
		fapi2::putScom(*l_itr, EQ_QPPM_DPLL_FREQ, l_data64);
		//Update VDM VID compare to safe mode value
		//Convert same mode value to a format that needs to be written to
		//the register
		uint32_t l_vdm_vid_value = (l_attr_safe_mode_mv - 512) / 4;
		// EQ_QPPM_VDMCFGR = 0x100F01B6
		fapi2::getScom(*l_itr, EQ_QPPM_VDMCFGR, l_data64);
		l_data64.insertFromRight<0, 8>(l_vdm_vid_value);
		fapi2::putScom(*l_itr, EQ_QPPM_VDMCFGR, l_data64);
	}
}

static void p9_pm_get_poundv_bucket(
    const fapi2::Target<fapi2::TARGET_TYPE_EQ>& i_target,
    fapi2::voltageBucketData_t& o_data)
{
    fapi2::ATTR_POUNDV_BUCKET_DATA_Type l_bucketAttr;
    FAPI_ATTR_GET(fapi2::ATTR_POUNDV_BUCKET_DATA, i_target, l_bucketAttr);
    memcpy(&o_data, l_bucketAttr, sizeof(o_data));
}

void PlatPmPPB::chk_valid_poundv(const bool i_biased_state)
{
	const uint8_t   pv_op_order[4] = {VPD_PV_POWERSAVE, VPD_PV_NOMINAL, VPD_PV_TURBO, VPD_PV_ULTRA};
	uint8_t         i = 0;
	bool            suspend_ut_check = false;
	uint8_t         l_chiplet_num = iv_procChip.getChipletNumber();

	VpdPoint        l_attr_mvpd_data[5];
	memcpy(l_attr_mvpd_data,iv_attr_mvpd_poundV_raw,sizeof(l_attr_mvpd_data));

	if (i_biased_state)
	{
		memcpy(l_attr_mvpd_data,iv_attr_mvpd_poundV_biased,sizeof(l_attr_mvpd_data));
	}

	// check valid operating points' values have this relationship (power save <= nominal <= turbo <= ultraturbo)
	for (i = 1; i < 4; i++)
	{
		// does not execute for i = 4 meaning UltraTurbo
		if (l_attr_mvpd_data[pv_op_order[i-1]].frequency_mhz > l_attr_mvpd_data[pv_op_order[i]].frequency_mhz
		|| l_attr_mvpd_data[pv_op_order[i-1]].vdd_mv        > l_attr_mvpd_data[pv_op_order[i]].vdd_mv
		|| l_attr_mvpd_data[pv_op_order[i-1]].idd_100ma     > l_attr_mvpd_data[pv_op_order[i]].idd_100ma
		|| l_attr_mvpd_data[pv_op_order[i-1]].vcs_mv        > l_attr_mvpd_data[pv_op_order[i]].vcs_mv
		|| l_attr_mvpd_data[pv_op_order[i-1]].ics_100ma     > l_attr_mvpd_data[pv_op_order[i]].ics_100ma)
		{
			iv_pstates_enabled = false;
		}
	}
}

void PlatPmPPB::get_mvpd_poundV()
{
	std::vector<fapi2::Target<fapi2::TARGET_TYPE_EQ>> l_eqChiplets;
	fapi2::voltageBucketData_t l_fstChlt_vpd_data;
	fapi2::Target<fapi2::TARGET_TYPE_EQ> l_firstEqChiplet;
	uint8_t    j                = 0;
	uint8_t l_buffer[61]         = {0};
	uint8_t*   l_buffer_inc     = NULL;

	memset(&l_fstChlt_vpd_data, 0, sizeof(l_fstChlt_vpd_data));
	iv_eq_chiplet_state = 0;

	l_eqChiplets = iv_procChip.getChildren<fapi2::TARGET_TYPE_EQ>(fapi2::TARGET_STATE_FUNCTIONAL);
	iv_eq_chiplet_state = l_eqChiplets.size();

	for (j = 0; j < l_eqChiplets.size(); j++)
	{
		l_buffer = {0};

		l_buffer_inc = l_buffer;
		l_buffer_inc++;
		for (int i = 0; i <= 4; i++)
		{
			iv_attr_mvpd_poundV_raw[i].frequency_mhz  = 0
			l_buffer_inc += 2;
			iv_attr_mvpd_poundV_raw[i].vdd_mv = 0
			l_buffer_inc += 2;
			iv_attr_mvpd_poundV_raw[i].idd_100ma = 0
			l_buffer_inc += 2;
			iv_attr_mvpd_poundV_raw[i].vcs_mv = 0
			l_buffer_inc += 2;
			iv_attr_mvpd_poundV_raw[i].ics_100ma = 0
			l_buffer_inc += 2;
		}

		iv_pstates_enabled = false;

		l_firstEqChiplet = l_eqChiplets[j];
		iv_poundV_bucket_id = iv_poundV_raw_data.bucketId;;
		memcpy(&l_fstChlt_vpd_data,&iv_poundV_raw_data,sizeof(iv_poundV_raw_data));

		if ((iv_poundV_raw_data.VddNomVltg    > l_fstChlt_vpd_data.VddNomVltg)
		||	(iv_poundV_raw_data.VddPSVltg     > l_fstChlt_vpd_data.VddPSVltg)
		||	(iv_poundV_raw_data.VddTurboVltg  > l_fstChlt_vpd_data.VddTurboVltg)
		||	(iv_poundV_raw_data.VddUTurboVltg > l_fstChlt_vpd_data.VddUTurboVltg)
		||	(iv_poundV_raw_data.VdnPbVltg     > l_fstChlt_vpd_data.VdnPbVltg) )
		{
			memcpy(&l_fstChlt_vpd_data,&iv_poundV_raw_data,sizeof(iv_poundV_raw_data));
			iv_poundV_bucket_id = iv_poundV_raw_data.bucketId;;
		}
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
    double freq_bias[NUM_OP_POINTS];
    double voltage_ext_vdd_bias[NUM_OP_POINTS];
    double voltage_ext_vcs_bias;
    double voltage_ext_vdn_bias;

	iv_bias[POWERSAVE].frequency_hp    = 0;
	iv_bias[POWERSAVE].vdd_ext_hp      = 0;
	iv_bias[POWERSAVE].vdd_int_hp      = 0;

	iv_bias[NOMINAL].frequency_hp    = 0;
	iv_bias[NOMINAL].vdd_ext_hp      = 0;
	iv_bias[NOMINAL].vdd_int_hp      = 0;

	iv_bias[TURBO].frequency_hp    = 0;
	iv_bias[TURBO].vdd_ext_hp      = 0;
	iv_bias[TURBO].vdd_int_hp      = 0;

	iv_bias[ULTRA].frequency_hp    = 0;
	iv_bias[ULTRA].vdd_ext_hp      = 0;
	iv_bias[ULTRA].vdd_int_hp      = 0;

    for (auto point = 0; point < NUM_OP_POINTS; p++)
    {
        iv_bias[point].vdn_ext_hp      = 0;
        iv_bias[point].vcs_ext_hp      = 0;

        freq_bias[point]                 = 1.0;
        voltage_ext_vdd_bias[point]      = 1.0;
    }

    // VCS bias applied to all operating points
    voltage_ext_vcs_bias = 1.0;

    // VDN bias applied to all operating points
    voltage_ext_vdn_bias = 1.0;

    // Change the VPD frequency, VDD and VCS values with the bias multiplers
    for (auto p = 0; p < NUM_OP_POINTS; p++)
    {
        double freq_mhz = ((double)iv_attr_mvpd_poundV_biased[p].frequency_mhz * freq_bias[p]);
        double vdd_mv = ((double)iv_attr_mvpd_poundV_biased[p].vdd_mv * voltage_ext_vdd_bias[p]);
        double vcs_mv = ((double)iv_attr_mvpd_poundV_biased[p].vcs_mv * voltage_ext_vcs_bias);

        iv_attr_mvpd_poundV_biased[p].frequency_mhz = (uint16_t)internal_floor(freq_mhz);
        iv_attr_mvpd_poundV_biased[p].vdd_mv        = (uint16_t)internal_ceil(vdd_mv);
        iv_attr_mvpd_poundV_biased[p].vcs_mv        = (uint16_t)(vcs_mv);
    }
    double vdn_mv =
           (( (double)iv_attr_mvpd_poundV_biased[VPD_PV_POWERBUS].vdd_mv) * voltage_ext_vdn_bias);

    iv_attr_mvpd_poundV_biased[VPD_PV_POWERBUS].vdd_mv = (uint32_t)internal_ceil(vdn_mv);

    return fapi2::FAPI2_RC_SUCCESS;
}

void PlatPmPPB::apply_biased_values()
{
	memcpy(iv_attr_mvpd_poundV_biased,iv_attr_mvpd_poundV_raw,sizeof(iv_attr_mvpd_poundV_raw));
	get_extint_bias();
	chk_valid_poundv(BIASED);
}

#define VALIDATE_THRESHOLD_VALUES(w, x, y, z, state) \
    if ((w > 0x7 && w != 0xC) || /* overvolt */   \
        (x == 8) ||  (x == 9) || (x > 0xF) ||     \
        (y == 8) ||  (y == 9) || (y > 0xF) ||     \
        (z == 8) ||  (z == 9) || (z > 0xF)   )    \
    { state = 0; }

void PlatPmPPB::get_mvpd_poundW (void)
{
    std::vector<fapi2::Target<fapi2::TARGET_TYPE_EQ>> l_eqChiplets;
    fapi2::vdmData_t l_vdmBuf;
    uint8_t    selected_eq      = 0;
    uint8_t    bucket_id        = 0;
    uint8_t    version_id       = 0;

    const char*     pv_op_str[NUM_OP_POINTS] = PV_OP_ORDER_STR;

	// Exit if both VDM and WOF is disabled
	if ((!is_vdm_enabled() && !is_wof_enabled()))
	{
		iv_vdm_enabled = false;
		iv_wof_enabled = false;
		iv_wov_underv_enabled = false;
		iv_wov_overv_enabled = false;
		break;
	}

	l_eqChiplets = iv_procChip.getChildren<fapi2::TARGET_TYPE_EQ>(fapi2::TARGET_STATE_FUNCTIONAL);

	for (selected_eq = 0; selected_eq < l_eqChiplets.size(); selected_eq++)
	{
		uint8_t l_chipletNum = 0xFF;

		FAPI_ATTR_GET(fapi2::ATTR_CHIP_UNIT_POS, l_eqChiplets[selected_eq], l_chipletNum);
		// clear out buffer to known value before calling fapiGetMvpdField
		memset(&l_vdmBuf, 0, sizeof(l_vdmBuf));
		FAPI_TRY(p9_pm_get_poundw_bucket(l_eqChiplets[selected_eq], l_vdmBuf));

		bucket_id   =   l_vdmBuf.bucketId;
		version_id  =   l_vdmBuf.version;


		if( version_id < POUNDW_VERSION_30 )
		{
			PoundW_data_per_quad  l_poundwPerQuad;
			PoundW_data           l_poundw;
			memcpy( &l_poundw, l_vdmBuf.vdmData, sizeof( PoundW_data ) );
			memset( &l_poundwPerQuad, 0, sizeof(PoundW_data_per_quad) );

			for( size_t op = 0; op <= ULTRA; op++ )
			{
				l_poundwPerQuad.poundw[op].ivdd_tdp_ac_current_10ma      =   l_poundw.poundw[op].ivdd_tdp_ac_current_10ma;

				l_poundwPerQuad.poundw[op].ivdd_tdp_dc_current_10ma      =   l_poundw.poundw[op].ivdd_tdp_dc_current_10ma;
				l_poundwPerQuad.poundw[op].vdm_overvolt_small_thresholds =   l_poundw.poundw[op].vdm_overvolt_small_thresholds;
				l_poundwPerQuad.poundw[op].vdm_large_extreme_thresholds  =   l_poundw.poundw[op].vdm_large_extreme_thresholds;
				l_poundwPerQuad.poundw[op].vdm_normal_freq_drop          =   l_poundw.poundw[op].vdm_normal_freq_drop;
				l_poundwPerQuad.poundw[op].vdm_normal_freq_return        =   l_poundw.poundw[op].vdm_normal_freq_return;
				l_poundwPerQuad.poundw[op].vdm_vid_compare_per_quad[0]   =   l_poundw.poundw[op].vdm_vid_compare_ivid;
				l_poundwPerQuad.poundw[op].vdm_vid_compare_per_quad[1]   =   l_poundw.poundw[op].vdm_vid_compare_ivid;
				l_poundwPerQuad.poundw[op].vdm_vid_compare_per_quad[2]   =   l_poundw.poundw[op].vdm_vid_compare_ivid;
				l_poundwPerQuad.poundw[op].vdm_vid_compare_per_quad[3]   =   l_poundw.poundw[op].vdm_vid_compare_ivid;
				l_poundwPerQuad.poundw[op].vdm_vid_compare_per_quad[4]   =   l_poundw.poundw[op].vdm_vid_compare_ivid;
				l_poundwPerQuad.poundw[op].vdm_vid_compare_per_quad[5]   =   l_poundw.poundw[op].vdm_vid_compare_ivid;
			}

			memcpy( &l_poundwPerQuad.resistance_data, &l_poundw.resistance_data , LEGACY_RESISTANCE_ENTRY_SIZE );
			l_poundwPerQuad.resistance_data.r_undervolt_allowed =   l_poundw.undervolt_tested;
			memset(&l_vdmBuf.vdmData, 0, sizeof(l_vdmBuf.vdmData));
			memcpy(&l_vdmBuf.vdmData, &l_poundwPerQuad, sizeof( PoundW_data_per_quad ) );
		}

		//if we match with the bucket id, then we don't need to continue
		if (iv_poundV_bucket_id == bucket_id)
		{
			break;
		}
	}

	uint8_t l_poundw_static_data = 0;
	const fapi2::Target<fapi2::TARGET_TYPE_SYSTEM> FAPI_SYSTEM;
	FAPI_ATTR_GET(fapi2::ATTR_POUND_W_STATIC_DATA_ENABLE, FAPI_SYSTEM, l_poundw_static_data);

	if (l_poundw_static_data)
	{
		memcpy (&iv_poundW_data, &g_vpdData, sizeof (g_vpdData));
	}
	else
	{
		memcpy (&iv_poundW_data, l_vdmBuf.vdmData, sizeof (l_vdmBuf.vdmData));
	}

	//Re-ordering to Natural order
	// When we read the data from VPD image the order will be N,PS,T,UT.
	// But we need the order PS,N,T,UT.. hence we are swapping the data
	// between PS and Nominal.
	poundw_entry_per_quad_t l_tmp_data;
	memset( &l_tmp_data ,0, sizeof(poundw_entry_per_quad_t));
	memcpy (&l_tmp_data,
			&(iv_poundW_data.poundw[VPD_PV_NOMINAL]),
			sizeof (poundw_entry_per_quad_t));

	memcpy (&(iv_poundW_data.poundw[VPD_PV_NOMINAL]),
			&(iv_poundW_data.poundw[VPD_PV_POWERSAVE]),
			sizeof(poundw_entry_per_quad_t));

	memcpy (&(iv_poundW_data.poundw[VPD_PV_POWERSAVE]),
			&l_tmp_data,
			sizeof(poundw_entry_per_quad_t));

	// If the #W version is less than 3, validate Turbo VDM large threshold
	// not larger than -32mV. This filters out parts that have bad VPD.  If
	// this check fails, log a recovered error, mark the VDMs disabled and
	// break out of the reset of the checks.
	uint32_t turbo_vdm_large_threshhold =
		(iv_poundW_data.poundw[TURBO].vdm_large_extreme_thresholds >> 4) & 0x0F;

	if (version_id < FULLY_VALID_POUNDW_VERSION &&
		g_GreyCodeIndexMapping[turbo_vdm_large_threshhold] > GREYCODE_INDEX_M32MV)
	{
		iv_vdm_enabled = false;
		break;

	}

	// Validate the WOF content is non-zero if WOF is enabled
	if (is_wof_enabled())
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
	if (!is_vdm_enabled())
	{
		iv_vdm_enabled = false;
		break;
	}

	// validate threshold values
	bool l_threshold_value_state = 1;

	validate_quad_spec_data( );

	for (uint8_t p = 0; p < NUM_OP_POINTS; ++p)
	{
		VALIDATE_THRESHOLD_VALUES(((iv_poundW_data.poundw[p].vdm_overvolt_small_thresholds >> 4) & 0x0F), // overvolt
									((iv_poundW_data.poundw[p].vdm_overvolt_small_thresholds) & 0x0F),      // small
									((iv_poundW_data.poundw[p].vdm_large_extreme_thresholds >> 4) & 0x0F),  // large
									((iv_poundW_data.poundw[p].vdm_large_extreme_thresholds) & 0x0F),       // extreme
									l_threshold_value_state);

		if (!l_threshold_value_state)
		{
			iv_vdm_enabled = false;
		}
	}

	bool l_frequency_value_state = 1;

	for (uint8_t p = 0; p < NUM_OP_POINTS; ++p)
	{
		VALIDATE_FREQUENCY_DROP_VALUES(((iv_poundW_data.poundw[p].vdm_normal_freq_drop) & 0x0F),        // N_L
										((iv_poundW_data.poundw[p].vdm_normal_freq_drop >> 4) & 0x0F),   // N_S
										((iv_poundW_data.poundw[p].vdm_normal_freq_return >> 4) & 0x0F), // L_S
										((iv_poundW_data.poundw[p].vdm_normal_freq_return) & 0x0F),      // S_N
										l_frequency_value_state);

		if (!l_frequency_value_state)
		{
			iv_vdm_enabled = false;
		}
	}

	//If we have reached this point, that means VDM is ok to be enabled. Only then we try to
	//enable wov undervolting
	if (((iv_poundW_data.resistance_data.r_undervolt_allowed) || (iv_attrs.attr_wov_underv_force)) && is_wov_underv_enabled())
	{
		iv_wov_underv_enabled = true;
	} else {
		iv_wov_underv_enabled = false;
	}


fapi_try_exit:

    // Given #W has both VDM and WOF content, a failure needs to disable both
    if (fapi2::current_err != fapi2::FAPI2_RC_SUCCESS)
    {
        iv_vdm_enabled = false;
        iv_wof_enabled = false;
        iv_wov_underv_enabled = false;
        iv_wov_overv_enabled = false;
    }

    if (!(iv_vdm_enabled))
    {
        large_jump_defaults();
    }
}

bool PlatPmPPB::is_vdm_enabled()
{
	const fapi2::Target<fapi2::TARGET_TYPE_SYSTEM> FAPI_SYSTEM;
	uint8_t attr_dd_vdm_not_supported;
	uint8_t attr_system_vdm_disable;

	FAPI_ATTR_GET(fapi2::ATTR_SYSTEM_VDM_DISABLE, FAPI_SYSTEM, attr_system_vdm_disable);
	FAPI_ATTR_GET(fapi2::ATTR_CHIP_EC_FEATURE_VDM_NOT_SUPPORTED, iv_procChip, attr_dd_vdm_not_supported);

	return (!(attr_system_vdm_disable)
	&& !(attr_dd_vdm_not_supported)
	&& iv_vdm_enabled);
}

bool PlatPmPPB::is_wov_underv_enabled()
{
    return(!(iv_attrs.attr_wov_underv_disable) &&
         iv_wov_underv_enabled)
         ? true : false;
}

void PlatPmPPB::vpd_init(void)
{
	iv_raw_vpd_pts = 0;
	iv_biased_vpd_pts = 0;
	iv_poundW_data = 0;
	iv_iddqt = 0;
	iv_operating_points = 0;
	iv_attr_mvpd_poundV_raw = 0;
	iv_attr_mvpd_poundV_biased = 0;

	get_mvpd_poundV();

	if(!iv_eq_chiplet_state)
	{
		break;
	}
	apply_biased_values();
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

void PlatPmPPB::load_mvpd_operating_point ()
{
	const uint8_t pv_op_order[NUM_OP_POINTS] = {VPD_PV_POWERSAVE, VPD_PV_NOMINAL, VPD_PV_TURBO, VPD_PV_ULTRA};
	for (uint32_t i = 0; i < NUM_OP_POINTS; i++)
	{
		iv_raw_vpd_pts[i].frequency_mhz  = iv_attr_mvpd_poundV_raw[pv_op_order[i]].frequency_mhz;
		iv_raw_vpd_pts[i].vdd_mv         = iv_attr_mvpd_poundV_raw[pv_op_order[i]].vdd_mv;
		iv_raw_vpd_pts[i].idd_100ma      = iv_attr_mvpd_poundV_raw[pv_op_order[i]].idd_100ma;
		iv_raw_vpd_pts[i].vcs_mv         = iv_attr_mvpd_poundV_raw[pv_op_order[i]].vcs_mv;
		iv_raw_vpd_pts[i].ics_100ma      = iv_attr_mvpd_poundV_raw[pv_op_order[i]].ics_100ma;
		iv_raw_vpd_pts[i].pstate = iv_attr_mvpd_poundV_raw[ULTRA].frequency_mhz
			- iv_attr_mvpd_poundV_raw[pv_op_order[i]].frequency_mhz * 1000 / 16666;

		iv_biased_vpd_pts[i].frequency_mhz  = iv_attr_mvpd_poundV_biased[pv_op_order[i]].frequency_mhz;
		iv_biased_vpd_pts[i].vdd_mv         = iv_attr_mvpd_poundV_biased[pv_op_order[i]].vdd_mv;
		iv_biased_vpd_pts[i].idd_100ma      = iv_attr_mvpd_poundV_biased[pv_op_order[i]].idd_100ma;
		iv_biased_vpd_pts[i].vcs_mv         = iv_attr_mvpd_poundV_biased[pv_op_order[i]].vcs_mv;
		iv_biased_vpd_pts[i].ics_100ma      = iv_attr_mvpd_poundV_biased[pv_op_order[i]].ics_100ma;
		iv_biased_vpd_pts[i].pstate = iv_attr_mvpd_poundV_biased[ULTRA].frequency_mhz
			- iv_attr_mvpd_poundV_biased[pv_op_order[i]].frequency_mhz * 1000 / 16666;
	}
}

void PlatPmPPB::compute_boot_safe()
{
	// query VPD if any of the voltage attributes are zero
	if(!iv_attrs.vdd_voltage_mv
	|| !iv_attrs.vcs_voltage_mv
	|| !iv_attrs.vdn_voltage_mv)
	{
		// ----------------
		// get VPD data (#V,#W)
		// ----------------
		vpd_init();
		// Compute the VPD operating points
		compute_vpd_pts();
		safe_mode_init();

		// set VCS voltage to UltraTurbo Voltage from MVPD data (if no override)
		if (!iv_attrs.vcs_voltage_mv)
		{
			iv_attrs.vcs_voltage_mv = (uint32_t)((double)iv_attr_mvpd_poundV_biased[POWERSAVE].vcs_mv + (double)(iv_attr_mvpd_poundV_biased[POWERSAVE].ics_100ma * 100 * revle32(64)) / 1000000);
		}

		// set VDN voltage to PowerSave Voltage from MVPD data (if no override)
		if(!iv_attrs.vdn_voltage_mv)
		{
			iv_attrs.vdn_voltage_mv = (uint32_t)((double)iv_attr_mvpd_poundV_biased[POWERBUS].vdd_mv + (double)(iv_attr_mvpd_poundV_biased[POWERBUS].idd_100ma * 100 * revle32(50)) / 1000000)
		}
	}
	else
	{
		// Set safe frequency to the default BOOT_FREQ_MULT
		fapi2::ATTR_BOOT_FREQ_MULT_Type l_boot_freq_mult;
		FAPI_ATTR_GET(fapi2::ATTR_BOOT_FREQ_MULT, iv_procChip, l_boot_freq_mult);
		uint32_t l_boot_freq_mhz = ((l_boot_freq_mult * 133333) / iv_attrs.proc_dpll_divider) / 1000;
		FAPI_ATTR_SET(fapi2::ATTR_SAFE_MODE_FREQUENCY_MHZ, iv_procChip, l_boot_freq_mhz);
		FAPI_ATTR_SET(fapi2::ATTR_SAFE_MODE_VOLTAGE_MV, iv_procChip, iv_attrs.vdd_voltage_mv);
	}
	FAPI_ATTR_SET(fapi2::ATTR_VDD_BOOT_VOLTAGE, iv_procChip, iv_attrs.vdd_voltage_mv);
	FAPI_ATTR_SET(fapi2::ATTR_VCS_BOOT_VOLTAGE, iv_procChip, iv_attrs.vcs_voltage_mv);
	FAPI_ATTR_SET(fapi2::ATTR_VDN_BOOT_VOLTAGE, iv_procChip, iv_attrs.vdn_voltage_mv);
}

void avsInitExtVoltageControl(
	const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target,
	const uint8_t i_avsBusNum,
	const uint8_t i_o2sBridgeNum)
{

	fapi2::buffer<uint64_t> l_data64;
	uint32_t l_avsbus_frequency, l_value, l_nest_frequency;
	uint16_t l_divider;

	// O2SCTRLF
	// [ 0: 5] o2s_frame_size = 32; -> 0x20
	// [ 6:11] o2s_out_count1 = 32; -> 0x20
	// [12:17] o2s_in_delay1  =  0;     No concurrent input
	// [18:23] o2s_in_l_count1  =  X;     No input on first frame

	//uint32_t O2SCTRLF_value = 0b10000010000011111100000000000000;
	ocb_o2sctrlf0a_t O2SCTRLF_value;
	O2SCTRLF_value.fields.o2s_frame_size_an = p9avslib::O2S_FRAME_SIZE;
	O2SCTRLF_value.fields.o2s_out_count1_an = p9avslib::O2S_FRAME_SIZE;
	O2SCTRLF_value.fields.o2s_in_delay1_an = p9avslib::O2S_IN_DELAY1;

	l_data64.insertFromRight<0, 6>(O2SCTRLF_value.fields.o2s_frame_size_an);
	l_data64.insertFromRight<6, 6>(O2SCTRLF_value.fields.o2s_out_count1_an);
	l_data64.insertFromRight<12, 6>(O2SCTRLF_value.fields.o2s_in_delay1_an);
	putScom(i_target, p9avslib::OCB_O2SCTRLF[i_avsBusNum][i_o2sBridgeNum], l_data64);
	// Note:  the buffer is a 32bit buffer.  make sure it is left
	// aligned for the SCOM

	// O2SCTRLS
	// [ 0: 5] o2s_out_count2  = 0;
	// [ 6:11] o2s_in_delay2   = 0;
	// [12:17] o2s_in_l_count2   = 32; -> 0x20;

	// uint32_t O2SCTRLS_value = 0b00000000000010000000000000000000;
	ocb_o2sctrls0a_t O2SCTRLS_value;
	O2SCTRLS_value.fields.o2s_in_count2_an = p9avslib::O2S_FRAME_SIZE;

	l_data64.flush<0>();
	l_data64.insertFromRight<12, 6>(O2SCTRLS_value.fields.o2s_in_count2_an);
	putScom(i_target, p9avslib::OCB_O2SCTRLS[i_avsBusNum][i_o2sBridgeNum], l_data64);
	// O2SCTRL2
	// [    0] o2s_bridge_enable
	// [    1] pmcocr1_reserved_1
	// [    2] o2s_cpol = 0;            Low idle clock
	// [    3] o2s_cpha = 0;            First edge
	// [ 4:13] o2s_clock_divider = 0xFA    Yield 1MHz with 2GHz nest
	// [14:16] pmcocr1_reserved_2
	// [   17] o2s_nr_of_frames = 1; Two frames
	// [18:20] o2s_port_enable (only port 0 (18) by default

	// Divider calculation (which can be overwritten)
	//  Nest Frequency:  2000MHz (0x7D0)
	//  AVSBus Frequency:    1MHz (0x1) (eg  1us per bit)
	//
	// Divider = Nest Frequency / (AVSBus Frequency * 8) - 1
	//
	// @note:  PPE can multiply by a recipricol.  A precomputed
	// 1 / (AVSBus frequency *8) held in an attribute allows a
	// fully l_data64 driven computation without a divide operation.

	ocb_o2sctrl10a_t O2SCTRL1_value;
	O2SCTRL1_value.fields.o2s_bridge_enable_an = 1;

	//Nest frequency attribute in MHz
	FAPI_ATTR_GET(fapi2::ATTR_FREQ_PB_MHZ, fapi2::Target<fapi2::TARGET_TYPE_SYSTEM>(), l_nest_frequency);
	// AVSBus frequency attribute in KHz
	FAPI_ATTR_GET(fapi2::ATTR_AVSBUS_FREQUENCY, fapi2::Target<fapi2::TARGET_TYPE_SYSTEM>(), l_value);
	if (l_value == 0)
	{
		l_avsbus_frequency = p9avslib::AVSBUS_FREQUENCY / 1000;
	}
	else
	{
		l_avsbus_frequency = l_value / 1000;
	}

	// Divider = Nest Frequency / (AVSBus Frequency * 8) - 1
	l_divider = (l_nest_frequency / (l_avsbus_frequency * 8)) - 1;

	O2SCTRL1_value.fields.o2s_clock_divider_an = l_divider;
	O2SCTRL1_value.fields.o2s_nr_of_frames_an = 1;
	O2SCTRL1_value.fields.o2s_cpol_an = 0;
	O2SCTRL1_value.fields.o2s_cpha_an = 1;

	l_data64.flush<0>();
	l_data64.insertFromRight<0, 1>(O2SCTRL1_value.fields.o2s_bridge_enable_an);
	l_data64.insertFromRight<2, 1>(O2SCTRL1_value.fields.o2s_cpol_an);
	l_data64.insertFromRight<3, 1>(O2SCTRL1_value.fields.o2s_cpha_an);
	l_data64.insertFromRight<4, 10>(O2SCTRL1_value.fields.o2s_clock_divider_an);
	l_data64.insertFromRight<17, 1>(O2SCTRL1_value.fields.o2s_nr_of_frames_an);
	putScom(i_target, p9avslib::OCB_O2SCTRL1[i_avsBusNum][i_o2sBridgeNum], l_data64);

	// O2SCTRL1
	// OCC O2S Control2
	// [ 0:16] o2s_inter_frame_delay = filled in with ATTR l_data64
	// Needs to be 10us or greater for SPIVID part operation
	// Set to ~16us for conservatism using a 100ns hang pulse
	// 16us = 16000ns -> 16000/100 = 160 = 0xA0;  aligned to 0:16 -> 0x005
	ocb_o2sctrl20a_t O2SCTRL2_value;
	O2SCTRL2_value.fields.o2s_inter_frame_delay_an = 0x0;

	l_data64.flush<0>();
	l_data64.insertFromRight<0, 17>
	(O2SCTRL2_value.fields.o2s_inter_frame_delay_an);
	putScom(i_target, p9avslib::OCB_O2SCTRL2[i_avsBusNum][i_o2sBridgeNum], l_data64);

fapi_try_exit:
	return fapi2::current_err;
}

fapi2::ReturnCode
avsIdleFrame(const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target,
			 const uint8_t i_avsBusNum,
			 const uint8_t i_o2sBridgeNum)
{
	fapi2::buffer<uint64_t> l_idleframe = 0xFFFFFFFFFFFFFFFF;
	fapi2::buffer<uint64_t> l_scomdata;
	// clear sticky bits in o2s_status_reg
	l_scomdata.setBit<1, 1>();
	putScom(i_target, p9avslib::OCB_O2SCMD[i_avsBusNum][i_o2sBridgeNum], l_scomdata);
	// Send the idle frame
	l_scomdata = l_idleframe;
	putScom(i_target, p9avslib::OCB_O2SWD[i_avsBusNum][i_o2sBridgeNum], l_scomdata));
	// Wait on o2s_ongoing = 0
	avsPollVoltageTransDone(i_target, i_avsBusNum, i_o2sBridgeNum);
}

fapi2::ReturnCode
avsVoltageRead(const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target,
			   const uint8_t i_bus_num,
			   const uint8_t BRIDGE_NUMBER,
			   const uint32_t i_rail_select,
			   uint32_t& l_present_voltage_mv)
{
	fapi2::buffer<uint64_t> l_data64;
	// Drive a Read Command
	avsDriveCommand(
		i_target,
		i_bus_num,
		BRIDGE_NUMBER,
		i_rail_select,
		3,
		0xFFFF);

	// Read returned voltage value from Read frame
	getScom(i_target, p9avslib::OCB_O2SRD[i_avsBusNum][i_o2sBridgeNum], l_data64);
	// Extracting bits 8:23 , which contains voltage read data
	l_present_voltage_mv = (l_data64 & 0x00FFFF0000000000) >> 40;
}

void avsValidateResponse(
	const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target,
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
	getScom(i_target, p9avslib::OCB_O2SRD[i_avsBusNum][i_o2sBridgeNum], l_data64);

	// Status Return Code and Received CRC
	// out SS len
	l_data_status_code = ~PPC_BITMASK(0, 1) & l_data64;
	l_rsp_rcvd_crc = (~PPC_BITMASK(29, 31) & l_data64) >> 32;
	l_rsp_data = ~PPC_BITMASK(0, 31) & l_data_64;

	// Compute CRC on Response frame
	l_rsp_computed_crc = avsCRCcalc(l_rsp_data);

	if (l_data_status_code == 0                 // no error code
	 && l_rsp_rcvd_crc == l_rsp_computed_crc    // good crc
	 && l_rsp_data != 0                         // valid response
	 && l_rsp_data != 0xFFFFFFFF)
	{
		o_goodResponse = true;
	}
}

fapi2::ReturnCode p9_fbc_eff_config_links_query_link_en(
	const fapi2::Target<fapi2::TARGET_TYPE_XBUS>& i_target,
	uint8_t& o_link_is_enabled)
{
	fapi2::ATTR_LINK_TRAIN_Type l_link_train;
	FAPI_ATTR_GET(fapi2::ATTR_LINK_TRAIN, i_target, l_link_train);

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
void p9_fbc_eff_config_links_query_endp(
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
fapi2::ReturnCode p9_fbc_eff_config_links_map_endp(
	const fapi2::Target<T>& i_target,
	const p9_fbc_link_ctl_t i_link_ctl_arr[],
	const uint8_t i_link_ctl_arr_size,
	uint8_t& o_link_id)
{
	uint8_t l_loc_unit_id;
	bool l_found = false;

	FAPI_ATTR_GET(fapi2::ATTR_CHIP_UNIT_POS, i_target, l_loc_unit_id);
	for (uint8_t l_link_id = 0; (l_link_id < i_link_ctl_arr_size) && !l_found; l_link_id++)
	{
		if ((static_cast<fapi2::TargetType>(i_link_ctl_arr[l_link_id].endp_type) == T) && (i_link_ctl_arr[l_link_id].endp_unit_id == l_loc_unit_id))
		{
			o_link_id = l_link_id;
			l_found = true;
		}
	}
}

void avsDriveCommand(const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target,
				const uint8_t  i_avsBusNum,
				const uint8_t  i_o2sBridgeNum,
				const uint32_t i_RailSelect,
				const uint32_t i_CmdType,
				const uint32_t i_CmdGroup,
				enum avsBusOpType i_opType)
{
	fapi2::buffer<uint64_t> l_data64;
	fapi2::buffer<uint32_t> l_data64WithoutCRC;
	uint32_t l_crc;
	// clear sticky bits in o2s_status_reg
	l_data64.setBit<1, 1>();
	putScom(i_target, p9avslib::OCB_O2SCMD[i_avsBusNum][i_o2sBridgeNum], l_data64);
	// MSB sent out first always, which should be start code 0b01
	// compose and send frame
	//      CRC(31:29),
	//      l_Reserved(28:13) (read), CmdData(28:13) (write)
	//      RailSelect(12:9),
	//      l_CmdDataType(8:5),
	//      l_CmdGroup(4),
	//      l_CmdType(3:2),
	//      l_StartCode(1:0)
	l_data64.flush<0>();
	l_data64.insertFromRight<0, 2>(1);
	l_data64.insertFromRight<2, 2>(i_CmdType);
	l_data64.insertFromRight<4, 1>(i_CmdGroup);
	l_data64.insertFromRight<5, 4>(0);
	l_data64.insertFromRight<9, 4>(i_RailSelect);
	l_data64.insertFromRight<13, 16>(0);
	l_data64.insertFromRight<29, 3>(0);
	// Generate CRC
	l_data64.extract(l_data64WithoutCRC, 0, 32);
	l_crc = avsCRCcalc(l_data64WithoutCRC);
	l_data64.insertFromRight<29, 3>(l_crc);
	putScom(i_target, p9avslib::OCB_O2SWD[i_avsBusNum][i_o2sBridgeNum], l_data64);
	// Wait on o2s_ongoing = 0
	avsPollVoltageTransDone(i_target, i_avsBusNum, i_o2sBridgeNum);
}
