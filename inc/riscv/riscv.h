#ifndef _INC_RISCV_H
#define _INC_RISCV_H

#include <inc/riscv/types.h>

#ifdef __ASSEMBLY__
#define __ASM_STR(x)	x
#else
#define __ASM_STR(x)	#x
#endif

#define csr_set(csr, val)                                          \
	({                                                         \
		unsigned long __v = (unsigned long)(val);          \
		__asm__ __volatile__("csrs " __ASM_STR(csr) ", %0" \
				     :                             \
				     : "rK"(__v)                   \
				     : "memory");                  \
	})

#define csr_clear(csr, val)                                        \
	({                                                         \
		unsigned long __v = (unsigned long)(val);          \
		__asm__ __volatile__("csrc " __ASM_STR(csr) ", %0" \
				     :                             \
				     : "rK"(__v)                   \
				     : "memory");                  \
	})
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

static inline void
load_satp_asid(uint64_t physical, uint16_t asid)
{
  uint64_t satp_ = 0;
  uint64_t asid_mask = 0xf0000fffffffffff;
  asm volatile("csrr %0,satp\n" \
	       "and  %0, %0, %3\n" \
	       "srli %0, %0, 44\n"\
	       "slli %0, %0, 44\n"\
	       "srli %1, %1, 12 \n" \
	       "or %0, %0, %1\n" \
	       "slli %2, %2, 44\n" \
	       "or %0, %0, %2\n" \
	       "sfence.vma\n" \
	       "csrw satp, %0\n" \
	       :
	       : "r"(satp_) , "r"(physical), "r"(asid), "r"(asid_mask)
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


static inline void
atomic_add(int i, unsigned long* p){
  unsigned long result = 0;
  asm volatile("amoadd.d %[result], %[i], (%[p])\n"
	       :[result]"=&r"(result), [p]"+r"(p)
	       :[i]"r"(i)
	       :"memory");
}

#define __axchg(ptr, new, size)							\
	({									\
		__typeof__(ptr) __ptr = (ptr);					\
		__typeof__(new) __new = (new);					\
		__typeof__(*(ptr)) __ret;					\
		switch (size) {							\
		case 4:								\
			__asm__ __volatile__ (					\
				"	amoswap.w.aqrl %0, %2, %1\n"		\
				: "=r" (__ret), "+A" (*__ptr)			\
				: "r" (__new)					\
				: "memory");					\
			break;							\
		case 8:								\
			__asm__ __volatile__ (					\
				"	amoswap.d.aqrl %0, %2, %1\n"		\
				: "=r" (__ret), "+A" (*__ptr)			\
				: "r" (__new)					\
				: "memory");					\
			break;							\
		default:							\
			break;							\
		}								\
		__ret;								\
	})

#define axchg(ptr, x)								\
	({									\
		__typeof__(*(ptr)) _x_ = (x);					\
		(__typeof__(*(ptr))) __axchg((ptr), _x_, sizeof(*(ptr)));	\
	})

static inline
unsigned long atomic_raw_xchg(volatile unsigned *ptr,
				    unsigned long newval)
{
	/* Atomically set new value and return old value. */
	return axchg(ptr, newval);
}


inline void local_flush_tlb_page_asid(unsigned long addr,
                 unsigned long asid)
{
         __asm__ __volatile__ ("sfence.vma %0, %1"
                         :
                         : "r" (addr), "r" (asid)
                         : "memory");
}


static inline void
breakpoint(void)
{
	asm volatile("ebreak");
}

static inline uint32_t
read_w(uint64_t addr){
  uint32_t val;
  asm volatile("ld %0, (%1)"
	       : "=r"(val)
	       : "r"(addr)
	       : "memory");
  return val;
}

static inline void
write_w(uint64_t addr, uint32_t data){
  asm volatile("sd %1, (%0)\n"
	       :
	       :"r"(addr), "r"(data)
	       :"memory");
}
#endif /* !_INC_RISCV_H */
