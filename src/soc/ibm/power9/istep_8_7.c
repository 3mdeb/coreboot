/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <console/console.h>
#include <soc/istep8.h>

static int p9_query_config_links(const uint8_t *i_target, uint8_t *o_link_is_enabled)
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
_
