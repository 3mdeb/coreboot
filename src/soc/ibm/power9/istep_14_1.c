/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/istep_14_1.h>
#include <cpu/power/scom.h>

// errlHndl_t runStep(const TargetHandleList & i_targetList)
// {
//     // memory diagnostics ipl step entry point
//     Globals globals;
//     TargetHandle_t top = nullptr;
//     targetService().getTopLevelTarget(top);

//     if(top)
//     {
//         globals.mfgPolicy = top->getAttr<ATTR_MNFG_FLAGS>();
//         // by default 0
//         // see hostboot/src/usr/targeting/common/xmltohb/attribute_types.xml:7292
//         uint8_t maxMemPatterns = top->getAttr<ATTR_RUN_MAX_MEM_PATTERNS>();
//         // This registry / attr is the same as the
//         // exhaustive mnfg one
//         if(maxMemPatterns)
//         {
//             globals.mfgPolicy |= MNFG_FLAG_ENABLE_EXHAUSTIVE_PATTERN_TEST;
//         }
//         globals.simicsRunning = false;
//     }

//     // get the workflow for each target mba passed in.
//     // associate each workflow with the target handle.
//     WorkFlowAssocMap list;
//     TargetHandleList::const_iterator tit;
//     DiagMode mode;
//     for(tit = i_targetList.begin(); tit != i_targetList.end(); ++tit)
//     {
//         // mode = 0 (ONE_PATTERN) is the default output
//         getDiagnosticMode(globals, *tit, mode);
//         // create a list with patterns
//         // for ONE_PATTERN the list is as follows
//         // list[0] = RESTORE_DRAM_REPAIRS
//         // list[1] = START_PATTERN_0
//         // list[2] = START_SCRUB
//         // list[3] = CLEAR_HW_CHANGED_STATE
//         //
//         // for FOUR_PATTERNS it can also be
//         // list[0] = RESTORE_DRAM_REPAIRS
//         // list[1] START_RANDOM_PATTERN
//         // list[2] START_SCRUB
//         // list[3] START_PATTERN_2
//         // list[4] START_SCRUB
//         // list[5] START_PATTERN_1
//         // list[6] START_SCRUB
//         // list[7] CLEAR_HW_CHANGED_STATE
//         //
//         // or dor NINE_PATTERNS
//         // list[0] = RESTORE_DRAM_REPAIRS
//         // list[1] START_PATTERN_7
//         // list[2] START_SCRUB
//         // list[3] START_PATTERN_6
//         // list[4] START_SCRUB
//         // list[5] START_PATTERN_5
//         // list[6] START_SCRUB
//         // list[7] START_PATTERN_4
//         // list[8] START_SCRUB
//         // list[9] START_PATTERN_3
//         // list[10] START_SCRUB
//         // list[11] CLEAR_HW_CHANGED_STATE
//         getWorkFlow(mode, list[*tit], globals);
//     }

//     // set global data
//     Singleton<StateMachine>::instance().setGlobals(globals);
//     // calling errlHndl_t StateMachine::run(const WorkFlowAssocMap & i_list)
//     Singleton<StateMachine>::instance().run(list);

//     // If this step completes without the need for a reconfig due to an RCD
//     // parity error, clear all RCD parity error counters.
//     ATTR_RECONFIGURE_LOOP_type attr = top->getAttr<ATTR_RECONFIGURE_LOOP>();
//     if (0 == (attr & RECONFIGURE_LOOP_RCD_PARITY_ERROR))
//     {
//         TargetHandleList trgtList; getAllChiplets(trgtList, TYPE_MCA);
//         for (auto & trgt : trgtList)
//         {
//             if (0 != trgt->getAttr<ATTR_RCD_PARITY_RECONFIG_LOOP_COUNT>())
//                 trgt->setAttr<ATTR_RCD_PARITY_RECONFIG_LOOP_COUNT>(0);
//         }
//     }
// }

void memDiag(void){

    // memory diagnostics ipl step entry point
    Globals globals;
    TargetHandle_t top = nullptr;
    targetService().getTopLevelTarget(top);

    if(top)
    {
        globals.mfgPolicy = top->getAttr<ATTR_MNFG_FLAGS>();
        // by default 0
        // see hostboot/src/usr/targeting/common/xmltohb/attribute_types.xml:7292
        uint8_t maxMemPatterns = top->getAttr<ATTR_RUN_MAX_MEM_PATTERNS>();
        // This registry / attr is the same as the
        // exhaustive mnfg one
        if(maxMemPatterns)
        {
            globals.mfgPolicy |= MNFG_FLAG_ENABLE_EXHAUSTIVE_PATTERN_TEST;
        }
        globals.simicsRunning = false;
    }

    // get the workflow for each target mba passed in.
    // associate each workflow with the target handle.
    WorkFlowAssocMap list;
    TargetHandleList::const_iterator tit;
    DiagMode mode;
    for(tit = i_targetList.begin(); tit != i_targetList.end(); ++tit)
    {
        // mode = 0 (ONE_PATTERN) is the default output
        getDiagnosticMode(globals, *tit, mode);
        // create a list with patterns
        // for ONE_PATTERN the list is as follows
        // list[0] = RESTORE_DRAM_REPAIRS
        // list[1] = START_PATTERN_0
        // list[2] = START_SCRUB
        // list[3] = CLEAR_HW_CHANGED_STATE
        //
        // for FOUR_PATTERNS it can also be
        // list[0] = RESTORE_DRAM_REPAIRS
        // list[1] START_RANDOM_PATTERN
        // list[2] START_SCRUB
        // list[3] START_PATTERN_2
        // list[4] START_SCRUB
        // list[5] START_PATTERN_1
        // list[6] START_SCRUB
        // list[7] CLEAR_HW_CHANGED_STATE
        //
        // or dor NINE_PATTERNS
        // list[0] = RESTORE_DRAM_REPAIRS
        // list[1] START_PATTERN_7
        // list[2] START_SCRUB
        // list[3] START_PATTERN_6
        // list[4] START_SCRUB
        // list[5] START_PATTERN_5
        // list[6] START_SCRUB
        // list[7] START_PATTERN_4
        // list[8] START_SCRUB
        // list[9] START_PATTERN_3
        // list[10] START_SCRUB
        // list[11] CLEAR_HW_CHANGED_STATE
        getWorkFlow(mode, list[*tit], globals);
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
