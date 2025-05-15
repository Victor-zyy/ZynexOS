#ifndef JOS_MACHINE_SETJMP_H
#define JOS_MACHINE_SETJMP_H

#ifndef __ASSEMBLER__
#include <inc/riscv/types.h>


struct jos_jmp_buf {
    uint64_t jb_ra;
    uint64_t jb_sp;
    uint64_t jb_s0;
    uint64_t jb_s1;
    uint64_t jb_s2;
    uint64_t jb_s3;
    uint64_t jb_s4;
    uint64_t jb_s5;
    uint64_t jb_s6;
    uint64_t jb_s7;
    uint64_t jb_s8;
    uint64_t jb_s9;
    uint64_t jb_s10;
    uint64_t jb_s11;
};
#else
#define JB_RA  0
#define JB_SP  8
#define JB_S0  16
#define JB_S1  24
#define JB_S2  32
#define JB_S3  40
#define JB_S4  48
#define JB_S5  56
#define JB_S6  64
#define JB_S7  72
#define JB_S8  80
#define JB_S9  88
#define JB_S10  96
#define JB_S11  104
#endif
#endif
