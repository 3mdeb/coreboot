/* SPDX-License-Identifier: GPL-2.0-only */

#include <program_loading.h>


#if ENV_PAYLOAD_LOADER

#include <cpu/power/scom.h>


union PIR
{
    uint32_t word;

    struct
    {
	// Normal Core Mode
	uint32_t reserved0:17;   // 00:16 = unused
	uint32_t groupId:4;      // 17:20 = group id
	uint32_t chipId:3;       // 21:23 = chip id
	uint32_t reserved1:1;    //    24 = reserved
	uint32_t coreId:5;       // 25:29 = core id (normal core)
	uint32_t threadId:2;     // 30:31 = thread id (normal core)
    } __attribute__((packed));
};

// talos-hostboot/src/include/kernel/doorbell.H
enum
{
    DOORBELL_MSG_TYPE = 0x0000000028000000, /// Comes from the ISA.
};

// talos-hostboot/src/kernel/doorbell.C
static void doorbell_send(uint64_t i_pir)
{
    uint64_t msgtype = DOORBELL_MSG_TYPE;
    register uint64_t msg = msgtype | i_pir;
    asm volatile("msgsnd %0" :: "r" (msg));

    return;
}

#define MAX_CORES_PER_CHIP 24

static void istep_16_2(int this_core, uint64_t cores)
{
	for (int i = 0; i < MAX_CORES_PER_CHIP; i++) {
		uint64_t val = read_scom_for_chiplet(EC00_CHIPLET_ID + i, 0xF0040);
		if (val & PPC_BIT(0)) {
			union PIR pir = {0};
			pir.coreId = i;

			for (int thread = 0; thread < 4; ++thread) {
				if (i == this_core && thread == 0) thread++;
				pir.threadId = thread;
				doorbell_send(pir.word);
			}
		}
	}
}

/*
 * Payload's entry point is an offset to the real entry point, not to OPD
 * (Official Procedure Descriptor) for entry point.
 */
void arch_prog_run(struct prog *prog)
{
	istep_16_2(1, 0x5900000000000000);

	asm volatile(
	    "mtctr %1\n"
	    "mr 3, %0\n"
	    "bctr\n"
	    :: "r"(prog_entry_arg(prog)), "r"(prog_entry(prog)) : "memory");
}

#else

void arch_prog_run(struct prog *prog)
{
	void (*doit)(void *) = prog_entry(prog);

	doit(prog_entry_arg(prog));
}

#endif
