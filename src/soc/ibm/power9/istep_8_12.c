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
	FAPI_ATTR_GET(fapi2::CHIP_EC_FEATURE_VDM_NOT_SUPPORTED, iv_procChip, iv_attrs.attr_dd_vdm_not_supported); // 1 if EC == 0x20
	FAPI_ATTR_GET(fapi2::CHIP_EC_FEATURE_WOF_NOT_SUPPORTED, iv_procChip, iv_attrs.attr_dd_wof_not_supported); // 1 if EC == 0x20
	FAPI_ATTR_GET(fapi2::FREQ_CORE_FLOOR_MHZ, FAPI_SYSTEM, iv_attrs.attr_freq_core_floor_mhz);
	FAPI_ATTR_GET(fapi2::ATTR_FREQ_DPLL_REFCLOCK_KHZ, FAPI_SYSTEM, iv_attrs.attr_freq_dpll_refclock_khz);
	FAPI_ATTR_GET(fapi2::ATTR_FREQ_PB_MHZ, FAPI_SYSTEM, iv_attrs.attr_nest_frequency_mhz);
	FAPI_ATTR_GET(fapi2::ATTR_FREQ_CORE_CEILING_MHZ, FAPI_SYSTEM, iv_attrs.attr_freq_core_ceiling_mhz);
	FAPI_ATTR_GET(fapi2::ATTR_SAFE_MODE_FREQUENCY_MHZ,iv_procChip, iv_attrs.attr_pm_safe_frequency_mhz);
	FAPI_ATTR_GET(fapi2::ATTR_SAFE_MODE_VOLTAGE_MV, iv_procChip, iv_attrs.attr_pm_safe_voltage_mv);
	FAPI_ATTR_GET(fapi2::ATTR_VCS_BOOT_VOLTAGE, iv_procChip, iv_attrs.vcs_voltage_mv);
	FAPI_ATTR_GET(fapi2::ATTR_VDD_BOOT_VOLTAGE, iv_procChip, iv_attrs.vdd_voltage_mv);
	FAPI_ATTR_GET(fapi2::ATTR_VDN_BOOT_VOLTAGE, iv_procChip, iv_attrs.vdn_voltage_mv);
	FAPI_ATTR_GET(fapi2::ATTR_PROC_DPLL_DIVIDER, iv_procChip, iv_attrs.proc_dpll_divider);

	FAPI_ATTR_SET(fapi2::ATTR_PROC_DPLL_DIVIDER, iv_procChip, 8);

	// Deal with defaults if attributes are not set
#define SET_DEFAULT(_attr_name, _attr_default) \
	if (!(iv_attrs._attr_name)) \
	{ \
	   iv_attrs._attr_name = _attr_default; \
	}

	SET_DEFAULT(attr_freq_dpll_refclock_khz, 133333);
	SET_DEFAULT(freq_proc_refclock_khz,      133333); // Future: collapse this out
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

	FAPI_ATTR_GET(fapi2::ATTR_VDM_DROOP_SMALL_OVERRIDE, FAPI_SYSTEM, iv_vdmpb.droop_small_override);
	FAPI_ATTR_GET(fapi2::ATTR_VDM_DROOP_LARGE_OVERRIDE, FAPI_SYSTEM, iv_vdmpb.droop_large_override);
	FAPI_ATTR_GET(fapi2::ATTR_VDM_DROOP_EXTREME_OVERRIDE, FAPI_SYSTEM, iv_vdmpb.droop_extreme_override);
	FAPI_ATTR_GET(fapi2::ATTR_VDM_OVERVOLT_OVERRIDE, FAPI_SYSTEM, iv_vdmpb.overvolt_override);
	FAPI_ATTR_GET(fapi2::ATTR_VDM_FMIN_OVERRIDE_KHZ, FAPI_SYSTEM, iv_vdmpb.fmin_override_khz);
	FAPI_ATTR_GET(fapi2::ATTR_VDM_FMAX_OVERRIDE_KHZ, FAPI_SYSTEM, iv_vdmpb.fmax_override_khz);
	FAPI_ATTR_GET(fapi2::ATTR_VDM_VID_COMPARE_OVERRIDE_MV, FAPI_SYSTEM, iv_vdmpb.vid_compare_override_mv);
	FAPI_ATTR_GET(api2::ATTR_DPLL_VDM_RESPONSE, FAPI_SYSTEM, iv_vdmpb.vdm_response);

	SET_ATTR(fapi2::ATTR_PSTATES_ENABLED, iv_procChip, 0);
	SET_ATTR(fapi2::ATTR_RESCLK_ENABLED,  iv_procChip, 0);
	SET_ATTR(fapi2::ATTR_VDM_ENABLED,     iv_procChip, 0);
	SET_ATTR(fapi2::ATTR_IVRM_ENABLED,    iv_procChip, 0);
	SET_ATTR(fapi2::ATTR_WOF_ENABLED,     iv_procChip, 0);
	SET_ATTR(fapi2::ATTR_WOV_UNDERV_ENABLED, iv_procChip, 0);
	SET_ATTR(fapi2::ATTR_WOV_OVERV_ENABLED, iv_procChip, 0);

	iv_pstates_enabled = true;
	iv_resclk_enabled  = true;
	iv_vdm_enabled     = true;
	iv_ivrm_enabled    = true;
	iv_wof_enabled     = true;
	iv_wov_underv_enabled = true;
	iv_wov_overv_enabled = true;
	iv_frequency_step_khz = (iv_attrs.attr_freq_dpll_refclock_khz / 8);
	iv_nest_freq_mhz      = iv_attrs.attr_nest_frequency_mhz;
}

void p9_setup_evid (chiplet_id_t proc_chip_target)
{
	pm_pstate_parameter_block::AttributeList attrs;
	// Instantiate PPB object
	PlatPmPPB l_pmPPB(proc_chip_target);
	l_pmPPB.attr_init();
	// Compute the boot/safe values
	l_pmPPB.compute_boot_safe();
	memcpy(&attrs ,&iv_attrs, sizeof(iv_attrs));
	// Set the DPLL frequency values to safe mode values
	p9_setup_dpll_values(
		proc_chip_target,
		133333,
		attrs.proc_dpll_divider);

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

static void p9_setup_dpll_values(
	const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target,
	const uint32_t i_freq_proc_refclock_khz,
	const uint32_t i_proc_dpll_divider)

{
	std::vector<fapi2::Target<fapi2::TARGET_TYPE_EQ>> l_eqChiplets;
	fapi2::Target<fapi2::TARGET_TYPE_EQ> l_firstEqChiplet;
	l_eqChiplets = i_target.getChildren<fapi2::TARGET_TYPE_EQ>(fapi2::TARGET_STATE_FUNCTIONAL);
	fapi2::buffer<uint64_t> l_data64;
	fapi2::buffer<uint64_t> l_fmult;
	const fapi2::Target<fapi2::TARGET_TYPE_SYSTEM> FAPI_SYSTEM;
	uint8_t l_chipNum = 0xFF;
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
		l_fmult.flush<0>();
		fapi2::getScom(*l_itr, EQ_QPPM_DPLL_FREQ , l_data64);
		l_data64.extractToRight<EQ_QPPM_DPLL_FREQ_FMULT, EQ_QPPM_DPLL_FREQ_FMULT_LEN>(l_fmult);
		// Convert frequency value to a format that needs to be written to the
		// register
		uint32_t l_safe_mode_dpll_value = l_attr_safe_mode_freq * 1000 * i_proc_dpll_divider / i_freq_proc_refclock_khz;
		FAPI_ATTR_GET(fapi2::ATTR_CHIP_UNIT_POS, *l_itr, l_chipNum);
		l_data64.insertFromRight<EQ_QPPM_DPLL_FREQ_FMAX, EQ_QPPM_DPLL_FREQ_FMAX_LEN>(l_safe_mode_dpll_value);
		l_data64.insertFromRight<EQ_QPPM_DPLL_FREQ_FMIN, EQ_QPPM_DPLL_FREQ_FMIN_LEN>(l_safe_mode_dpll_value);
		l_data64.insertFromRight<EQ_QPPM_DPLL_FREQ_FMULT, EQ_QPPM_DPLL_FREQ_FMULT_LEN>(l_safe_mode_dpll_value);
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

static void p9_fbc_ioo_dl_scom(chiplet_id_t obus_target)
{
	// Power Bus OLL Configuration Register
	// PB.IOO.LL0.IOOL_CONFIG
	// Processor bus OLL configuration register
	// l_PB_IOO_LL0_CONFIG_LINK_PAIR_OFF
	// l_PB_IOO_LL0_CONFIG_CRC_LANE_ID_ON
	// l_PB_IOO_LL0_CONFIG_SL_UE_CRC_ERR_ON
	scom_and_for_chiplet(obus_target, 0x901080A, ~(PPC_BIT(0) | PPC_BIT(15) | PPC_BITMASK(32, 35)), 0x280F000F0000F000)
	// Power Bus OLL PHY Training Configuration Register
	// PB.IOO.LL0.IOOL_PHY_CONFIG
	// Processor bus OLL PHY training configuration register
	// l_PB_IOO_LL0_CONFIG_PHY_TRAIN_A_ADJ_USE4
	// l_PB_IOO_LL0_CONFIG_PHY_TRAIN_B_ADJ_USE12
	// l_PB_IOO_LL0_CONFIG_LINK0_OLL_ENABLED_OFF
	// l_PB_IOO_LL0_CONFIG_LINK1_OLL_ENABLED_OFF
	scom_and_or_for_chiplet(obus_target, 0x901080C, ~(PPC_BITMASK(0, 15) | PPC_BITMASK(58, 59)), PPC_BIT(0) | PPC_BIT(2) | PPC_BITMASK(4, 7));
	// Power Bus OLL Optical Configuration Register
	// PB.IOO.LL0.IOOL_OPTICAL_CONFIG
	// Processor bus OLL optical configuration register
	// l_PB_IOO_LL0_CONFIG_ELEVEN_LANE_MODE_ON
	// l_PB_IOO_LL0_CONFIG_LINK_FAIL_CRC_ERROR_ON
	// l_PB_IOO_LL0_CONFIG_LINK_FAIL_NO_SPARE_ON
	// l_PB_IOO_LL0_CONFIG_REPLAY_BUFFER_SIZE_REPLAY
	// l_PB_IOO_LL0_LINK1_ELEVEN_LANE_SHIFT_ON
	// l_PB_IOO_LL0_LINK1_RX_LANE_SWAP_ON
	// l_PB_IOO_LL0_LINK1_TX_LANE_SWAP_ON
	scom_and_or_for_chiplet(obus_target, 0x901080F, 0xF080F0FFFFFFFFBF, 0x350F057F05300080);
	// Power Bus OLL Replay Threshold Register
	// PB.IOO.LL0.IOOL_REPLAY_THRESHOLD
	// Processor bus OLL replay threshold register
	scom_and_or_for_chiplet(obus_target, 0x9010818, ~PPC_BITMASK(0, 10), PPC_BITMASK(1, 2) | PPC_BITMASK(4, 10));
	// Power Bus OLL SL ECC Threshold Register
	// PB.IOO.LL0.IOOL_SL_ECC_THRESHOLD
	// Processor bus OLL SL ECC Threshold register
	scom_and_or_for_chiplet(obus_target, 0x9010819, ~PPC_BITMASK(0, 9), PPC_BITMASK(1, 9));
	// Power Bus OLL Retrain Threshold Register
	// PB.IOO.LL0.IOOL_RETRAIN_THRESHOLD
	// Processor bus OLL retrain threshold register
	scom_and_or_for_chiplet(obus_target, 0x901081A, ~PPC_BITMASK(0, 16), PPC_BITMASK(1, 7) | PPC_BITMASK(14, 16));
}

fapi2::ReturnCode PlatPmPPB::compute_boot_safe()
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
			uint32_t l_int_vcs_mv = (iv_attr_mvpd_poundV_biased[POWERSAVE].vcs_mv);
			uint32_t l_ics_ma = (iv_attr_mvpd_poundV_biased[POWERSAVE].ics_100ma) * 100;
			uint32_t l_ext_vcs_mv = sysparm_uplift(l_int_vcs_mv,
					l_ics_ma,
					revle32(0),
					revle32(64),
					revle32(0));
			iv_attrs.vcs_voltage_mv = (l_ext_vcs_mv);

		}

		// set VDN voltage to PowerSave Voltage from MVPD data (if no override)
		if(!iv_attrs.vdn_voltage_mv)
		{
			uint32_t l_int_vdn_mv = (iv_attr_mvpd_poundV_biased[POWERBUS].vdd_mv);
			uint32_t l_idn_ma = (iv_attr_mvpd_poundV_biased[POWERBUS].idd_100ma) * 100;
			// Returns revle32
			uint32_t l_ext_vdn_mv = sysparm_uplift(l_int_vdn_mv,
					l_idn_ma,
					revle32(0),
					revle32(50),
					revle32(0));

			iv_attrs.vdn_voltage_mv = (l_ext_vdn_mv);
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

uint32_t sysparm_uplift(const uint32_t i_vpd_mv,
						const uint32_t i_vpd_ma,
						const uint32_t i_loadline_uohm,
						const uint32_t i_distloss_uohm,
						const uint32_t i_distoffset_uohm)
{
	return (uint32_t)((double)i_vpd_mv + (((double)(i_vpd_ma * (i_loadline_uohm + i_distloss_uohm)) / 1000 + (double)i_distoffset_uohm)) / 1000);
	//                              mV +           (mA       *             uOhm                   ) / 1000 +                       uV
	// mV + (mA * uOhm / 1000) / 1000
	// mV + mV + mV
}

fapi2::ReturnCode
avsInitExtVoltageControl(
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
	getScom(i_target, p9avslib::OCB_O2SRD[i_avsBusNum][i_o2sBridgeNum], l_data64));
	// Extracting bits 8:23 , which contains voltage read data
	l_present_voltage_mv = (l_data64 & 0x00FFFF0000000000) >> 40;
}

fapi2::ReturnCode
avsValidateResponse(const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target,
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
	getScom(i_target, p9avslib::OCB_O2SRD[i_avsBusNum][i_o2sBridgeNum], l_data64));

	// Status Return Code and Received CRC
	l_data64.extractToRight(l_data_status_code, 0, 2);
	l_data64.extractToRight(l_rsp_rcvd_crc, 29, 3);
	l_data64.extractToRight(l_rsp_data, 0, 32);

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
fapi2::ReturnCode p9_fbc_eff_config_links_query_endp(
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

fapi2::ReturnCode p9_fbc_ioo_tl_scom(const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& TGT0,
									 const fapi2::Target<fapi2::TARGET_TYPE_SYSTEM>& TGT1)
{
	fapi2::ATTR_EC_Type   l_chip_ec;
	fapi2::ATTR_NAME_Type l_chip_id;
	FAPI_ATTR_GET_PRIVILEGED(fapi2::ATTR_NAME, TGT0, l_chip_id);
	FAPI_ATTR_GET_PRIVILEGED(fapi2::ATTR_EC, TGT0, l_chip_ec);
	fapi2::ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG_Type l_TGT0_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG;
	FAPI_ATTR_GET(fapi2::ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG, TGT0, l_TGT0_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG);
	fapi2::ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_Type l_TGT0_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG;
	FAPI_ATTR_GET(fapi2::ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG, TGT0, l_TGT0_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG);
	uint64_t l_def_OBUS0_FBC_ENABLED =
			l_TGT0_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG[3] != fapi2::ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_FALSE
		 || l_TGT0_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG[0] != fapi2::ENUM_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG_FALSE;
	fapi2::ATTR_FREQ_A_MHZ_Type l_TGT1_ATTR_FREQ_A_MHZ;
	FAPI_ATTR_GET(fapi2::ATTR_FREQ_A_MHZ, TGT1, l_TGT1_ATTR_FREQ_A_MHZ);
	fapi2::ATTR_FREQ_PB_MHZ_Type l_TGT1_ATTR_FREQ_PB_MHZ;
	FAPI_ATTR_GET(fapi2::ATTR_FREQ_PB_MHZ, TGT1, l_TGT1_ATTR_FREQ_PB_MHZ);
	uint64_t l_def_LO_LIMIT_R = l_TGT1_ATTR_FREQ_PB_MHZ * 10 > l_TGT1_ATTR_FREQ_A_MHZ * 12;
	uint64_t l_def_OBUS0_LO_LIMIT_D = l_TGT1_ATTR_FREQ_A_MHZ * 10;
	uint64_t l_def_OBUS0_LO_LIMIT_N = l_TGT1_ATTR_FREQ_PB_MHZ * 154;
	uint64_t l_def_OBUS1_FBC_ENABLED =
			l_TGT0_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG[4] != fapi2::ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_FALSE
		 || l_TGT0_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG[1] != fapi2::ENUM_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG_FALSE;
	uint64_t l_def_OBUS1_LO_LIMIT_D = l_TGT1_ATTR_FREQ_A_MHZ;
	uint64_t l_def_OBUS1_LO_LIMIT_N = l_TGT1_ATTR_FREQ_PB_MHZ * 12;
	uint64_t l_def_OBUS2_FBC_ENABLED =
			l_TGT0_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG[5] != fapi2::ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_FALSE
		 || l_TGT0_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG[2] != fapi2::ENUM_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG_FALSE;
	uint64_t l_def_OBUS2_LO_LIMIT_D = l_TGT1_ATTR_FREQ_A_MHZ * 10;
	uint64_t l_def_OBUS2_LO_LIMIT_N = l_TGT1_ATTR_FREQ_PB_MHZ * 74;
	uint64_t l_def_OBUS3_FBC_ENABLED =
			l_TGT0_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG[6] != fapi2::ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_FALSE
		 || l_TGT0_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG[3] != fapi2::ENUM_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG_FALSE;
	uint64_t l_def_OBUS3_LO_LIMIT_D = l_TGT1_ATTR_FREQ_A_MHZ * 10;
	uint64_t l_def_OBUS3_LO_LIMIT_N = l_TGT1_ATTR_FREQ_PB_MHZ * 95;
	fapi2::ATTR_PROC_FABRIC_SMP_OPTICS_MODE_Type l_TGT1_ATTR_PROC_FABRIC_SMP_OPTICS_MODE;
	FAPI_ATTR_GET(fapi2::ATTR_PROC_FABRIC_SMP_OPTICS_MODE, TGT1, l_TGT1_ATTR_PROC_FABRIC_SMP_OPTICS_MODE);
	uint64_t l_def_OPTICS_IS_A_BUS = l_TGT1_ATTR_PROC_FABRIC_SMP_OPTICS_MODE == fapi2::ENUM_ATTR_PROC_FABRIC_SMP_OPTICS_MODE_OPTICS_IS_A_BUS;
	uint64_t l_def_OB0_IS_PAIRED =
			l_TGT0_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG[3] == fapi2::ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_TRUE
		 || l_TGT0_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG[0] == fapi2::ENUM_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG_TRUE;
	uint64_t l_def_OB1_IS_PAIRED =
			l_TGT0_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG[4] == fapi2::ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_TRUE
		 || l_TGT0_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG[1] == fapi2::ENUM_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG_TRUE;
	uint64_t l_def_OB2_IS_PAIRED =
			l_TGT0_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG[5] == fapi2::ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_TRUE
		 || l_TGT0_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG[2] == fapi2::ENUM_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG_TRUE;
	uint64_t l_def_OB3_IS_PAIRED =
			l_TGT0_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG[6] == fapi2::ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_TRUE
		 || l_TGT0_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG[3] == fapi2::ENUM_ATTR_PROC_FABRIC_A_ATTACHED_CHIP_CNFG_TRUE;
	fapi2::ATTR_PROC_NPU_REGION_ENABLED_Type l_TGT0_ATTR_PROC_NPU_REGION_ENABLED;
	FAPI_ATTR_GET(fapi2::ATTR_PROC_NPU_REGION_ENABLED, TGT0, l_TGT0_ATTR_PROC_NPU_REGION_ENABLED);
	fapi2::ATTR_PROC_FABRIC_OPTICS_CONFIG_MODE_Type l_TGT0_ATTR_PROC_FABRIC_OPTICS_CONFIG_MODE;
	FAPI_ATTR_GET(fapi2::ATTR_PROC_FABRIC_OPTICS_CONFIG_MODE, TGT0, l_TGT0_ATTR_PROC_FABRIC_OPTICS_CONFIG_MODE);
	uint64_t l_def_NVLINK_ACTIVE = (
			 l_TGT0_ATTR_PROC_FABRIC_OPTICS_CONFIG_MODE[0] == fapi2::ENUM_ATTR_PROC_FABRIC_OPTICS_CONFIG_MODE_NV
		  || l_TGT0_ATTR_PROC_FABRIC_OPTICS_CONFIG_MODE[1] == fapi2::ENUM_ATTR_PROC_FABRIC_OPTICS_CONFIG_MODE_NV
		  || l_TGT0_ATTR_PROC_FABRIC_OPTICS_CONFIG_MODE[2] == fapi2::ENUM_ATTR_PROC_FABRIC_OPTICS_CONFIG_MODE_NV
		  || l_TGT0_ATTR_PROC_FABRIC_OPTICS_CONFIG_MODE[3] == fapi2::ENUM_ATTR_PROC_FABRIC_OPTICS_CONFIG_MODE_NV)
		  && l_TGT0_ATTR_PROC_NPU_REGION_ENABLED;
	fapi2::buffer<uint64_t> l_scom_buffer;

	// Power Bus Optical Framer/Parser 01 Configuration Register
	// PB.IOO.SCOM.PB_FP01_CFG
	fapi2::getScom(TGT0, 0x501380A, l_scom_buffer);
	if (l_def_OBUS0_FBC_ENABLED)
	{
		constexpr auto l_PB_IOO_SCOM_A0_MODE_NORMAL = 0x0;
		l_scom_buffer.insert<20, 1, 60, uint64_t>(l_PB_IOO_SCOM_A0_MODE_NORMAL);
		l_scom_buffer.insert<25, 1, 61, uint64_t>(l_PB_IOO_SCOM_A0_MODE_NORMAL);
		l_scom_buffer.insert<52, 1, 62, uint64_t>(l_PB_IOO_SCOM_A0_MODE_NORMAL);
		l_scom_buffer.insert<57, 1, 63, uint64_t>(l_PB_IOO_SCOM_A0_MODE_NORMAL);
		l_scom_buffer.insert<22, 2, 62, uint64_t>(1);
		l_scom_buffer.insert<12, 8, 56, uint64_t>(0x40);
		l_scom_buffer.insert<44, 8, 56, uint64_t>(0x40);
		l_scom_buffer.insert<36, 8, 56, uint64_t>(0x37 - (l_def_OBUS0_LO_LIMIT_N / l_def_OBUS0_LO_LIMIT_D));
		if (l_def_LO_LIMIT_R == 1)
		{
			l_scom_buffer.insert<4, 8, 56, uint64_t>(0x36 - (l_def_OBUS0_LO_LIMIT_N / l_def_OBUS0_LO_LIMIT_D));
			l_scom_buffer.insert<36, 8, 56, uint64_t>(0x36 - (l_def_OBUS0_LO_LIMIT_N / l_def_OBUS0_LO_LIMIT_D));
		}
		else
		{
			l_scom_buffer.insert<4, 8, 56, uint64_t>(0x37 - (l_def_OBUS0_LO_LIMIT_N / l_def_OBUS0_LO_LIMIT_D));
		}
	}
	else
	{
		constexpr auto l_PB_IOO_SCOM_A0_MODE_BLOCKED = 0xF;
		l_scom_buffer.insert<20, 1, 60, uint64_t>(l_PB_IOO_SCOM_A0_MODE_BLOCKED);
		l_scom_buffer.insert<25, 1, 61, uint64_t>(l_PB_IOO_SCOM_A0_MODE_BLOCKED);
		l_scom_buffer.insert<52, 1, 62, uint64_t>(l_PB_IOO_SCOM_A0_MODE_BLOCKED);
		l_scom_buffer.insert<57, 1, 63, uint64_t>(l_PB_IOO_SCOM_A0_MODE_BLOCKED);
	}
	fapi2::putScom(TGT0, 0x501380A, l_scom_buffer);
	// Power Bus Optical Framer/Parser 23 Configuration Register
	// PB.IOO.SCOM.PB_FP23_CFG
	fapi2::getScom(TGT0, 0x501380B, l_scom_buffer);
	if (l_def_OBUS1_FBC_ENABLED)
	{
		constexpr auto l_PB_IOO_SCOM_A1_MODE_NORMAL = 0x0;
		l_scom_buffer.insert<20, 1, 60, uint64_t>(l_PB_IOO_SCOM_A1_MODE_NORMAL);
		l_scom_buffer.insert<25, 1, 61, uint64_t>(l_PB_IOO_SCOM_A1_MODE_NORMAL);
		l_scom_buffer.insert<52, 1, 62, uint64_t>(l_PB_IOO_SCOM_A1_MODE_NORMAL);
		l_scom_buffer.insert<57, 1, 63, uint64_t>(l_PB_IOO_SCOM_A1_MODE_NORMAL);
		l_scom_buffer.insert<22, 2, 62, uint64_t>(1);
		l_scom_buffer.insert<12, 8, 56, uint64_t>(0x40);
		l_scom_buffer.insert<44, 8, 56, uint64_t>(0x40);
		if (l_def_LO_LIMIT_R == 1)
		{
			l_scom_buffer.insert<4, 8, 56, uint64_t>(0x2A - (l_def_OBUS1_LO_LIMIT_N / l_def_OBUS1_LO_LIMIT_D));
			l_scom_buffer.insert<36, 8, 56, uint64_t>(0x2A - (l_def_OBUS1_LO_LIMIT_N / l_def_OBUS1_LO_LIMIT_D));
		}
		else
		{
			l_scom_buffer.insert<4, 8, 56, uint64_t>(0x2C - (l_def_OBUS1_LO_LIMIT_N / l_def_OBUS1_LO_LIMIT_D));
			l_scom_buffer.insert<36, 8, 56, uint64_t>(0x2C - (l_def_OBUS1_LO_LIMIT_N / l_def_OBUS1_LO_LIMIT_D));
		}
	}
	else
	{
		constexpr auto l_PB_IOO_SCOM_A1_MODE_BLOCKED = 0xF;
		l_scom_buffer.insert<20, 1, 60, uint64_t>(l_PB_IOO_SCOM_A1_MODE_BLOCKED);
		l_scom_buffer.insert<25, 1, 61, uint64_t>(l_PB_IOO_SCOM_A1_MODE_BLOCKED);
		l_scom_buffer.insert<52, 1, 62, uint64_t>(l_PB_IOO_SCOM_A1_MODE_BLOCKED);
		l_scom_buffer.insert<57, 1, 63, uint64_t>(l_PB_IOO_SCOM_A1_MODE_BLOCKED);
	}
	fapi2::putScom(TGT0, 0x501380B, l_scom_buffer);
	// Power Bus Optical Framer/Parser 45 Configuration Register
	// PB.IOO.SCOM.PB_FP45_CFG
	fapi2::getScom(TGT0, 0x501380C, l_scom_buffer);
	if (l_def_OBUS2_FBC_ENABLED)
	{
		constexpr auto l_PB_IOO_SCOM_A2_MODE_NORMAL = 0x0;
		l_scom_buffer.insert<20, 1, 60, uint64_t>(l_PB_IOO_SCOM_A2_MODE_NORMAL);
		l_scom_buffer.insert<25, 1, 61, uint64_t>(l_PB_IOO_SCOM_A2_MODE_NORMAL);
		l_scom_buffer.insert<52, 1, 62, uint64_t>(l_PB_IOO_SCOM_A2_MODE_NORMAL);
		l_scom_buffer.insert<57, 1, 63, uint64_t>(l_PB_IOO_SCOM_A2_MODE_NORMAL);
		l_scom_buffer.insert<22, 2, 62, uint64_t>(1);
		l_scom_buffer.insert<12, 8, 56, uint64_t>(0x40);
		l_scom_buffer.insert<44, 8, 56, uint64_t>(0x40);
		if (l_def_LO_LIMIT_R == 1)
		{
			l_scom_buffer.insert<4, 8, 56, uint64_t>(0x1B - (l_def_OBUS2_LO_LIMIT_N / l_def_OBUS2_LO_LIMIT_D));
			l_scom_buffer.insert<36, 8, 56, uint64_t>(0x1B - (l_def_OBUS2_LO_LIMIT_N / l_def_OBUS2_LO_LIMIT_D));
		}
		else
		{
			l_scom_buffer.insert<4, 8, 56, uint64_t>(0x1C - (l_def_OBUS2_LO_LIMIT_N / l_def_OBUS2_LO_LIMIT_D));
			l_scom_buffer.insert<36, 8, 56, uint64_t>(0x1C - (l_def_OBUS2_LO_LIMIT_N / l_def_OBUS2_LO_LIMIT_D));
		}
	}
	else
	{
		constexpr auto l_PB_IOO_SCOM_A2_MODE_BLOCKED = 0xF;
		l_scom_buffer.insert<20, 1, 60, uint64_t>(l_PB_IOO_SCOM_A2_MODE_BLOCKED);
		l_scom_buffer.insert<25, 1, 61, uint64_t>(l_PB_IOO_SCOM_A2_MODE_BLOCKED);
		l_scom_buffer.insert<52, 1, 62, uint64_t>(l_PB_IOO_SCOM_A2_MODE_BLOCKED);
		l_scom_buffer.insert<57, 1, 63, uint64_t>(l_PB_IOO_SCOM_A2_MODE_BLOCKED);
	}
	fapi2::putScom(TGT0, 0x501380C, l_scom_buffer);
	// Power Bus Optical Framer/Parser 67 Configuration Register
	// PB.IOO.SCOM.PB_FP67_CFG
	fapi2::getScom(TGT0, 0x501380D, l_scom_buffer);
	if (l_def_OBUS3_FBC_ENABLED)
	{
		constexpr auto l_PB_IOO_SCOM_A3_MODE_NORMAL = 0x0;
		l_scom_buffer.insert<20, 1, 60, uint64_t>(l_PB_IOO_SCOM_A3_MODE_NORMAL);
		l_scom_buffer.insert<25, 1, 61, uint64_t>(l_PB_IOO_SCOM_A3_MODE_NORMAL);
		l_scom_buffer.insert<52, 1, 62, uint64_t>(l_PB_IOO_SCOM_A3_MODE_NORMAL);
		l_scom_buffer.insert<57, 1, 63, uint64_t>(l_PB_IOO_SCOM_A3_MODE_NORMAL);
		l_scom_buffer.insert<22, 2, 62, uint64_t>(1);
		l_scom_buffer.insert<12, 8, 56, uint64_t>(0x40);
		l_scom_buffer.insert<44, 8, 56, uint64_t>(0x40);
		if (l_def_LO_LIMIT_R == 1)
		{
			l_scom_buffer.insert<4, 8, 56, uint64_t>(0x22 - (l_def_OBUS3_LO_LIMIT_N / l_def_OBUS3_LO_LIMIT_D));
			l_scom_buffer.insert<36, 8, 56, uint64_t>(0x22 - (l_def_OBUS3_LO_LIMIT_N / l_def_OBUS3_LO_LIMIT_D));
		}
		else
		{
			l_scom_buffer.insert<4, 8, 56, uint64_t>(0x24 - (l_def_OBUS3_LO_LIMIT_N / l_def_OBUS3_LO_LIMIT_D));
			l_scom_buffer.insert<36, 8, 56, uint64_t>(0x24 - (l_def_OBUS3_LO_LIMIT_N / l_def_OBUS3_LO_LIMIT_D));
		}
	}
	else
	{
		constexpr auto l_PB_IOO_SCOM_A3_MODE_BLOCKED = 0xF;
		l_scom_buffer.insert<20, 1, 60, uint64_t>(l_PB_IOO_SCOM_A3_MODE_BLOCKED);
		l_scom_buffer.insert<25, 1, 61, uint64_t>(l_PB_IOO_SCOM_A3_MODE_BLOCKED);
		l_scom_buffer.insert<52, 1, 62, uint64_t>(l_PB_IOO_SCOM_A3_MODE_BLOCKED);
		l_scom_buffer.insert<57, 1, 63, uint64_t>(l_PB_IOO_SCOM_A3_MODE_BLOCKED);
	}
	fapi2::putScom(TGT0, 0x501380D, l_scom_buffer);
	// Power Bus Optical Link Data Buffer 01 Configuration Register
	// PB.IOO.SCOM.PB_OLINK_DATA_01_CFG_REG
	fapi2::getScom(TGT0, 0x5013810, l_scom_buffer);
	if(l_def_OBUS0_FBC_ENABLED)
	{
		if (l_def_OPTICS_IS_A_BUS)
		{
			l_scom_buffer.insert<24, 5, 59, uint64_t>(0x10);
		}
		else
		{
			l_scom_buffer.insert<24, 5, 59, uint64_t>(0x1F);
		}
		l_scom_buffer.insert<9, 7, 57, uint64_t>(0x3C);
		l_scom_buffer.insert<17, 7, 57, uint64_t>(0x3C);
		l_scom_buffer.insert<41, 7, 57, uint64_t>(0x3C);
		l_scom_buffer.insert<49, 7, 57, uint64_t>(0x3C);
	}
	fapi2::putScom(TGT0, 0x5013810, l_scom_buffer);
	fapi2::getScom(TGT0, 0x5013811, l_scom_buffer);
	if (l_def_OBUS1_FBC_ENABLED)
	{
		l_scom_buffer.insert<1, 7, 57, uint64_t>(0x40);
		l_scom_buffer.insert<9, 7, 57, uint64_t>(0x3C);
		l_scom_buffer.insert<17, 7, 57, uint64_t>(0x3C);
		l_scom_buffer.insert<33, 7, 57, uint64_t>(0x40);
		l_scom_buffer.insert<41, 7, 57, uint64_t>(0x3C);
		l_scom_buffer.insert<49, 7, 57, uint64_t>(0x3C);
		if (l_def_OPTICS_IS_A_BUS)
		{
			l_scom_buffer.insert<24, 5, 59, uint64_t>(0x10);
		}
		else
		{
			l_scom_buffer.insert<24, 5, 59, uint64_t>(0x1F);
		}
	}
	fapi2::putScom(TGT0, 0x5013811, l_scom_buffer);
	// Power Bus Optical Link Data Buffer 45 Configuration Register
	// PB.IOO.SCOM.PB_OLINK_DATA_45_CFG_REG
	fapi2::getScom(TGT0, 0x5013812, l_scom_buffer);
	if (l_def_OBUS2_FBC_ENABLED)
	{
		if (l_def_OPTICS_IS_A_BUS)
		{
			l_scom_buffer.insert<24, 5, 59, uint64_t>(0x10);
		}
		else
		{
			l_scom_buffer.insert<24, 5, 59, uint64_t>(0x1F);
		}
		l_scom_buffer.insert<1, 7, 57, uint64_t>(0x40);
		l_scom_buffer.insert<9, 7, 57, uint64_t>(0x3C);
		l_scom_buffer.insert<17, 7, 57, uint64_t>(0x3C);
		l_scom_buffer.insert<33, 7, 57, uint64_t>(0x40);
		l_scom_buffer.insert<41, 7, 57, uint64_t>(0x3C);
		l_scom_buffer.insert<49, 7, 57, uint64_t>(0x3C);
	}
	fapi2::putScom(TGT0, 0x5013812, l_scom_buffer);
	// Power Bus Optical Link Data Buffer 67 Configuration Register
	// PB.IOO.SCOM.PB_OLINK_DATA_67_CFG_REG
	fapi2::getScom(TGT0, 0x5013813, l_scom_buffer);
	if(l_def_OBUS3_FBC_ENABLED)
	{
		if (l_def_OPTICS_IS_A_BUS)
		{
			l_scom_buffer.insert<24, 5, 59, uint64_t>(0x0E);
		}
		else
		{
			l_scom_buffer.insert<24, 5, 59, uint64_t>(0x1C);
		}
		l_scom_buffer.insert<9, 7, 57, uint64_t>(0x3C);
		l_scom_buffer.insert<17, 7, 57, uint64_t>(0x3C);
		l_scom_buffer.insert<41, 7, 57, uint64_t>(0x3C);
		l_scom_buffer.insert<49, 7, 57, uint64_t>(0x3C);
	}
	fapi2::putScom(TGT0, 0x5013813, l_scom_buffer);
	// Power Bus Optical Miscellaneous Configuration Register
	// PB.IOO.SCOM.PB_MISC_CFG
	fapi2::getScom(TGT0, 0x5013823, l_scom_buffer);
	if (l_def_OB0_IS_PAIRED)
	{
		l_scom_buffer.insert<0, 1, 63, uint64_t>(0x1); // l_PB_IOO_SCOM_PB_CFG_IOO01_IS_LOGICAL_PAIR_ON
	}
	else
	{
		l_scom_buffer.insert<0, 1, 63, uint64_t>(0x0); // l_PB_IOO_SCOM_PB_CFG_IOO01_IS_LOGICAL_PAIR_OFF
	}
	if (l_def_OBUS0_FBC_ENABLED)
	{
		l_scom_buffer.insert<8, 1, 63, uint64_t>(0x1); // l_PB_IOO_SCOM_LINKS01_TOD_ENABLE_ON
	}
	else
	{
		l_scom_buffer.insert<8, 1, 63, uint64_t>(0x0); // l_PB_IOO_SCOM_LINKS01_TOD_ENABLE_OFF
	}
	if (l_def_OB1_IS_PAIRED)
	{
		l_scom_buffer.insert<1, 1, 63, uint64_t>(0x1); // l_PB_IOO_SCOM_PB_CFG_IOO23_IS_LOGICAL_PAIR_ON
	}
	else
	{
		l_scom_buffer.insert<1, 1, 63, uint64_t>(0x0); // l_PB_IOO_SCOM_PB_CFG_IOO23_IS_LOGICAL_PAIR_OFF
	}
	if (l_def_OBUS1_FBC_ENABLED)
	{
		l_scom_buffer.insert<9, 1, 63, uint64_t>(0x1); // l_PB_IOO_SCOM_LINKS23_TOD_ENABLE_ON
	}
	else
	{
		l_scom_buffer.insert<9, 1, 63, uint64_t>(0x0); // l_PB_IOO_SCOM_LINKS23_TOD_ENABLE_OFF
	}
	if (l_def_OB2_IS_PAIRED)
	{
		l_scom_buffer.insert<2, 1, 63, uint64_t>(0x1); // l_PB_IOO_SCOM_PB_CFG_IOO45_IS_LOGICAL_PAIR_ON
	}
	else
	{
		l_scom_buffer.insert<2, 1, 63, uint64_t>(0x0); // l_PB_IOO_SCOM_PB_CFG_IOO45_IS_LOGICAL_PAIR_OFF
	}
	if (l_def_OBUS2_FBC_ENABLED)
	{
		l_scom_buffer.insert<10, 1, 63, uint64_t>(0x1); // l_PB_IOO_SCOM_LINKS45_TOD_ENABLE_ON
	}
	else
	{
		l_scom_buffer.insert<10, 1, 63, uint64_t>(0x0); // l_PB_IOO_SCOM_LINKS45_TOD_ENABLE_OFF
	}
	if (l_def_OB3_IS_PAIRED)
	{
		l_scom_buffer.insert<3, 1, 63, uint64_t>(0x1); // l_PB_IOO_SCOM_PB_CFG_IOO67_IS_LOGICAL_PAIR_ON
	}
	else
	{
		l_scom_buffer.insert<3, 1, 63, uint64_t>(0x0); // l_PB_IOO_SCOM_PB_CFG_IOO67_IS_LOGICAL_PAIR_OFF
	}
	if (l_def_OBUS3_FBC_ENABLED)
	{
		l_scom_buffer.insert<11, 1, 63, uint64_t>(0x1); // l_PB_IOO_SCOM_LINKS67_TOD_ENABLE_ON
	}
	else
	{
		l_scom_buffer.insert<11, 1, 63, uint64_t>(0x0); // l_PB_IOO_SCOM_LINKS67_TOD_ENABLE_OFF
	}
	if (l_def_NVLINK_ACTIVE)
	{
		l_scom_buffer.insert<13, 1, 63, uint64_t>(0x1); // l_PB_IOO_SCOM_SEL_03_NPU_NOT_PB_ON
		l_scom_buffer.insert<14, 1, 63, uint64_t>(0x1); // l_PB_IOO_SCOM_SEL_04_NPU_NOT_PB_ON
		l_scom_buffer.insert<15, 1, 63, uint64_t>(0x1); // l_PB_IOO_SCOM_SEL_05_NPU_NOT_PB_ON
	}
	fapi2::putScom(TGT0, 0x5013823, l_scom_buffer);
	// Power Bus Optical Link Trace Configuration Register
	// PB.IOO.SCOM.PB_TRACE_CFG
	// Trace group encodes:
	// 0 = disabled
	// 1 = inbound link
	// 2 = inbound dhdr position 0
	// 3 = inbound dhdr position 1
	// 4 = inbound dhdr position 2
	// 5 = outbound link
	// 6 = outbound dhdr position 0
	// 7 = outbound dhdr position 1
	// 8 = outbound dhdr position 2
	// 9 = dob1 pos 0
	// A = dob1 pos 1
	// B = dob2 pos 0
	// C = dob2 pos
	// D = dib pos 0
	// E = dib pos 1
	// F = Reserved.
	fapi2::getScom(TGT0, 0x5013824, l_scom_buffer);
	if (l_def_OBUS0_FBC_ENABLED)
	{
		l_scom_buffer.insert<0, 4, 60, uint64_t>(1);
		l_scom_buffer.insert<8, 4, 60, uint64_t>(1);
		l_scom_buffer.insert<4, 4, 60, uint64_t>(4);
		l_scom_buffer.insert<12, 4, 60, uint64_t>(4);
	}
	else if(l_def_OBUS1_FBC_ENABLED)
	{
		l_scom_buffer.insert<16, 4, 60, uint64_t>(1);
		l_scom_buffer.insert<24, 4, 60, uint64_t>(1);
		l_scom_buffer.insert<20, 4, 60, uint64_t>(4);
		l_scom_buffer.insert<28, 4, 60, uint64_t>(4);
	}
	else if(l_def_OBUS2_FBC_ENABLED)
	{
		l_scom_buffer.insert<32, 4, 60, uint64_t>(1);
		l_scom_buffer.insert<40, 4, 60, uint64_t>(1);
		l_scom_buffer.insert<36, 4, 60, uint64_t>(4);
		l_scom_buffer.insert<44, 4, 60, uint64_t>(4);
	}
	else if(l_def_OBUS3_FBC_ENABLED)
	{
		l_scom_buffer.insert<48, 4, 60, uint64_t>(1);
		l_scom_buffer.insert<56, 4, 60, uint64_t>(1);
		l_scom_buffer.insert<52, 4, 60, uint64_t>(4);
		l_scom_buffer.insert<60, 4, 60, uint64_t>(4);
	}
	fapi2::putScom(TGT0, 0x5013824, l_scom_buffer);
}

fapi2::ReturnCode
avsDriveCommand(const fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>& i_target,
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
