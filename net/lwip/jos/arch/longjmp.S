#ifndef _ALIGN_TEXT
#define _ALIGN_TEXT .align 16, 0x90
#endif

#define ENTRY(x) \
        .text; _ALIGN_TEXT; .globl x; .type x,@function; x:

#include <arch/riscv/setjmp.h>

ENTRY(jos_setjmp)
	// a0 is jos_jmp_buf

	// ra is stored by jal instruction
	sd	ra, 	JB_RA(a0)
	
	// where sp will point when we return
	sd	sp,	JB_SP(a0)

	// save other regs of contexts 
	sd	s0,	JB_S0(a0)
	sd	s1,	JB_S1(a0)
	sd	s2,	JB_S2(a0)
	sd	s3,	JB_S3(a0)
	sd	s4,	JB_S4(a0)
	sd	s5,	JB_S5(a0)
	sd	s6,	JB_S6(a0)
	sd	s7,	JB_S7(a0)
	sd	s8,	JB_S8(a0)
	sd	s9,	JB_S9(a0)
	sd	s10,	JB_S10(a0)
	sd	s11,	JB_S11(a0)

	mv	a0,	zero
	ret

ENTRY(jos_longjmp)
	// a0 is the jos_jmp_buf*
	// a1 is the return value

	// 	restore context of riscv
	ld	s0,	JB_S0(a0)
	ld	s1,	JB_S1(a0)
	ld	s2,	JB_S2(a0)
	ld	s3,	JB_S3(a0)
	ld	s4,	JB_S4(a0)
	ld	s5,	JB_S5(a0)
	ld	s6,	JB_S6(a0)
	ld	s7,	JB_S7(a0)
	ld	s8,	JB_S8(a0)
	ld	s9,	JB_S9(a0)
	ld	s10,	JB_S10(a0)
	ld	s11, 	JB_S11(a0)

	ld	t0,	JB_RA(a0)	// t0 = ra
	ld	sp,	JB_SP(a0)	// restore the sp address

	mv	a0,	a1
	jr	t0


