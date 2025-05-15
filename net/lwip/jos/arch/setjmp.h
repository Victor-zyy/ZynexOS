#ifndef JOS_INC_SETJMP_H
#define JOS_INC_SETJMP_H

#include <arch/riscv/setjmp.h>

int  jos_setjmp(volatile struct jos_jmp_buf *buf);
void jos_longjmp(volatile struct jos_jmp_buf *buf, int val)
	__attribute__((__noreturn__));

#endif
