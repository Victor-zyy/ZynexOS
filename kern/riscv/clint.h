#ifndef CLINT_H
#define CLINT_H

#include <inc/riscv/csr.h>
#include <inc/riscv/riscv.h>
#define TIME_SECOND_TICKS 10000000
#define TIME_MILISECOND_TICKS 10000
#define TIME_TENMILISECOND_TICKS 100000

void clint_init(void);
void reset_timer(void);

#endif
