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
#include <kern/riscv/cpu.h>
#include <kern/riscv/sched.h>
#include <kern/riscv/spinlock.h>
#include <kern/riscv/clint.h>


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

// Initialize and load the per-CPU kern_sp
void
trap_init_percpu(void)
{

  thiscpu->kern_sp = (uintptr_t)(KSTACKTOP - cpunum() * (KSTKSIZE + KSTKGAP));
  
}


void
print_trapframe(struct Trapframe *tf)
{

        cprintf("TRAP frame at %p Base envs : 0x%08lx\n", tf, envs);
	print_regs(tf);
	#if 0
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
print_regs(struct Trapframe *regs)
{
	cprintf("  sepc      0x%08lx\n", regs->sepc);
	cprintf("  ra        0x%08lx\n", regs->ra);
	cprintf("  sp        0x%08lx\n", regs->sp);
	cprintf("  gp        0x%08lx\n", regs->gp);
	// parse the thread pointer
	cprintf("  tp        0x%08lx\n", regs->tp);
	cprintf("  env_n     0x%08lx\n", (struct Env*)regs->tp - envs);
	cprintf("  t0        0x%08lx\n", regs->t0);
	cprintf("  t1        0x%08lx\n", regs->t1);
	cprintf("  t2        0x%08lx\n", regs->t2);
	cprintf("  s0        0x%08lx\n", regs->s0);
	cprintf("  s1        0x%08lx\n", regs->s1);
	cprintf("  a0        0x%08lx\n", regs->a0);
	cprintf("  a1        0x%08lx\n", regs->a1);
	cprintf("  a2        0x%08lx\n", regs->a2);
	cprintf("  a3        0x%08lx\n", regs->a3);
	cprintf("  a4        0x%08lx\n", regs->a4);
	cprintf("  a5        0x%08lx\n", regs->a5);
	cprintf("  a6        0x%08lx\n", regs->a6);
	cprintf("  a7        0x%08lx\n", regs->a7);
	cprintf("  s2        0x%08lx\n", regs->s2);
	cprintf("  s3        0x%08lx\n", regs->s3);
	cprintf("  s4        0x%08lx\n", regs->s4);
	cprintf("  s5        0x%08lx\n", regs->s5);
	cprintf("  s6        0x%08lx\n", regs->s6);
	cprintf("  s7        0x%08lx\n", regs->s7);
	cprintf("  s8        0x%08lx\n", regs->s8);
	cprintf("  s9        0x%08lx\n", regs->s9);
	cprintf("  s10       0x%08lx\n", regs->s10);
	cprintf("  s11       0x%08lx\n", regs->s11);
	cprintf("  t3        0x%08lx\n", regs->t3);
	cprintf("  t4        0x%08lx\n", regs->t4);
	cprintf("  t5        0x%08lx\n", regs->t5);
	cprintf("  t6        0x%08lx\n", regs->t6);
	cprintf("  status    0x%08lx\n", regs->status);
	cprintf("  stval     0x%08lx\n", regs->stval);
	cprintf("  scause    0x%08lx\n", regs->scause);
	cprintf("  orig_a0   0x%08lx\n", regs->orig_a0);
}

static void
trap_dispatch(struct Trapframe *tf)
{
  // Handle processor exceptions.
  // LAB 3: Your code here.
  if (tf->scause & (1UL << (__riscv_xlen - 1))) {
	    tf->scause &= ~(1UL << (__riscv_xlen - 1));
	    switch(tf->scause){
	    case IRQ_TIMER: 
	      //cprintf("tf->orig_a0 : 0x%08lx\n", tf->orig_a0);
	      tf->orig_a0 = 0;
	      csr_clear(CSR_SIE, MIP_STIP);
	      // reset the mtimecmp register
	      reset_timer();/* FIXME:  interrupt nested */
	      // sched
	      sched_yield(1);
	    default:
	      cprintf("unhandle interrupt : 0x%x\n", tf->scause);
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
	// Systemcall /* FIXME: user mode a6 as the return value from asm code */
       // That is because the ecall you don't specify the a0 as the return val 
       tf->a0 = syscall(tf->a0, tf->a1, tf->a2, tf->a3, tf->a4, tf->a5);
       tf->sepc += 4;
       return;
     case T_SAMOPGFLT:
       page_fault_handler(tf);
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


        // Halt the CPU if some other CPU has called panic()
        extern char *panicstr;
        if (panicstr)
	  while(1);
        // Re-acqurie the big kernel lock if we were halted in
        // sched_yield()
        if (atomic_raw_xchg(&thiscpu->cpu_status, CPU_STARTED) == CPU_HALTED)
              lock_kernel();

	// Check that interrupts are disabled.  If this assertion
	// fails, DO NOT be tempted to fix it by inserting a "cli" in
	// the interrupt path.
	assert(!(read_status() & SSTATUS_SIE));

	if (((tf->status & SSTATUS_SPP ) >> SPP_SHIFT) == 0) {
		// Trapped from user mode.
                // Acquire the big kernel lock before doing any
                // serious kernel work.
                // LAB 4: Your code here.
                assert(curenv);
                lock_kernel();

                // Garbage collect if current enviroment is a zombie
                if (curenv->env_status == ENV_DYING) {
                        env_free(curenv);
                        curenv = NULL;
                        sched_yield(0);
                }

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

       // If we made it to this point, then no other environment was
       // scheduled, so we should return to the current environment
       // if doing so makes sense.
       if (curenv && curenv->env_status == ENV_RUNNING)
               env_run(curenv);
       else
               sched_yield(0);

}


void
page_fault_handler(struct Trapframe *tf)
{
	uint64_t fault_va;

	// Read processor's stval register to find the faulting address
	fault_va = tf->stval;

	// Handle kernel-mode page faults.

	// LAB 3: Your code here.
	if (((tf->status & SSTATUS_SPP ) >> SPP_SHIFT) == 1) {
	  panic("page-fault kernel mode va : 0x%08lx scause : 0x%x sepc  : 0x%08lx form kernel mode!", fault_va, tf->scause, tf->sepc);
	}

	// We've already handled kernel-mode exceptions, so if we get here,
	// the page fault happened in user mode.
// Call the environment's page fault upcall, if one exists.  Set up a
	// page fault stack frame on the user exception stack (below
	// UXSTACKTOP), then branch to curenv->env_pgfault_upcall.
	//
	// The page fault upcall might cause another page fault, in which case
	// we branch to the page fault upcall recursively, pushing another
	// page fault stack frame on top of the user exception stack.
	//
	// It is convenient for our code which returns from a page fault
	// (lib/pfentry.S) to have one word of scratch space at the top of the
	// trap-time stack; it allows us to more easily restore the eip/esp. In
	// the non-recursive case, we don't have to worry about this because
	// the top of the regular user stack is free.  In the recursive case,
	// this means we have to leave an extra word between the current top of
	// the exception stack and the new stack frame because the exception
	// stack _is_ the trap-time stack.
	//
	// If there's no page fault upcall, the environment didn't allocate a
	// page for its exception stack or can't write to it, or the exception
	// stack overflows, then destroy the environment that caused the fault.
	// Note that the grade script assumes you will first check for the page
	// fault upcall and print the "user fault va" message below if there is
	// none.  The remaining three checks can be combined into a single test.
	//
	// Hints:
	//   user_mem_assert() and env_run() are useful here.
	//   To change what the user environment runs, modify 'curenv->env_tf'
	//   (the 'tf' variable points at 'curenv->env_tf').

	// LAB 4: Your code here.
	// page fault happended in user mode
	// no page fault handler registered 
	//user_mem_assert(curenv, (void *)(UXSTACKTOP - PGSIZE), PGSIZE, PTE_W);

	if(curenv->env_pgfault_upcall == NULL){
		// JOS destroys the user environment with a message
		goto bad;
	}
	// check whether exception user stack is allocated or not
	user_mem_assert(curenv, (void *)(UXSTACKTOP - 4), 4 , PTE_W);

	// otherwise set up the exception stack
	// check whether we are in user exception user stack
	//
	uint64_t *sp = (uint64_t *)UXSTACKTOP;
	if(tf->sp >= (UXSTACKTOP-PGSIZE) && tf->sp < UXSTACKTOP){
		// handle recrusively call of user page fault handler
		sp = (uint64_t *)tf->sp;
		*(--sp) = 0; //empty word
	}	
	// check the user exception stack is overflowed or not
	*(--sp) = tf->sp;
	*(--sp) = tf->sepc;
	*(--sp) = tf->scause;
	*(--sp) = tf->t6;
	*(--sp) = tf->t5;
	*(--sp) = tf->t4;
	*(--sp) = tf->t3;
	*(--sp) = tf->s11;
	*(--sp) = tf->s10;
	*(--sp) = tf->s9;
	*(--sp) = tf->s8;
	*(--sp) = tf->s7;
	*(--sp) = tf->s6;
	*(--sp) = tf->s5;
	*(--sp) = tf->s4;
	*(--sp) = tf->s3;
	*(--sp) = tf->s2;
	*(--sp) = tf->a7;
	*(--sp) = tf->a6;
	*(--sp) = tf->a5;
	*(--sp) = tf->a4;
	*(--sp) = tf->a3;
	*(--sp) = tf->a2;
	*(--sp) = tf->a1;
	*(--sp) = tf->a0;
	*(--sp) = tf->s1;
	*(--sp) = tf->s0;
	*(--sp) = tf->t2;
	*(--sp) = tf->t1;
	*(--sp) = tf->t0;
	*(--sp) = tf->tp;
	*(--sp) = tf->gp;
	*(--sp) = tf->sp;
	*(--sp) = tf->ra;
	*(--sp) = fault_va;
	// push esp as an argument
	uintptr_t entry = (uintptr_t)curenv->env_pgfault_upcall;

	user_mem_assert(curenv, (void *)entry, 4, PTE_U | PTE_W | PTE_X | PTE_R);

	curenv->env_tf.sp = (uintptr_t)sp;
	curenv->env_tf.sepc= entry;

	env_run(curenv);
	// Destroy the environment that caused the fault.
bad:
	cprintf("[%08x] user fault va %08lx sepc %08lx scause %08lx\n",
		curenv->env_id, fault_va, tf->sepc, tf->scause);
	print_trapframe(tf);
	env_destroy(curenv);


}

