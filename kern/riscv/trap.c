#include <inc/riscv/mmu.h>
#include <inc/riscv/riscv.h>
#include <inc/riscv/csr.h>
#include <inc/riscv/assert.h>

#include <kern/riscv/pmap.h>
#include <kern/riscv/trap.h>
#include <kern/riscv/console.h>
#include <kern/riscv/monitor.h>
#include <kern/riscv/env.h>
#include <kern/riscv/syscall.h>


/* For debugging, so print_trapframe can distinguish between printing
 * a saved trapframe and printing the current trapframe and print some
 * additional information in the latter case.
 */
static struct Trapframe *last_tf;

static const char *trapname(int trapno)
{
	static const char * const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < ARRAY_SIZE(excnames))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";
	return "(unknown trap)";
}


void
trap_init(void)
{
	// LAB 3: Your code here.

	// Per-CPU setup 
	trap_init_percpu();
}

// Initialize and load the per-CPU TSS and IDT
void
trap_init_percpu(void)
{
}

void
print_trapframe(struct Trapframe *tf)
{

	cprintf("TRAP frame at %p\n", tf);
	#if 0
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	// If this trap was a page fault that just happened
	// (so %cr2 is meaningful), print the faulting linear address.
	if (tf == last_tf && tf->tf_trapno == T_PGFLT)
		cprintf("  cr2  0x%08x\n", rcr2());
	cprintf("  err  0x%08x", tf->tf_err);
	// For page faults, print decoded fault error code:
	// U/K=fault occurred in user/kernel mode
	// W/R=a write/read caused the fault
	// PR=a protection violation caused the fault (NP=page not present).
	if (tf->tf_trapno == T_PGFLT)
		cprintf(" [%s, %s, %s]\n",
			tf->tf_err & 4 ? "user" : "kernel",
			tf->tf_err & 2 ? "write" : "read",
			tf->tf_err & 1 ? "protection" : "not-present");
	else
		cprintf("\n");
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	if ((tf->tf_cs & 3) != 0) {
		cprintf("  esp  0x%08x\n", tf->tf_esp);
		cprintf("  ss   0x----%04x\n", tf->tf_ss);
	}
	#endif
}

void
print_regs(struct PushRegs *regs)
{
  #if 0
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
  #endif 
}

static void
trap_dispatch(struct Trapframe *tf)
{
  // Handle processor exceptions.
  // LAB 3: Your code here.
  if (tf->scause & (1UL << (__riscv_xlen - 1))) {
	    tf->scause &= ~(1UL << (__riscv_xlen - 1));
	    switch(tf->scause){
	    default:
		break;
	    }
	    return;
    }
   switch(tf->scause){
	//page fault
     case T_IPGFLT:
	  page_fault_handler(tf);
	  return;

     case T_LPGFLT:
	  page_fault_handler(tf);
	  return;

     case T_BRKPT:
	//break point exception
       monitor(tf);
       return;
     case T_SYSCALL:
	// Systemcall
       tf->a0 = syscall(tf->a0, tf->a1, tf->a2, tf->a3, tf->a4, tf->a5);
       tf->sepc += 4;
       return;
     default: break;
   }
   // Unexpected trap: The user process or the kernel has a bug.
    print_trapframe(tf);
    if (((tf->status & SSTATUS_SPP) >> SPP_SHIFT ) == 1)
	    panic("unhandled trap in kernel");
    else {
	    env_destroy(curenv);
	    return;
    }
}

void
trap(struct Trapframe *tf)
{
	// The environment may have set DF and some versions
	// of GCC rely on DF being clear

	// Check that interrupts are disabled.  If this assertion
	// fails, DO NOT be tempted to fix it by inserting a "cli" in
	// the interrupt path.
	assert(!(read_status() & SSTATUS_SIE));

	cprintf("Incoming TRAP frame at %p\n", tf);

	if (((tf->status & SSTATUS_SPP ) >> SPP_SHIFT) == 0) {
		// Trapped from user mode.
		assert(curenv);

		// Copy trap frame (which is currently on the stack)
		// into 'curenv->env_tf', so that running the environment
		// will restart at the trap point.
		curenv->env_tf = *tf;
		// The trapframe on the stack should be ignored from here on.
		tf = &curenv->env_tf;
	}

	// Record that tf is the last real trapframe so
	// print_trapframe can print some additional information.
	last_tf = tf;

	// Dispatch based on what type of trap occurred
	trap_dispatch(tf);

	// Return to the current environment, which should be running.
	assert(curenv && curenv->env_status == ENV_RUNNING);
	env_run(curenv);
}


void
page_fault_handler(struct Trapframe *tf)
{
	uint32_t fault_va;

	// Read processor's CR2 register to find the faulting address
	fault_va = tf->stval;

	// Handle kernel-mode page faults.

	// LAB 3: Your code here.
	if (((tf->status & SSTATUS_SPP ) >> SPP_SHIFT) == 0) {
		panic("page-fault form kernel mode!");
	}

	// We've already handled kernel-mode exceptions, so if we get here,
	// the page fault happened in user mode.

	// Destroy the environment that caused the fault.
	cprintf("[%08x] user fault va %08x ip %08x\n",
		curenv->env_id, fault_va, tf->sepc);
	print_trapframe(tf);
	env_destroy(curenv);
}

