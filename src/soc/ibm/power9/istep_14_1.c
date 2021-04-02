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
    // target type is MBA
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

        FAPI_INVOKE_HWP(err, mss_get_address_range, fapiMba, MSS_ALL_RANKS, startAddr, endAddr);
        // new command...use the full range
        if(workItem == START_RANDOM_PATTERN)
        {
            cmd = new mss_SuperFastRandomInit(
                fapiMba,
                startAddr,
                endAddr,
                mss_MaintCmd::PATTERN_RANDOM,
                stopCondition,
                false);
        } else if(workItem == START_SCRUB)
        {
            cmd = new mss_SuperFastRead(
                fapiMba,
                startAddr,
                endAddr,
                stopCondition,
                false);
        } else // PATTERNS
        {
            cmd = new mss_SuperFastInit(
                fapiMba,
                startAddr,
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
    //target type is MCBIST
    else if(TYPE_MCBIST == trgtType)
    {
        fapi2::Target<fapi2::TARGET_TYPE_MCBIST> fapiMcbist(target);
        mss::mcbist::stop_conditions<mss::mc_type::NIMBUS> stopCond;
        switch(workItem)
        {
            case START_RANDOM_PATTERN:
                FAPI_INVOKE_HWP(err, sf_init, fapiMcbist, mss::mcbist::PATTERN_RANDOM);
                break;
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
            case START_PATTERN_1:
            case START_PATTERN_2:
            case START_PATTERN_3:
            case START_PATTERN_4:
            case START_PATTERN_5:
            case START_PATTERN_6:
            case START_PATTERN_7:
                FAPI_INVOKE_HWP(err, sf_init, fapiMcbist, workItem);
                break;
            default:
                break;
        }
    }
}

template<TARGETING::TYPE T>
uint32_t restoreDramRepairs( TargetHandle_t i_trgt )
{
    // will unlock when going out of scope
    PRDF_SYSTEM_SCOPELOCK;

    bool calloutMade = false;

    // Will need the chip and system objects initialized for several parts
    // of this function and sub-functions.
    if ( (false == g_initialized) || (nullptr == systemPtr) )
    {
        errlHndl_t errl = noLock_initialize();
        if ( nullptr != errl )
        {
            RDR::commitErrl<T>( errl, i_trgt );
            return FAIL;
        }
    }

    std::vector<MemRank> ranks;
    getMasterRanks<T>( i_trgt, ranks );

    bool spareDramDeploy = mnfgSpareDramDeploy();

    if ( spareDramDeploy )
    {
        // Deploy all spares for MNFG corner tests.
        RDR::deployDramSpares<T>( i_trgt, ranks );
    }

    if ( areDramRepairsDisabled() )
    {
        // DRAM Repairs are disabled in MNFG mode, so screen all DIMMs with
        // VPD information.
        if ( RDR::screenBadDqs<T>(i_trgt, ranks) )
            calloutMade = true;

        // No need to continue because there will not be anything to
        // restore.
        return FAIL;
    }

    if ( spareDramDeploy )
    {
        // This is an error. The MNFG spare DRAM deply bit is set, but DRAM
        // Repairs have not been disabled.

        RDR::commitSoftError<T>( PRDF_INVALID_CONFIG, i_trgt,
                                    PRDFSIG_RdrInvalidConfig, true );

        return FAIL;
    }

    uint8_t rankMask = 0, dimmMask = 0;
    if ( SUCCESS != mssRestoreDramRepairs<T>( i_trgt, rankMask,
                                                dimmMask) )
    {
        return FAIL;
    }

    // Callout DIMMs with too many bad bits and not enough repairs available
    if ( RDR::processBadDimms<T>(i_trgt, dimmMask) )
        calloutMade = true;

    // Check repaired ranks for RAS policy violations.
    if ( RDR::processRepairedRanks<T>(i_trgt, rankMask) )
        calloutMade = true;

    return calloutMade ? FAIL : SUCCESS;
}

bool StateMachine::executeWorkItem(WorkFlowProperties * i_wfp)
{
    uint64_t workItem = *i_wfp->workItem;

    switch(workItem)
    {
        case RESTORE_DRAM_REPAIRS:
        {
            TargetHandle_t target = getTarget(*i_wfp);
            // Get the connected MCAs.
            TargetHandleList mcaList;
            getChildAffinityTargets(mcaList, target, CLASS_UNIT, TYPE_MCA);
            for (auto & mca : mcaList)
            {
                PRDF::restoreDramRepairs<TYPE_MCA>(mca);
            }
            break;
        }
        case START_PATTERN_0:
        case START_PATTERN_1:
        case START_PATTERN_2:
        case START_PATTERN_3:
        case START_PATTERN_4:
        case START_PATTERN_5:
        case START_PATTERN_6:
        case START_PATTERN_7:
        case START_RANDOM_PATTERN:
        case START_SCRUB:
            doMaintCommand(*i_wfp);
            break;
        default:
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

    globals.mfgPolicy = 0;
    globals.simicsRunning = false;

    // get the workflow for each target mba passed in.
    // associate each workflow with the target handle.
    WorkFlowAssocMap list;
    TargetHandleList::const_iterator tit;
    DiagMode mode;
    for(unsigned int targetIndex = 0; targetIndex < memdiagTargetsSize; ++targetIndex) {
        getDiagnosticMode(globals, memdiagTargets[targetIndex], mode);
        // create a list with patterns
        // for ONE_PATTERN the list is as follows
        // list[0] = RESTORE_DRAM_REPAIRS
        // list[1] = START_PATTERN_0
        // list[2] = START_SCRUB
        getWorkFlow(mode, list[memdiagTargets[targetIndex]], globals);
    }

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

void broadcast_out_of_sync(uint64_t i_target)
{
    scom_and_for_chiplet(
        i_target, MCBIST_MCBISTFIRACT1,
        ~MCBIST_MCBISTFIRQ_MCBIST_BRODCAST_OUT_OF_SYNC);

    for(unsigned int port = 0; port < 4; ++port)
    {
        scom_or(ECC_REG | port << PORT_OFFSET, RECR_ENABLE_UE_NOISE_WINDOW);
    }
}

void after_memdiags(uint64_t i_target)
{
    uint64_t dsm0_buffer;
    uint64_t l_mnfg_buffer;
    uint64_t rd_tag_delay = 0;
    uint64_t wr_done_delay = 0;
    uint64_t rcd_protect_time = 0;

    // Broadcast mode workaround for UEs causing out of sync
    broadcast_out_of_sync(i_target);

    for(unsigned int port = 0; port < 4; ++port)
    {
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
