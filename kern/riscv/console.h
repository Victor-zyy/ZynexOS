/* See COPYRIGHT for copyright information. */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/riscv/types.h>


void cons_init(void);
int cons_getc(void);

void serial_intr(void);

#endif /* _CONSOLE_H_ */
