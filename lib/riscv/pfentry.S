#include <inc/riscv/mmu.h>
#include <inc/riscv/trap.h>
#include <inc/riscv/memlayout.h>

// Page fault upcall entrypoint.

// This is where we ask the kernel to redirect us to whenever we cause
// a page fault in user space (see the call to sys_set_pgfault_handler
// in pgfault.c).
//
// When a page fault actually occurs, the kernel switches our ESP to
// point to the user exception stack if we're not already on the user
// exception stack, and then it pushes a UTrapframe onto our user
// exception stack:
//
//	trap-time esp
//	trap-time eflags
//	trap-time eip
//	utf_regs.reg_eax
//	...
//	utf_regs.reg_esi
//	utf_regs.reg_edi
//	utf_err (error code)
//	utf_fault_va            <-- %esp
//
// If this is a recursive fault, the kernel will reserve for us a
// blank word above the trap-time esp for scratch work when we unwind
// the recursive call.
//
// We then have call up to the appropriate page fault handler in C
// code, pointed to by the global variable '_pgfault_handler'.

.text
.globl _pgfault_upcall
_pgfault_upcall:
	// Call the C page fault handler.
	mv a0, sp // function argument: pointer to UTF
	ld	a1,(_pgfault_handler)
	jalr	a1
	//call 0(_pgfault_handler)
	
	// Now the C page fault handler has returned and you must return
	// to the trap time state.
	// Push trap-time %eip onto the trap-time stack.
	//
	// Explanation:
	//   We must prepare the trap-time stack for our eventual return to
	//   re-execute the instruction that faulted.
	//   Unfortunately, we can't return directly from the exception stack:
	//   We can't call 'jmp', since that requires that we load the address
	//   into a register, and all registers must have their trap-time
	//   values after the return.
	//   We can't call 'ret' from the exception stack either, since if we
	//   did, %esp would have the wrong value.
	//   So instead, we push the trap-time %eip onto the *trap-time* stack!
	//   Below we'll switch to that stack and call 'ret', which will
	//   restore %eip to its pre-fault value.
	//
	//   In the case of a recursive fault on the exception stack,
	//   note that the word we're pushing now will fit in the
	//   blank word that the kernel reserved for us.
	//
	// Throughout the remaining code, think carefully about what
	// registers are available for intermediate calculations.  You
	// may find that you have to rearrange your code in non-obvious
	// ways as registers become unavailable as scratch space.
	//
	// LAB 4: Your code here.
	ld  	t0, 	UTRAP_EPC(sp)             // get the sepc
	// Restore the trap-time registers.  After you do this, you
	// can no longer modify any general-purpose registers.
	// LAB 4: Your code here.

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

	// Restore eflags from the stack.  After you do this, you can
	// no longer use arithmetic operations or anything else that
	// modifies eflags.
	mv	gp,	t0
	ld	t0, 	PT_T0(sp)
	// Switch back to the adjusted trap-time stack.
	// LAB 4: Your code here.
	addi	sp,	sp, UTRAP_SP
	ld	sp,	(sp)
	// Return to re-execute the instruction that faulted.
	// LAB 4: Your code here.
	jr	 gp	
