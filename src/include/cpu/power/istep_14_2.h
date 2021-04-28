#ifndef ISTEP_14_2
#define ISTEP_14_2

#include <cpu/power/scom.h>

#define MCA_MBA_FARB3Q (0x7010916)
#define MCS_MCSYNC (0x5010815)
#define MCS_MCMODE0 (0x5010811)
#define MCS_MCMODE0_ENABLE_EMER_THROTTLE (21)
#define MCS_MCSYNC_SYNC_GO_CH0 (16)
#define MCS_MCSYNC_CHANNEL_SELECT (0)
#define MCS_MCSYNC_CHANNEL_SELECT_LEN (8)
#define MCS_MCSYNC_SYNC_TYPE (8)
#define MCS_MCSYNC_SYNC_TYPE_LEN (8)
#define SUPER_SYNC_BIT (14)
#define MCS_MCSYNC_SYNC_GO_CH0 (16)
#define MBA_REFRESH_SYNC_BIT (8)
#define MAX_MC_SIDES_PER_PROC (2)
#define MCS_MCMODE0_DISABLE_MC_SYNC (27)
#define MCS_MCMODE0_DISABLE_MC_PAIR_SYNC (28)

#define LEN(a) (sizeof(a)/sizeof(*a))
chiplet_id_t MCSTargets[] = {MC01_CHIPLET_ID, MC23_CHIPLET_ID};

void istep_14_2(void);
void thermalInit(void);
void throttleSync(void);
void progMaster(chiplet_id_t MCSTTarget);
void progMCSMode0(chiplet_id_t MCSTTarget);
#endif
