#ifndef _INC_RISCV_H
#define _INC_RISCV_H



#define wfi()                                             \
	do {                                              \
		__asm__ __volatile__("wfi" ::: "memory"); \
	} while (0)



#endif /* !_INC_RISCV_H */
