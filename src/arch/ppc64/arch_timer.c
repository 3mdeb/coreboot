/* SPDX-License-Identifier: GPL-2.0-only */

#include <timer.h>

/* Refer to hostboot/src/kernel/timemgr.C */

/* Time base frequency is 512 MHz so 512 ticks per usec */
#define TB_TICKS_PER_USEC 512

static inline uint64_t get_time_base(void)
{
	uint64_t tb = 0;
	asm volatile("mfspr %0, 268" : "=r" (tb));
	return tb;
}

static struct monotonic_counter {
	int initialized;
	struct mono_time time;
	uint64_t last_value;
} mono_counter;

void timer_monotonic_get(struct mono_time *mt)
{
	uint64_t current_tick;
	uint64_t usecs_elapsed;

	if (!mono_counter.initialized) {
		mono_counter.last_value = get_time_base();
		mono_counter.initialized = 1;
	}

	current_tick = get_time_base();
	usecs_elapsed = (current_tick - mono_counter.last_value) / TB_TICKS_PER_USEC;

	/* Update current time and tick values only if a full tick occurred. */
	if (usecs_elapsed) {
		mono_time_add_usecs(&mono_counter.time, usecs_elapsed);
		mono_counter.last_value = current_tick;
	}

	/* Save result. */
	*mt = mono_counter.time;
}
