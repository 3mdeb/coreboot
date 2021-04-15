/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <console/console.h>
#include <soc/istep8.h>

static int p9_query_config_links(const uint8_t *i_target, 
				 uint8_t *o_link_is_enabled)
{	

	switch(*i_target)
	{
	case TARGET_TYPE_XBUS:
		ATTR_LINK_TRAIN_TARGET_TYPE_XBUS l_link_train;	
		if (l_link_train == ENUM_ATTR_LINK_TRAIN_BOTH)
		{
			*o_link_is_enabled = ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_TRUE;
		}
		else if (l_link_train == ENUM_ATTR_LINK_TRAIN_EVEN_ONLY)
		{
			*o_link_is_enabled = ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_EVEN_ONLY;
		}
		else if (l_link_train == ENUM_ATTR_LINK_TRAIN_ODD_ONLY)
		{
			*o_link_is_enabled = ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_ODD_ONLY;
		}
		else
		{
			*o_link_is_enabled = ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_FALSE;
			return -1;
		}
		break;
	case TARGET_TYPE_OBUS:
		ATTR_OPTICS_CONFIG_MODE_TARGET_TYPE_OBUS l_link_config_mode;
		if (l_link_config_mode != ENUM_ATTR_OPTICS_CONFIG_MODE_SMP)
		{
			*o_link_is_enabled = ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_FALSE;
		}
		else
		{
			ATTR_LINK_TRAIN_TARGET_TYPE_OBUS l_link_train;
			if (l_link_train = ENUM_ATTR_LINK_TRAIN_BOTH)
			{
				*o_link_is_enabled = ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_TRUE;
			}
			else if (l_link_train == ENUM_ATTR_LINK_TRAIN_EVEN_ONLY)
			{
				*o_link_is_enabled = ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_EVEN_ONLY;
			}
			else if (l_link_train == ENUM_ATTR_LINK_TRAIN_ODD_ONLY)
			{
				*o_link_is_enabled = ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_ODD_ONLY;
			}
			else
			{
	
				*o_link_is_enabled = ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNG_FALSE;
				return -1;
			}
		}	
		break;
	default:
		printk(BIOS_WARNING, "Invalid Endpoint target type");
		break;
	}
	return 0;

}

static int p9_cfg_links_set_link_active_attr(const uint64_t* i_target,
	       				     const bool i_enable)
{
	if (*i_target == TARGET_TYPE_XBUS || *i_target == TARGET_TYPE_OBUS)
	{	
		uint8_t l_active = (i_enable) ?
			ENUM_ATTR_PROC_FABRIC_LINK_ACTIVE_TRUE:
			ENUM_ATTR_PROC_FABRIC_LINK_ACTIVE_FALSE;
	}
	else 
	{
		printk (BIOS_ERR, "Invalid Target Type");
		return -1;
	}

	return 0;
}

static int p9_config_links_map_endp(const uint64_t* i_target, 
				    const p9_fbc_link_ctl_t* i_link_ctl_arr, 
				    const uint8_t i_link_ctl_arr_size, 
				    uint8_t* o_link_id)
{
	uint8_t l_loc_unit_id;
	bool l_found = false;

	l_loc_unit_id = *i_target;

	for (uint8_t l_link_id = 0; (l_link_id < i_link_ctl_arr_size) && !l_found; l_link_id++)
	{
		if ((((i_link_ctl_arr + l_link_id)->endp_type == ELECTRICAL )  || ((i_link_ctl_arr + l_link_id)->endp_type == OPTICAL )) && 
		   ((i_link_ctl_arr + l_link_id)->endp_unit_id == l_loc_unit_id))
		{
			o_link_id = l_link_id;
			l_found = true;
		}
	}
	return 0;
}

static int p9_config_links_query_endp(const uint64_t* i_loc_target,
				      const uint8_t i_loc_fbc_chip_id,
				      const uint8_t i_loc_fbc_group_id,
				      const p9_fbc_link_ctl_t i_link_ctl_arr*,
				      const uint8_t i_link_ctl_arr_size,
				      bool i_rem_fbc_id_is_chip,
				      uint8_t o_loc_link_en*,
				      uint8_t o_rem_link_id*,
				      uint8_t o_rem_fbc_id*)
{

	uint8_t l_loc_link_id = 0;
	// l_rem_target;
	p9_config_links_map_endp(i_loc_target,
				 i_link_ctl_arr,
				 i_link_ctl_arr_size,
				 l_loc_link_id);

	p9_config_links_query_link_en(i_loc_target, o_lock_link_en + l_lock_link_id);

	if (o_loc_link_en[l_loc_link_id])
	{
		//
		//
		if(//)
		{

			o_loc_link_en[l_loc_link_id] = 0;
		}
		else
		{
			p9_fbc_eff_config_links_query_link_en(l_rem_target,
							      o_loc_link_en + l_loc_link_id);
		}
	}

	if (o_loc_link_en[l_loc_link_id])
	{
		uint8_t l_rem_fbc_group_id;
		uint8_t l_rem_fbc_chip_id;

		p9_fbc_eff_config_links_map_endp(l_rem_target,
						 i_link_ctl_arr,
						 i_link_ctl_arr_size,
						 o_rem_link_id + l_loc_link_id);

		if (i_rem_fbc_id_is_chip)
		{
			//
			o_rem_fbc_id[l_loc_link_id] = l_rem_fbc_chip_id;
		}
		else
		{
			o_rem_fbc_id[l_loc_link_id] = l_rem_fbc_group_id;
		}
	}
	
	p9_fbc_eff_config_links_set_link_active_attr(i_loc_target,
						     o_loc_link_en + l_loc_link_id);
	return 0;
}


static int p9_fbc_eff_config_links(const uint64_t* i_target,
				   p9_build_smp_operation i_op,
				   bool i_process_electrical,
				   bool i_process_optical)

{

	uint8_t l_loc_fbc_chip_id;
	uint8_t l_loc_fbc_group_id;
	
	uint8_t l_x_en[P9_FBC_UTILS_MAX_X_LINKS] = { 0 };
	uint8_t l_x_num = 0;
	uint8_t l_a_en[P9_FBC_UTILS_MAX_A_LINKS] = { 0 };
	uint8_t l_a_num = 0;

	uint8_t l_x_rem_link_id[P9_FBC_UTILS_MAX_X_LINKS] = { 0 };
	uint8_t l_x_rem_fbc_chip_id[P9_FBC_UTILS_MAX_X_LINKS] = { 0 };
	uint8_t l_a_rem_link_id[P9_FBC_UTILS_MAX_A_LINKS] = { 0 };
	uint8_t l_a_rem_fbc_group_id[P9_FBC_UTILS_MAX_A_LINKS] = { 0 };
	
	if (i_op == SMP_ACTIVATE_PHASE2)
	{
	//
	//
	}

	p9_fbc_utils_get_chip_id_attr(i_target, l_loc_fbc_chip_id);
	p9_fbc_utils_get_group_id_attr(i_target, l_loc_fbc_group_id);
	//

	if (i_process_electrical)
	{
		
		//
		for( ; ; ) //
		{
			p9_fbc_eff_config_links_query_endp(*l_iter,
							   l_loc_fbc_chip_id,
							   l_loc_fbc_group_id,
							   P9_FBC_XBUS_LINK_CTL_ARR,
							   P9_FBC_UTILS_MAX_X_LINKS,
							   (l_pump_mode == ENUM_ATTR_PROC_FABRIC_PUMP_MODE_CHIP_IS_NODE),
							   l_x_en,
							   l_x_rem_link_id,
							   l_x_rem_fbc_chip_id);
		
		}


	}

	if (i_process_optical)
	{
		//
		for( ; ; )
		{
			if (l_smp_optics_mode == ENUM_ATTR_PROC_FABRIC_SMP_OPTICS_MODE_OPTICS_IS_X_BUS)
			{
				p9_fbc_eff_config_links_query_endp(*l_iter,
								   l_loc_fbc_chip_id,
								   l_loc_fbc_group_id,
								   P9_FBC_XBUS_LINK_CTL_ARR,
								   P9_FBC_UTILS_MAX_X_LINKS,
								   (l_pump_mode == ENUM_ATTR_PROC_FABRIC_PUMP_MODE_CHIP_IS_NODE),
								   l_x_en,
								   l_x_rem_link_id,
								   l_x_rem_fbc_chip_id);
			}
			else
			{
				p9_fbc_eff_config_links_query_endp(*l_iter,
								   l_loc_fbc_chip_id,
								   l_loc_fbc_group_id,
								   P9_FBC_ABUS_LINK_CTL_ARR,
								   P9_FBC_UTILS_MAX_A_LINKS,
								   false,
								   l_a_en,
								   l_a_rem_link_id,
								   l_a_rem_fbc_group_id);
			}
		}

	}

	l_x_num = 0;
	l_a_num = 0;

	for (uint8_t l_link_id = 0; l_link_id < P9_FBC_UTILS_MAX_X_LINKS; l_link_id++)
	{
		if (l_x_en[l_link_id])
		{
			l_x_num++;
		}
	
	}

	for (uint8_t l_link_id = 0; l_link_id < P9_FBC_UTILS_MAX_A_LINKS; l_link_id++)
	{
		if (l_a_en[l_link_id])
		{
			l_a_num++;
		}

	}

	if (i_op == SMP_ACTIVATE_PHASE1)
	{
		uint32_t l_x_agg_link_delay[P9_FBC_UTILS_MAX_X_LINKS];
		uint32_t l_a_agg_link_delay[P9_FBC_UTILS_MAX_A_LINKS];

		for (uint8_t i = 0; i < P9_FBC_UTILS_MAX_X_LINKS; i++)
		{
			l_x_agg_link_delay[i] = 0xFFFFFFFF;
		}
		for (uint8_t i = 0; i < P9_FBC_UTILS_MAX_A_LINKS; i++)
		{
			l_a_agg_link_delay[i] = 0xFFFFFFFF;
		}

		uint8_t l_x_addr_dis[P9_FBC_UTILS_MAX_X_LINKS] = { 0 };
		uint8_t l_x_aggregate = 0;
		uint8_t l_a_addr_dis[P9_FBC_UTILS_MAX_A_LINKS] = { 0 };
		uint8_t l_a_aggregate = 0;


}	
