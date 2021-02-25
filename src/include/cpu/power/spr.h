#ifndef CPU_PPC64_SPR_H
#define CPU_PPC64_SPR_H

#include <arch/byteorder.h>	// PPC_BIT()

#define SPR_HMER			0x150
/* Bits in HMER/HMEER */
#define SPR_HMER_MALFUNCTION_ALERT	PPC_BIT(0)
#define SPR_HMER_PROC_RECV_DONE		PPC_BIT(2)
#define SPR_HMER_PROC_RECV_ERROR_MASKED	PPC_BIT(3)
#define SPR_HMER_TFAC_ERROR		PPC_BIT(4)
#define SPR_HMER_TFMR_PARITY_ERROR	PPC_BIT(5)
#define SPR_HMER_XSCOM_FAIL		PPC_BIT(8)
#define SPR_HMER_XSCOM_DONE		PPC_BIT(9)
#define SPR_HMER_PROC_RECV_AGAIN	PPC_BIT(11)
#define SPR_HMER_WARN_RISE		PPC_BIT(14)
#define SPR_HMER_WARN_FALL		PPC_BIT(15)
#define SPR_HMER_SCOM_FIR_HMI		PPC_BIT(16)
#define SPR_HMER_TRIG_FIR_HMI		PPC_BIT(17)
#define SPR_HMER_HYP_RESOURCE_ERR	PPC_BIT(20)
#define SPR_HMER_XSCOM_STATUS		PPC_BITMASK(21,23)
#define SPR_HMER_XSCOM_OCCUPIED		PPC_BIT(23)

#ifndef __ASSEMBLER__
#include <types.h>

static inline uint64_t read_hmer(void)
{
	uint64_t val;
	asm volatile("mfspr %0,%1" : "=r"(val) : "i"(SPR_HMER) : "memory");
	return val;
}

static inline void clear_hmer(void)
{
	asm volatile("mtspr %0, %1" :: "i"(SPR_HMER), "r"(0) : "memory");
}

static inline uint64_t read_msr(void)
{
	uint64_t val;
	asm volatile("mfmsr %0" : "=r"(val) :: "memory");
	return val;
}

#endif /* __ASSEMBLER__ */
#endif /* CPU_PPC64_SPR_H */
