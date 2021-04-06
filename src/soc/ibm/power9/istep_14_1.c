/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/istep_14_1.h>
#include <cpu/power/scom.h>

errlHndl_t StateMachine::doMaintCommand(WorkFlowProperties & i_wfp)
{
    errlHndl_t err = nullptr;
    uint64_t workItem;

    TargetHandle_t target;

    // starting a maint cmd ...  register a timeout monitor
    uint64_t maintCmdTO = getTimeoutValue();

    uint64_t monitorId = CommandMonitor::INVALID_MONITOR_ID;
    i_wfp.timeoutCnt = 0; // reset for new work item
    workItem = *i_wfp.workItem;

    target = getTarget(i_wfp);

    TYPE trgtType = target->getAttr<ATTR_TYPE>();
    // new command...use the full range
    if (TYPE_MBA == trgtType)
    {
        uint32_t stopCondition =
            mss_MaintCmd::STOP_END_OF_RANK                  |
            mss_MaintCmd::STOP_ON_MPE                       |
            mss_MaintCmd::STOP_ON_UE                        |
            mss_MaintCmd::STOP_ON_END_ADDRESS               |
            mss_MaintCmd::ENABLE_CMD_COMPLETE_ATTENTION;

        if(TARGETING::MNFG_FLAG_IPL_MEMORY_CE_CHECKING & iv_globals.mfgPolicy)
        {
            // For MNFG mode, check CE also
            stopCondition |= mss_MaintCmd::STOP_ON_HARD_NCE_ETE;
        }

        fapi2::buffer<uint64_t> startAddr, endAddr;
        mss_MaintCmd * cmd = nullptr;
        cmd = static_cast<mss_MaintCmd *>(i_wfp.data);
        fapi2::Target<fapi2::TARGET_TYPE_MBA> fapiMba(target);

        // We will always do ce setup though CE calculation
        // is only done during MNFG. This will give use better ffdc.
        err = ceErrorSetup<TYPE_MBA>(target);

        FAPI_INVOKE_HWP(err, mss_get_address_range, fapiMba, endAddr);
        if(workItem == START_SCRUB)
        {
            cmd = new mss_SuperFastRead(
                fapiMba,
                0,
                endAddr,
                stopCondition,
                false);
        } else // PATTERNS
        {
            cmd = new mss_SuperFastInit(
                fapiMba,
                0,
                endAddr,
                static_cast<mss_MaintCmd::PatternIndex>(workItem),
                stopCondition,
                false);
        }
        i_wfp.data = cmd;
        // Command and address configured.
        // Invoke the command.
        FAPI_INVOKE_HWP(err, cmd->setupAndExecuteCmd);
    }
    else if(TYPE_MCBIST == trgtType)
    {
        fapi2::Target<fapi2::TARGET_TYPE_MCBIST> fapiMcbist(target);
        mss::mcbist::stop_conditions<mss::mc_type::NIMBUS> stopCond;
        switch(workItem)
        {
            case START_SCRUB:
                //set stop conditions
                stopCond.set_pause_on_mpe(mss::ON);
                stopCond.set_pause_on_ue(mss::ON);
                stopCond.set_pause_on_aue(mss::ON);
                stopCond.set_nce_inter_symbol_count_enable(mss::ON);
                stopCond.set_nce_soft_symbol_count_enable(mss::ON);
                stopCond.set_nce_hard_symbol_count_enable(mss::ON);
                if(TARGETING::MNFG_FLAG_IPL_MEMORY_CE_CHECKING & iv_globals.mfgPolicy)
                {
                    stopCond.set_pause_on_nce_hard(mss::ON);
                }
                FAPI_INVOKE_HWP(err, nim_sf_read, fapiMcbist, stopCond);
                break;
            case START_PATTERN_0:
                FAPI_INVOKE_HWP(err, sf_init, fapiMcbist, workItem);
                break;
        }
    }
}

fapi2::ReturnCode mss_get_address_range(
    const fapi2::Target<fapi2::TARGET_TYPE_MBA>& i_target,
    fapi2::buffer<uint64_t>& o_end_addr)
{
    constexpr uint8_t NUM_CONFIG_TYPES = 9;
    constexpr uint8_t NUM_CONFIG_SUBTYPES = 4;
    constexpr uint8_t NUM_SLOT_CONFIGS = 2;
    static const uint8_t memConfigType[NUM_CONFIG_TYPES][NUM_CONFIG_SUBTYPES][NUM_SLOT_CONFIGS] =
    {

        // Refer to Centaur Workbook: 5.2 Master and Slave Rank Usage
        //
        //       SUBTYPE_A                    SUBTYPE_B                    SUBTYPE_C                    SUBTYPE_D
        //
        //SLOT_0_ONLY   SLOT_0_AND_1   SLOT_0_ONLY   SLOT_0_AND_1   SLOT_0_ONLY   SLOT_0_AND_1   SLOT_0_ONLY   SLOT_0_AND_1
        //
        //master slave  master slave   master slave  master slave   master slave  master slave   master slave  master slave
        //
        {{0xff,         0xff},         {0xff,        0xff},         {0xff,         0xff},       {0xff,         0xff}},  // TYPE_0
        {{0x00,         0x40},         {0x10,        0x50},         {0x30,         0x70},       {0xff,         0xff}},  // TYPE_1
        {{0x01,         0x41},         {0x03,        0x43},         {0x07,         0x47},       {0xff,         0xff}},  // TYPE_2
        {{0x11,         0x51},         {0x13,        0x53},         {0x17,         0x57},       {0xff,         0xff}},  // TYPE_3
        {{0x31,         0x71},         {0x33,        0x73},         {0x37,         0x77},       {0xff,         0xff}},  // TYPE_4
        {{0x00,         0x40},         {0x10,        0x50},         {0x30,         0x70},       {0xff,         0xff}},  // TYPE_5
        {{0x01,         0x41},         {0x03,        0x43},         {0x07,         0x47},       {0xff,         0xff}},  // TYPE_6
        {{0x11,         0x51},         {0x13,        0x53},         {0x17,         0x57},       {0xff,         0xff}},  // TYPE_7
        {{0x31,         0x71},         {0x33,        0x73},         {0x37,         0x77},       {0xff,         0xff}}   // TYPE_8
    };

    uint64_t l_data;
    mss_memconfig::MemOrg l_row;
    mss_memconfig::MemOrg l_col;
    mss_memconfig::MemOrg l_bank;
    uint8_t l_dramWidth = 0;
    uint8_t l_mbaPosition = 0;
    uint8_t l_dram_gen = 0;

    const auto l_targetCentaur = i_target.getParent<fapi2::TARGET_TYPE_MEMBUF_CHIP>();
    FAPI_ATTR_GET(fapi2::ATTR_CHIP_UNIT_POS, i_target,  l_mbaPosition);
    FAPI_ATTR_GET(fapi2::ATTR_CEN_EFF_DRAM_WIDTH, i_target,  l_dramWidth);
    FAPI_ATTR_GET(fapi2::ATTR_CEN_EFF_DRAM_GEN, i_target,  l_dram_gen);
    fapi2::getScom(l_targetCentaur, mss_mbaxcr[l_mbaPosition], l_data);

    uint32_t l_dramSize = (uint32_t)((l_data & 0x300000000000000) >> 24);

    if((l_dramWidth == mss_memconfig::X8) &&
       (l_dramSize == mss_memconfig::GBIT_2) &&
       (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR3))
    {
        l_row =   mss_memconfig::ROW_15;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_3;
    } else if((l_dramWidth == mss_memconfig::X8) &&
              (l_dramSize == mss_memconfig::GBIT_2) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   mss_memconfig::ROW_14;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    } else if((l_dramWidth == mss_memconfig::X4) &&
              (l_dramSize == mss_memconfig::GBIT_2) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR3))
    {
        l_row =   mss_memconfig::ROW_15;
        l_col =   mss_memconfig::COL_11;
        l_bank =  mss_memconfig::BANK_3;
    } else if((l_dramWidth == mss_memconfig::X4) &&
              (l_dramSize == mss_memconfig::GBIT_2) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   mss_memconfig::ROW_15;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    } else if((l_dramWidth == mss_memconfig::X8) &&
              (l_dramSize == mss_memconfig::GBIT_4) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR3))
    {
        l_row =   mss_memconfig::ROW_16;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_3;
    } else if((l_dramWidth == mss_memconfig::X8) &&
              (l_dramSize == mss_memconfig::GBIT_4) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   mss_memconfig::ROW_15;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    } else if((l_dramWidth == mss_memconfig::X4) &&
              (l_dramSize == mss_memconfig::GBIT_4) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR3))
    {
        l_row =   mss_memconfig::ROW_16;
        l_col =   mss_memconfig::COL_11;
        l_bank =  mss_memconfig::BANK_3;
    } else if((l_dramWidth == mss_memconfig::X4) &&
              (l_dramSize == mss_memconfig::GBIT_4) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   mss_memconfig::ROW_16;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    } else if((l_dramWidth == mss_memconfig::X8) &&
              (l_dramSize == mss_memconfig::GBIT_8) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR3))
    {
        l_row =   mss_memconfig::ROW_16;
        l_col =   mss_memconfig::COL_11;
        l_bank =  mss_memconfig::BANK_3;
    } else if((l_dramWidth == mss_memconfig::X8) &&
              (l_dramSize == mss_memconfig::GBIT_8) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   mss_memconfig::ROW_16;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    } else if((l_dramWidth == mss_memconfig::X4) &&
              (l_dramSize == mss_memconfig::GBIT_8) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR3))
    {
        l_row =   mss_memconfig::ROW_16;
        l_col =   mss_memconfig::COL_12;
        l_bank =  mss_memconfig::BANK_3;
    } else if((l_dramWidth == mss_memconfig::X4) &&
              (l_dramSize == mss_memconfig::GBIT_8) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   mss_memconfig::ROW_17;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    } else if((l_dramWidth == mss_memconfig::X4) &&
              (l_dramSize == mss_memconfig::GBIT_16) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   ss_memconfig::ROW_17;
        l_col =   ss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    } else if((l_dramWidth == mss_memconfig::X8) &&
              (l_dramSize == mss_memconfig::GBIT_16) &&
              (l_dram_gen == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4))
    {
        l_row =   mss_memconfig::ROW_17;
        l_col =   mss_memconfig::COL_10;
        l_bank =  mss_memconfig::BANK_4;
    }

    uint8_t l_configType = (l_data & 0xF0) >> 4;
    uint8_t l_configSubType = (l_data & 0xC) >> 2;
    uint8_t l_slotConfig = l_data & 0x1;

    uint8_t l_end_master_rank = (memConfigType[l_configType][l_configSubType][l_slotConfig] & 0xf0) >> 4;
    uint8_t l_end_slave_rank = memConfigType[l_configType][l_configSubType][l_slotConfig] & 0x0f;

    o_end_addr =
        (l_end_master_rank & 0xF) << 60
      | (l_end_slave_rank & 0x7) << 57
      | ((uint32_t)l_bank & 0xF) << 53
      | ((uint32_t)l_row & 0x1FFFF) << 36
      | ((uint32_t)l_col & 0xFFF) << 24;

    if(l_dramWidth == mss_memconfig::X4
    && l_dramSize  == mss_memconfig::GBIT_16
    && l_dram_gen  == fapi2::ENUM_ATTR_CEN_EFF_DRAM_GEN_DDR4)
    {
        o_end_addr.writeBit<CEN_MBA_MBMEAQ_CMD_ROW17>(l_row18);
    }
}

template<  mss::mc_type MC = DEFAULT_MC_TYPE, fapi2::TargetType T >
fapi2::ReturnCode sf_init( const fapi2::Target<T>& i_target,
                           const uint64_t i_pattern = PATTERN_0 )
{
    using ET = mss::mcbistMCTraits<MC>;
    constraints<MC> l_const(i_pattern);
    sf_init_operation<MC> l_init_op(i_target, l_const, l_rc);
    return l_init_op.execute();
}

// Helper function to access the state of manufacturing policy flags.
bool isMnfgFlagSet(uint32_t i_flag) { return false; }

bool areDramRepairsDisabled() { return false; }
bool mnfgSpareDramDeploy() { return false; }

template<TARGETING::TYPE T>
uint32_t restoreDramRepairs( TargetHandle_t i_trgt )
{
    bool calloutMade = false;

    // Will need the chip and system objects initialized for several parts
    // of this function and sub-functions.
    if(false == g_initialized || nullptr == systemPtr)
    {
        noLock_initialize();
    }

    std::vector<MemRank> ranks;
    getMasterRanks<T>( i_trgt, ranks );

    uint8_t rankMask = 0, dimmMask = 0;
    if(SUCCESS != mssRestoreDramRepairs<T>(i_trgt, rankMask, dimmMask))
    {
        return FAIL;
    }

    // Callout DIMMs with too many bad bits and not enough repairs available
    if(RDR::processBadDimms<T>(i_trgt, dimmMask))
        calloutMade = true;

    // Check repaired ranks for RAS policy violations.
    if(RDR::processRepairedRanks<T>(i_trgt, rankMask))
        calloutMade = true;

    return calloutMade ? FAIL : SUCCESS;
}

bool StateMachine::executeWorkItem(WorkFlowProperties * i_wfp)
{
    switch(*i_wfp->workItem)
    {
        case RESTORE_DRAM_REPAIRS:
            TargetHandle_t target = getTarget(*i_wfp);
            // Get the connected MCAs.
            TargetHandleList mcaList;
            getChildAffinityTargets(mcaList, target, CLASS_UNIT, TYPE_MCA);
            for (auto & mca : mcaList)
            {
                PRDF::restoreDramRepairs<TYPE_MCA>(mca);
            }
            break;
        case START_PATTERN_0:
        case START_SCRUB:
            doMaintCommand(*i_wfp);
            break;
    }
    ++i_wfp->workItem;
    scheduleWorkItem(*i_wfp);
}

void memDiag(void) {

    // memory diagnostics ipl step entry point
    Globals globals;
    TargetHandle_t top = nullptr;
    targetService().getTopLevelTarget(top);

    // get the workflow for each target mba passed in.
    // associate each workflow with the target handle.
    WorkFlowAssocMap list;
    TargetHandleList::const_iterator tit;
    DiagMode mode;
    for(unsigned int targetIndex = 0; targetIndex < memdiagTargetsSize; ++targetIndex) {
        // create a list with patterns
        // for ONE_PATTERN the list is as follows
        // list[0] = RESTORE_DRAM_REPAIRS
        // list[1] = START_PATTERN_0
        // list[2] = START_SCRUB
        getWorkFlow(ONE_PATTERN, list[memdiagTargets[targetIndex]]);
    }

    globals.mfgPolicy = 0;
    globals.simicsRunning = false;
    // set global data
    Singleton<StateMachine>::instance().setGlobals(globals);
    // calling errlHndl_t StateMachine::run(const WorkFlowAssocMap & i_list)
    Singleton<StateMachine>::instance().run(list);

    // If this step completes without the need for a reconfig due to an RCD
    // parity error, clear all RCD parity error counters.
    ATTR_RECONFIGURE_LOOP_type attr = top->getAttr<ATTR_RECONFIGURE_LOOP>();
    if (0 == (attr & RECONFIGURE_LOOP_RCD_PARITY_ERROR))
    {
        TargetHandleList trgtList; getAllChiplets(trgtList, TYPE_MCA);
        for (auto & trgt : trgtList)
        {
            if (0 != trgt->getAttr<ATTR_RCD_PARITY_RECONFIG_LOOP_COUNT>())
                trgt->setAttr<ATTR_RCD_PARITY_RECONFIG_LOOP_COUNT>(0);
        }
    }
}

void after_memdiags(uint64_t i_target)
{
    uint64_t dsm0_buffer;
    uint64_t rd_tag_delay = 0;
    uint64_t wr_done_delay = 0;
    uint64_t rcd_protect_time = 0;

    scom_and_for_chiplet(
        i_target, MCBIST_MCBISTFIRACT1,
        ~MCBIST_MCBISTFIRQ_MCBIST_BRODCAST_OUT_OF_SYNC);

    for(unsigned int port = 0; port < 4; ++port)
    {
        scom_or(ECC_REG | port << PORT_OFFSET, RECR_ENABLE_UE_NOISE_WINDOW);

        dsm0_buffer = scom_read(DSM0Q_REG | (port << PORT_OFFSET));
        wr_done_delay = (dsm0_buffer & 0xFC00000000) >> 34;
        rd_tag_delay = (dsm0_buffer & 0xFC00000) >> 22;
        rcd_protect_time = min(wr_done_delay, rd_tag_delay);
        scom_and_or(MBA_FARB0Q | (port << PORT_OFFSET), ~0xFC00, (rcd_protect_time & 0x3F) << 10);

        scom_and(
            FIR_ACTION0 | (port << PORT_OFFSET),
            ~(MCA_FIR_MAINLINE_AUE | MCA_FIR_MAINLINE_UE
           | MCA_FIR_MAINLINE_IAUE | MCA_FIR_MAINLINE_IUE
           | MCA_FIR_MAINTENANCE_IUE));

        scom_and_or(
            FIR_ACTION1 | (port << PORT_OFFSET),
            ~(MCA_FIR_MAINLINE_AUE | MCA_FIR_MAINLINE_IAUE),
              MCA_FIR_MAINLINE_UE  | MCA_FIR_MAINLINE_IUE
            | MCA_FIR_MAINTENANCE_IUE);

        scom_and(
            FIR_MASK | (port << PORT_OFFSET),
            ~(MCA_FIR_MAINLINE_AUE | MCA_FIR_MAINLINE_UE
           | MCA_FIR_MAINLINE_IAUE | MCA_FIR_MAINLINE_IUE
           | MCA_FIR_MAINTENANCE_IUE));

        scom_and(
            MBACALFIR_ACTION0 | (port << PORT_OFFSET),
            ~(MCA_MBACALFIRQ_PORT_FAIL));
        scom_or(
            MBACALFIR_ACTION1 | (port << PORT_OFFSET),
            MCA_MBACALFIRQ_PORT_FAIL);
        scom_and(
            MBACALFIR_MASK | (port << PORT_OFFSET),
            ~(MCA_MBACALFIRQ_PORT_FAIL));

        if(CHIP_EC == 0x20)
        {
            scom_and(FIR_ACTION0 | (port << PORT_OFFSET), ~MCA_FIR_MAINLINE_RCD);
            scom_and(FIR_ACTION1 | (port << PORT_OFFSET), ~(MCA_FIR_MAINLINE_UE | MCA_FIR_MAINLINE_RCD));
            scom_and(FIR_MASK | (port << PORT_OFFSET), ~MCA_FIR_MAINLINE_RCD);
            scom_and(MBACALFIR_ACTION1 | (port << PORT_OFFSET), ~(MCA_MBACALFIRQ_PORT_FAIL));
        }

        scom_and(MCA_ACTION1 | (port << PORT_OFFSET), ~0x48000000);
        scom_and(MBA_FARB0Q | (port << PORT_OFFSET), ~0x240);
    }
}

void istep_14_1(void)
{
    printk(BIOS_EMERG, "starting istep 14.1\n");
    report_istep(14, 1);
    memDiag();
    for(unsigned int targetIndex = 0; targetIndex < memdiagTargetsSize; ++targetIndex) {
        after_memdiags(memdiagTargets[targetIndex]);
        for(unsigned int port = 0; port < 4; ++port) {
            scom_or(RRQ0Q_REG | (port << PORT_OFFSET), MBA_RRQ0Q_CFG_RRQ_FIFO_MODE);
            scom_or(WRQ0Q_REG | (port << PORT_OFFSET), MBA_WRQ0Q_CFG_WRQ_FIFO_MODE);
        }
    }
    printk(BIOS_EMERG, "ending istep 14.1\n");
    return;
}
