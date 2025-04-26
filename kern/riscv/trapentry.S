/* See COPYRIGHT for copyright information. */

#include <inc/riscv/mmu.h>
#include <inc/riscv/memlayout.h>
#include <inc/riscv/trap.h>

#include <inc/riscv/csr.h>
#include <inc/riscv/asm-offset.h>
#include <inc/riscv/env.h>


###################################################################
# exceptions/interrupts
###################################################################


/*
 * Lab 3: Your code here for _alltraps
 */

	.section .text
	.global _alltraps
	.align 4
_alltraps:
	// step1. stack switch if necessary
	// swap tp and scratch in smode
	// if we traped into kernel from user-mode then the sscratch is kernel stack
	// but if we traped into kernel form kernel-mode then the sscratch is zero
	// refered to PA4-NJU
	csrrw	sp, CSR_SSCRATCH, sp
	// sp != 0 means traped from user mode
	bnez	sp, _save_context
	// sp == 0 means trapped form S mode
	csrr	sp, CSR_SSCRATCH

_save_context:	
	// save registers
	addi	sp, sp, -PT_SIZE
	sd	t0, PT_T0(sp)

	
	// save the stack
	csrr 	t0, sscratch
	sd	t0, PT_SP(sp)
	
	li	t0, 0
	csrw	sscratch, t0

	sd	ra, PT_RA(sp)
	sd	fp, PT_FP(sp)
	sd	s1, PT_S1(sp)
	sd	s2, PT_S2(sp)
	sd	s3, PT_S3(sp)
	sd	s4, PT_S4(sp)
	sd	s5, PT_S5(sp)
	sd	s6, PT_S6(sp)
	sd	s7, PT_S7(sp)
	sd	s8, PT_S8(sp)
	sd	s9, PT_S9(sp)
	sd	s10, PT_S10(sp)
	sd	s11, PT_S11(sp)
	sd	tp, PT_TP(sp)
	sd	a0, PT_A0(sp)
	sd	a0, PT_ORIG_A0(sp)
	sd	a1, PT_A1(sp)
	sd	a2, PT_A2(sp)
	sd	a3, PT_A3(sp)
	sd	a4, PT_A4(sp)
	sd	a5, PT_A5(sp)
	sd	a6, PT_A6(sp)
	sd	a7, PT_A7(sp)
	sd	t1, PT_T1(sp)
	sd	t2, PT_T2(sp)
	sd	t3, PT_T3(sp)
	sd	t4, PT_T4(sp)
	sd	t5, PT_T5(sp)
	sd	t6, PT_T6(sp)
	sd	gp, PT_GP(sp)

	// save sstatus
	csrr 	t0, sstatus
	sd	t0, PT_SSTATUS(sp)

	// save stval
	csrr 	t0, stval
	sd	t0, PT_STVAL(sp)

	// save scause
	csrr 	t0, scause
	sd	t0, PT_SCAUSE(sp)

	// save epc
	csrr 	t0, sepc
	sd	t0, PT_SEPC(sp)

	mv	a0, sp
	call trap;

.global _alltraps_ret
// call trap ever return
_alltraps_ret:
	// now sp is set to trapframe bottom

	ld	ra, PT_RA(sp)
	ld	s0, PT_S0(sp)
	ld	s1, PT_S1(sp)
	ld	s2, PT_S2(sp)
	ld	s3, PT_S3(sp)
	ld	s4, PT_S4(sp)
	ld	s5, PT_S5(sp)
	ld	s6, PT_S6(sp)
	ld	s7, PT_S7(sp)
	ld	s8, PT_S8(sp)
	ld	s9, PT_S9(sp)
	ld	s10, PT_S10(sp)
	ld	s11, PT_S11(sp)
	ld	tp, PT_TP(sp)
	ld	a0, PT_A0(sp)
	ld	a1, PT_A1(sp)
	ld	a2, PT_A2(sp)
	ld	a3, PT_A3(sp)
	ld	a4, PT_A4(sp)
	ld	a5, PT_A5(sp)
	ld	a6, PT_A6(sp)
	ld	a7, PT_A7(sp)
	ld	t3, PT_T3(sp)
	ld	t4, PT_T4(sp)
	ld	t5, PT_T5(sp)
	ld	t6, PT_T6(sp)
	ld	gp, PT_GP(sp)

	// use t0 to set csr registers
	ld	t0, PT_SEPC(sp)
	csrw	sepc, 	t0

	ld	t0, PT_ORIG_A0(sp)
	addi	t0, t0, (-0xa)
	bnez	t0, yield_sepc_remain
	ld	t0, PT_SEPC(sp)
	addi	t0,	t0, 0x4
	csrw	sepc, 	t0


yield_sepc_remain:	


	ld	t0, PT_SCAUSE(sp)
	csrw	scause, t0

	ld	t0, PT_STVAL(sp)
	csrw	stval, t0

	ld	t0, PT_SSTATUS(sp)
	li	t1, 1
	slli	t1, t1, 8
	and	t0, t0, t1
	beqz 	t0, scratch_save_ // user mode

	li	t0, 0
	csrw	sscratch, t0

	ld	t0, PT_SSTATUS(sp)
	csrw	sstatus, t0
	ld	t0, PT_T0(sp)
	ld	t1, PT_T1(sp)
	ld	sp, PT_SP(sp)
	sret;
	
	// sp finally
scratch_save_:	
	// tp points to env of itself
	// t0 equals to cpunum which the env runs on it
	lw	t0,  ENV_CPUNUM_OFF(tp)
	// t1 points to the cpu0 kstack top
	// KSTKSIZE + KSTKGAP
	li	t1, (KSTKSIZE)
	li	t2, (KSTKGAP)
	add	t2, t1, t2
	// calculate the gap
	mulw    t2,  t0, t2	
	//
	li 	t1, (KSTACKTOP)
	sub	t1, t1, t2

	csrw	sscratch, t1

	ld	t0, PT_SSTATUS(sp)
	csrw	sstatus, t0

	ld	t0, PT_T0(sp)
	ld	t1, PT_T1(sp)
	ld	t2, PT_T2(sp)
	ld	sp, PT_SP(sp)


	sret;
