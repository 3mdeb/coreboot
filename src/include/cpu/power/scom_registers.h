typedef enum
{
    PB_WEST_MODE_CFG =              0x501180A, // Power Bus PB West Mode Configuration Register
    PB_CENT_MODE_CFG =              0x5011C0A, // Power Bus PB CENT Mode Register
    PB_CENT_GP_COMMAND_RATE_DP0 =   0x5011C26, // Power Bus PB CENT GP command RATE DP0 Register
    PB_CENT_GP_COMMAND_RATE_DP1 =   0x5011C27, // Power Bus PB CENT GP command RATE DP1 Register
    PB_CENT_RGP_COMMAND_RATE_DP0 =  0x5011C28, // Power Bus PB CENT RGP command RATE DP0 Register
    PB_CENT_RGP_COMMAND_RATE_DP1 =  0x5011C29, // Power Bus PB CENT RGP command RATE DP1 Register
    PB_CENT_SP_COMMAND_RATE_DP0 =   0x5011C2A, // Power Bus PB CENT SP command RATE DP0 Register
    PB_CENT_SP_COMMAND_RATE_DP1 =   0x5011C2B, // Power Bus PB CENT SP command RATE DP1 Register
    PB_EAST_MODE_CFG =              0x501200A, // Power Bus PB East Mode Configuration Register
    PU_PB_IOE_FIR_ACTION0_REG =     0x5013406,
    PU_PB_IOE_FIR_ACTION1_REG =     0x5013407,
    PB_ELE_PB_FRAMER_PARSER_01_CFG =0x501340A, // Power bus Electrical Framer/Parser 01 configuration register
    PB_ELE_PB_FRAMER_PARSER_23_CFG =0x501340B, // Power Bus Electrical Framer/Parser 23 Configuration Register
    PB_ELE_PB_FRAMER_PARSER_45_CFG =0x501340C, // Power Bus Electrical Framer/Parser 45 Configuration Register
    PB_ELE_PB_DATA_BUFF_01_CFG =    0x5013410, // Power Bus Electrical Link Data Buffer 01 Configuration Register
    PB_ELE_PB_DATA_BUFF_23_CFG =    0x5013411, // Power Bus Electrical Link Data Buffer 23 Configuration Register
    PB_ELE_PB_DATA_BUFF_45_CFG =    0x5013412, // Power Bus Electrical Link Data Buffer 45 Configuration Register
    PB_ELE_MISC_CFG =               0x5013423, // Power Bus Electrical Miscellaneous Configuration Register
    PB_ELE_LINK_TRACE_CFG =         0x5013424, // Power Bus Electrical Link Trace Configuration Register
    XBUS_LL0_IOEL_FIR_MASK_REG =    0x6011803,
    XBUS_LL0_IOEL_FIR_ACTION0_REG = 0x6011806,
    XBUS_LL0_IOEL_FIR_ACTION1_REG = 0x6011807,
    PB_ELL_CFG_REG =                0x601180A, // ELL Configuration Register
    PB_ELL_REPLAY_TRESHOLD_REG =    0x6011818, // ELL Replay Threshold Register
    PB_ELL_SL_ECC_TRESHOLD_REG =    0x6011819, // ELL SL ECC Threshold Register
    XBUS_FIR_ACTION0_REG = 0x06010C06,
    XBUS_FIR_ACTION1_REG = 0x06010C07,
} scom_reg_t;
