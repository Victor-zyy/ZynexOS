#ifndef _INC_RISCV_H
#define _INC_RISCV_H

#include <riscv/types.h>

static inline uint64_t
read_fp(void)
{
	uint64_t fp;
	asm volatile("mv %0,s0" : "=r" (fp));
	return fp;
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
