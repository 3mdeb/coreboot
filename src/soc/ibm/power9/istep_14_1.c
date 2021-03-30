/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/scom.h>

errlHndl_t runStep(const TargetHandleList & i_targetList)
{
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

void memDiag(void){
    return;
}

#define DSM0Q_REG   0x0701090A
#define MBA_FARB0Q  0x07010913
#define MCA_ACTION1 0x07010A07
#define MCA_MBACALFIRQ 0x7010940
#define MCA_FIR     0x7010A00

fapi2::ReturnCode after_memdiags(const fapi2::Target<fapi2::TARGET_TYPE_MCBIST>& i_target)
{
    fapi2::ReturnCode l_rc1, l_rc2;
    fapi2::buffer<uint64_t> dsm0_buffer;
    fapi2::buffer<uint64_t> l_mnfg_buffer;
    uint64_t rd_tag_delay = 0;
    uint64_t wr_done_delay = 0;
    fapi2::buffer<uint64_t> l_aue_buffer;

    // Broadcast mode workaround for UEs causing out of sync
    mss::workarounds::mcbist::broadcast_out_of_sync(i_target, mss::ON);

    for(unsigned int port = 0; port < 4; ++port)
    {
        fir::reg<MCA_FIR> l_ecc64_fir_reg(p, l_rc1);
        fir::reg<MCA_MBACALFIRQ> l_cal_fir_reg(p, l_rc2);
        uint64_t rcd_protect_time = 0;

        dsm0_buffer = scom_read(DSM0Q_REG | (port << PORT_OFFSET));
        wr_done_delay = (dsm0_buffer & 0xFC00000000) >> 34;
        rd_tag_delay = (i_data & 0xFC00000) >> 22;
        rcd_protect_time = min(wr_done_delay, rd_tag_delay);
        scom_and_or(MBA_FARB0Q | (port << PORT_OFFSET), ~0xFC00, (rcd_protect_time & 0x3F) << 10);

        l_ecc64_fir_reg.checkstop<MCA_FIR_MAINLINE_AUE>()
          .recoverable_error<MCA_FIR_MAINLINE_UE>()
          .checkstop<MCA_FIR_MAINLINE_IAUE>()
          .recoverable_error<MCA_FIR_MAINLINE_IUE>();

        l_cal_fir_reg.recoverable_error<MCA_MBACALFIRQ_PORT_FAIL>();
        if (CHIP_EC == 0x20)
        {
            l_ecc64_fir_reg
              .checkstop<MCA_FIR_MAINLINE_UE>()
              .checkstop<MCA_FIR_MAINLINE_RCD>();
            l_cal_fir_reg.checkstop<MCA_MBACALFIRQ_PORT_FAIL>();
        }

        // If MNFG FLAG Threshhold is enabled skip IUE unflagging
        mss::mnfg_flags(l_mnfg_buffer);

        if (!l_mnfg_buffer.getBit<63>())
        {
            l_ecc64_fir_reg.recoverable_error<MCA_FIR_MAINTENANCE_IUE>();
        }

        l_ecc64_fir_reg.write();
        l_cal_fir_reg.write();

        scom_and(MCA_ACTION1 | (port << PORT_OFFSET), ~0x48000000)
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
            scom_or(RRQ0Q_REG | (port << PORT_OFFSEET), 1 << MBA_RRQ0Q_CFG_RRQ_FIFO_MODE);
        }
        for(unsigned int port = 0; port < 4; ++port) {
            scom_or(WRQ0Q_REG | (port << PORT_OFFSEET), 1 << MBA_WRQ0Q_CFG_WRQ_FIFO_MODE);
        }
    }
    printk(BIOS_EMERG, "ending istep 14.1\n");
    return;
}
