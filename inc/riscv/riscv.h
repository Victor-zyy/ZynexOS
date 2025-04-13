#ifndef _INC_RISCV_H
#define _INC_RISCV_H

#include <riscv/types.h>

#define ASID_MASK(satp) ((satp >> 44) & 0x0ffff)
#define ASID_PPN(satp) ((satp & 0x00000fffffffffff)

static inline void
load_satp(uint64_t physical)
{
  uint64_t satp_ = 0;
  asm volatile("csrr %0,satp\n" \
	       "srli %1, %1, 12 \n" \
	       "or %0, %0, %1\n" \
	       "csrw satp, %0\n" \
	       "sfence.vma\n" \
	       :
	       : "r"(satp_) , "r"(physical)
	       );
}
static inline uint64_t
read_fp(void)
{
	uint64_t fp;
	asm volatile("mv %0,s0" : "=r" (fp));
	return fp;
}

static inline uint64_t
read_asid(void)
{
	uint64_t asid;
	asm volatile("csrr %0,satp" : "=r" (asid));
	return ASID_MASK(asid);
}
#define wfi()                                             \
	do {                                              \
		__asm__ __volatile__("wfi" ::: "memory"); \
	} while (0)

static inline int
cpunum(void)
{
  return 0;
}

#endif /* !_INC_RISCV_H */
