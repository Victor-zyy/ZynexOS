#ifndef _INC_RISCV_H
#define _INC_RISCV_H

#include <inc/riscv/types.h>

#define ASID_MASK(satp) ((satp >> 44) & 0x0ffff)
#define ASID_PPN(satp) ((satp & 0x00000fffffffffff)
#define ASID_PPN_MASK 0xfffff00000000000

static inline void
load_satp(uint64_t physical)
{
  uint64_t satp_ = 0;
  asm volatile("csrr %0,satp\n" \
	       "srli %0, %0, 44\n"\
	       "slli %0, %0, 44\n"\
	       "srli %1, %1, 12 \n" \
	       "or %0, %0, %1\n" \
	       "sfence.vma\n" \
	       "csrw satp, %0\n" \
	       :
	       : "r"(satp_) , "r"(physical)
	       : "cc", "memory"
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

static inline uint64_t
read_status(void)
{
	uint64_t status;
	asm volatile("csrr %0,sstatus" : "=r" (status));
	return status;
}

static inline void
set_status_sum(bool sum)
{
	uint64_t status = 0;
	asm volatile("csrr %0,sstatus\n"\
		     "slli %1, %1, 18\n"\
		     "or  %0, %0, %1\n"\
		     "csrw sstatus, %0\n"\
		     :
		     : "r" (status), "r"(sum)
		     : "cc", "memory"
		     );
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

static inline void
atomic_add(int i, unsigned long* p){
  unsigned long result = 0;
  asm volatile("amoadd.d %[result], %[i], (%[p])\n"
	       :[result]"=&r"(result), [p]"+r"(p)
	       :[i]"r"(i)
	       :"memory");
}
#endif /* !_INC_RISCV_H */
