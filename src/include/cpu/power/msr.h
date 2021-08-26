/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_MSR_H
#define CPU_PPC64_MSR_H

static inline uint64_t getMSR(void)
{
    uint64_t msr;
    asm volatile("mfmsr %0" : "=r" (msr));
    return msr;
}

static inline void setMSR(uint64_t msr)
{
    asm volatile("mtmsr %0; isync" :: "r" (msr));
}

#endif /* CPU_PPC64_MSR_H */
