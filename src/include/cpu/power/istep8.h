/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SOC_ISTEP8_H
#define SOC_ISTEP8_H

enum FABRIC_ASYNC_MODE
{
	FABRIC_ASYNC_PERFORMANCE_MODE = 0x0,
	FABRIC_ASYNC_SAFE_MODE = 0x1,
};

enum FABRIC_CORE_FLOOR_RATIO
{
	FABRIC_CORE_FLOOR_RATIO_RATIO_8_8 = 0x0,
	FABRIC_CORE_FLOOR_RATIO_RATIO_7_8 = 0x1,
	FABRIC_CORE_FLOOR_RATIO_RATIO_6_8 = 0x2,
	FABRIC_CORE_FLOOR_RATIO_RATIO_5_8 = 0x3,
	FABRIC_CORE_FLOOR_RATIO_RATIO_4_8 = 0x4,
	FABRIC_CORE_FLOOR_RATIO_RATIO_2_8 = 0x5,
};

enum FABRIC_CORE_CEILING_RATIO
{
    FABRIC_CORE_CEILING_RATIO_RATIO_8_8 = 0x0,
    FABRIC_CORE_CEILING_RATIO_RATIO_7_8 = 0x1,
    FABRIC_CORE_CEILING_RATIO_RATIO_6_8 = 0x2,
    FABRIC_CORE_CEILING_RATIO_RATIO_5_8 = 0x3,
    FABRIC_CORE_CEILING_RATIO_RATIO_4_8 = 0x4,
    FABRIC_CORE_CEILING_RATIO_RATIO_2_8 = 0x5,
};

#define EPSILON_MIN_VALUE 0x1;
#define EPSILON_MAX_VALUE 0xFFFFFFFF;

#define NUM_EPSILON_READ_TIERS 3;
#define NUM_EPSILON_WRITE_TIERS 2;

#define CHIP_IS_NODE	0x01
#define CHIP_IS_GROUP	0x02

/* From src/import/chips/p9/procedures/xml/attribute_info/nest_attributes.xml */
#define EPS_TYPE_LE	0x01
#define EPS_TYPE_HE	0x02
#define EPS_TYPE_HE_F8	0x03

/*--------------ISTEP_8_7-------------------------*/

#define ENUM_ATTR_LINK_TRAIN_BOTH 	0x0
#define ENUM_ATTR_LINK_TRAIN_EVEN_ONLY	0x1
#define ENUM_ATTR_LINK_TRAIN_ODD_ONLY	0x2
/*#define ENUM_ATTR_LINK_TRAIN_NONE	0x3*/

#define ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_FALSE	0x0
#define ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_TRUE		0x1
#define ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_EVEN_ONLY 	0x2
#define ENUM_ATTR_PROC_FABRIC_X_ATTACHED_CHIP_CNFG_ODD_ONLY	0x3

#define ENUM_ATTR_PROC_FABRIC_LINK_ACTIVE_FALSE	0x0
#define ENUM_ATTR_PROC_FABRIC_LINK_ACTIVE_TRUE	0x1

#define ENUM_ATTR_OPTICS_CONFIG_MODE_SMP	0x0


#define P9_FBC_UTILS_MAX_X_LINKS	7
#define P9_FBC_UTILS_MAX_A_LINKS	4

/*-src/import/chips/p9/procedures/hwp/nest/p9_fbc_smp_utils.H*/
// link types
enum p9_fbc_link_t
{
	ELECTRICAL = TARGET_TYPE_XBUS
	OPTICAL	= TARGET_TYPE_OBUS
};

// XBUS/ABUS link control structure
struct p9_fbc_link_ctl_t
{
	// associated endpoint target type & unit target number
	p9_fbc_link_t	endp_type;
	uint8_t		endp_unit_id;	
	// iovalid SCOM addresses/control field
	uint64_t	iovalid_or_addr;
	uint64_t	iovalid_clear_addr;
	uint8_t		iovalid_field_start_bit;
	// EXTFIR/RAS control field
	uint8_t		ras_fir_field_bit;
	// DL layer SCOM addresses
	uint64_t	dl_fir_addr;
	uint64_t	dl_control_addr;
	uint64_t	dl_status_addr;
	// TL layer SCOM addresses
	uint64_t	tl_fir_addr;
	uint8_t		tl_fir_trained_field_start_bit;
	uint64_t	tl_link_delay_addr;
	uint32_t	tl_link_delay_hi_start_bit;
	uint32_t	tl_link_delay_lo_start_bit;
};

const p9_fbc_link_ctl_t P9_FBC_XBUS_LINK_CTL_ARR[P9_FBC_UTILS_MAX_X_LINKS] =
{
    {
        ELECTRICAL,
        0,
        PERV_XB_CPLT_CONF1_OR,
        PERV_XB_CPLT_CONF1_CLEAR,
        PERV_1_CPLT_CONF1_IOVALID_4D,
        PU_PB_CENT_SM1_EXTFIR_MASK_REG_PB_X0_FIR_ERR,
        XBUS_LL0_LL0_LL0_IOEL_FIR_REG,
        XBUS_0_LL0_IOEL_CONTROL,
        XBUS_LL0_IOEL_DLL_STATUS,
        PU_PB_IOE_FIR_REG,
        PU_PB_IOE_FIR_REG_FMR00_TRAINED,
        PU_PB_ELINK_DLY_0123_REG,
        4,
        20
    },
    {
        ELECTRICAL,
        1,
        PERV_XB_CPLT_CONF1_OR,
        PERV_XB_CPLT_CONF1_CLEAR,
        PERV_1_CPLT_CONF1_IOVALID_6D,
        PU_PB_CENT_SM1_EXTFIR_MASK_REG_PB_X1_FIR_ERR,
        XBUS_1_LL1_LL1_LL1_IOEL_FIR_REG,
        XBUS_1_LL1_IOEL_CONTROL,
        XBUS_1_LL1_IOEL_DLL_STATUS,
        PU_PB_IOE_FIR_REG,
        PU_PB_IOE_FIR_REG_FMR02_TRAINED,
        PU_PB_ELINK_DLY_0123_REG,
        36,
        52
    },
    {
        ELECTRICAL,
        2,
        PERV_XB_CPLT_CONF1_OR,
        PERV_XB_CPLT_CONF1_CLEAR,
        PERV_1_CPLT_CONF1_IOVALID_8D,
        PU_PB_CENT_SM1_EXTFIR_MASK_REG_PB_X2_FIR_ERR,
        XBUS_2_LL2_LL2_LL2_IOEL_FIR_REG,
        XBUS_2_LL2_IOEL_CONTROL,
        XBUS_2_LL2_IOEL_DLL_STATUS,
        PU_PB_IOE_FIR_REG,
        PU_PB_IOE_FIR_REG_FMR04_TRAINED,
        PU_PB_ELINK_DLY_45_REG,
        4,
        20
    },
    {
        OPTICAL,
        0,
        PERV_OB0_CPLT_CONF1_OR,
        PERV_OB0_CPLT_CONF1_CLEAR,
        PERV_1_CPLT_CONF1_IOVALID_4D,
        PU_PB_CENT_SM1_EXTFIR_MASK_REG_PB_X3_FIR_ERR,
        OBUS_LL0_LL0_LL0_PB_IOOL_FIR_REG,
        OBUS_0_LL0_IOOL_CONTROL,
        OBUS_0_LL0_IOOL_DLL_STATUS,
        PU_IOE_PB_IOO_FIR_REG,
        PU_IOE_PB_IOO_FIR_REG_FMR00_TRAINED,
        PU_IOE_PB_OLINK_DLY_0123_REG,
        4,
        20
    },
    {
        OPTICAL,
        1,
        PERV_OB1_CPLT_CONF1_OR,
        PERV_OB1_CPLT_CONF1_CLEAR,
        PERV_1_CPLT_CONF1_IOVALID_4D,
        PU_PB_CENT_SM1_EXTFIR_MASK_REG_PB_X4_FIR_ERR,
        OBUS_1_LL1_LL1_LL1_PB_IOOL_FIR_REG,
        OBUS_1_LL1_IOOL_CONTROL,
        OBUS_1_LL1_IOOL_DLL_STATUS,
        PU_IOE_PB_IOO_FIR_REG,
        PU_IOE_PB_IOO_FIR_REG_FMR02_TRAINED,
        PU_IOE_PB_OLINK_DLY_0123_REG,
        36,
        52
    },
    {
        OPTICAL,
        2,
        PERV_OB2_CPLT_CONF1_OR,
        PERV_OB2_CPLT_CONF1_CLEAR,
        PERV_1_CPLT_CONF1_IOVALID_4D,
        PU_PB_CENT_SM1_EXTFIR_MASK_REG_PB_X5_FIR_ERR,
        OBUS_2_LL2_LL2_LL2_PB_IOOL_FIR_REG,
        OBUS_2_LL2_IOOL_CONTROL,
        OBUS_2_LL2_IOOL_DLL_STATUS,
        PU_IOE_PB_IOO_FIR_REG,
        PU_IOE_PB_IOO_FIR_REG_FMR04_TRAINED,
        PU_IOE_PB_OLINK_DLY_4567_REG,
        4,
        20
    },
    {
        OPTICAL,
        3,
        PERV_OB3_CPLT_CONF1_OR,
        PERV_OB3_CPLT_CONF1_CLEAR,
        PERV_1_CPLT_CONF1_IOVALID_4D,
        PU_PB_CENT_SM1_EXTFIR_MASK_REG_PB_X6_FIR_ERR,
        OBUS_3_LL3_LL3_LL3_PB_IOOL_FIR_REG,
        OBUS_3_LL3_IOOL_CONTROL,
        OBUS_3_LL3_IOOL_DLL_STATUS,
        PU_IOE_PB_IOO_FIR_REG,
        PU_IOE_PB_IOO_FIR_REG_FMR06_TRAINED,
        PU_IOE_PB_OLINK_DLY_4567_REG,
        36,
        52
    }
};

const p9_fbc_link_ctl_t P9_FBC_ABUS_LINK_CTL_ARR[P9_FBC_UTILS_MAX_A_LINKS] =
{
    {
        OPTICAL,
        0,
        PERV_OB0_CPLT_CONF1_OR,
        PERV_OB0_CPLT_CONF1_CLEAR,
        PERV_1_CPLT_CONF1_IOVALID_4D,
        PU_PB_CENT_SM1_EXTFIR_MASK_REG_PB_X3_FIR_ERR,
        OBUS_LL0_LL0_LL0_PB_IOOL_FIR_REG,
        OBUS_0_LL0_IOOL_CONTROL,
        OBUS_0_LL0_IOOL_DLL_STATUS,
        PU_IOE_PB_IOO_FIR_REG,
        PU_IOE_PB_IOO_FIR_REG_FMR00_TRAINED,
        PU_IOE_PB_OLINK_DLY_0123_REG,
        4,
        20
    },
    {
        OPTICAL,
        1,
        PERV_OB1_CPLT_CONF1_OR,
        PERV_OB1_CPLT_CONF1_CLEAR,
        PERV_1_CPLT_CONF1_IOVALID_4D,
        PU_PB_CENT_SM1_EXTFIR_MASK_REG_PB_X4_FIR_ERR,
        OBUS_1_LL1_LL1_LL1_PB_IOOL_FIR_REG,
        OBUS_1_LL1_IOOL_CONTROL,
        OBUS_1_LL1_IOOL_DLL_STATUS,
        PU_IOE_PB_IOO_FIR_REG,
        PU_IOE_PB_IOO_FIR_REG_FMR02_TRAINED,
        PU_IOE_PB_OLINK_DLY_0123_REG,
        36,
        52
    },
    {
        OPTICAL,
        2,
        PERV_OB2_CPLT_CONF1_OR,
        PERV_OB2_CPLT_CONF1_CLEAR,
        PERV_1_CPLT_CONF1_IOVALID_4D,
        PU_PB_CENT_SM1_EXTFIR_MASK_REG_PB_X5_FIR_ERR,
        OBUS_2_LL2_LL2_LL2_PB_IOOL_FIR_REG,
        OBUS_2_LL2_IOOL_CONTROL,
        OBUS_2_LL2_IOOL_DLL_STATUS,
        PU_IOE_PB_IOO_FIR_REG,
        PU_IOE_PB_IOO_FIR_REG_FMR04_TRAINED,
        PU_IOE_PB_OLINK_DLY_4567_REG,
        4,
        20
    },
    {
        OPTICAL,
        3,
        PERV_OB3_CPLT_CONF1_OR,
        PERV_OB3_CPLT_CONF1_CLEAR,
        PERV_1_CPLT_CONF1_IOVALID_4D,
        PU_PB_CENT_SM1_EXTFIR_MASK_REG_PB_X6_FIR_ERR,
        OBUS_3_LL3_LL3_LL3_PB_IOOL_FIR_REG,
        OBUS_3_LL3_IOOL_CONTROL,
        OBUS_3_LL3_IOOL_DLL_STATUS,
        PU_IOE_PB_IOO_FIR_REG,
        PU_IOE_PB_IOO_FIR_REG_FMR06_TRAINED,
        PU_IOE_PB_OLINK_DLY_4567_REG,
        36,
        52
    }
};

//P9_FBC_XBUS_LINK_CTL_ARR ok
//P9_FBC_ABUS_LINK_CTL_ARR ok
//SMP_ACTIVATE_PHASE1
//SMP_ACTIVATE_PHASE2
// p9_fbc_utils_get_chip_id_attr
// p9_fbc_utils_get_group_id_attr

#endif
