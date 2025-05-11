#include <inc/riscv/assert.h>
#include <inc/riscv/riscv.h>
#include <kern/riscv/spinlock.h>
#include <kern/riscv/env.h>
#include <kern/riscv/pmap.h>
#include <kern/riscv/monitor.h>
#include <kern/riscv/cpu.h>
#include <kern/riscv/clint.h>

#define debug 0
void sched_halt(void);

// Choose a user environment to run and run it.
void
sched_yield(bool time)
{
	struct Env *idle;
	if(debug)
	  cprintf("sched_yield times %d next env_i ", time);
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.
	//
	// LAB 4: Your code here.
	//
	int i = 0;
	if(curenv == NULL){
		//find RUNNABLE env to run
		for(i = 0; i < NENV; i++) if(envs[i].env_status == ENV_RUNNABLE) break;
		if( i == NENV)// no RUNNABLE ENV sch
			sched_halt();
		idle = &envs[i];
		if(debug)
		  cprintf(" %d sepc 0x%08x\n", i, idle->env_tf.sepc);
		env_run(idle);
	}

	// Round-Robin Scheduler 
	int run = curenv->env_status;
	int curid = curenv - envs;
	// search right after the previously running environment
	for(i = curid ; i < NENV; i++){
		if(envs[i].env_status == ENV_RUNNABLE) break;
	}
	
	assert(i >= 0);

	if(i == NENV){
		// not find runnable right after curenv
		// then research at the beginging of the envs[]
		for(i = 0; i < curid; i++){
			if(envs[i].env_status == ENV_RUNNABLE) break;
		}
		if(i == curid){
			// not found one
			if(run == ENV_RUNNING){
				idle = &envs[curid];
				//cprintf(" %d sepc 0x%08x\n", i, idle->env_tf.sepc);
				env_run(idle);
			}
		}else{
			//found one
			//
			idle = &envs[i];
			if(debug)
			  cprintf(" %d sepc 0x%08x\n", i, idle->env_tf.sepc);
			env_run(idle);
		}
		sched_halt();
	}else{
		//find one do dome change
		idle = &envs[i];
		if(debug)
		  cprintf(" %d sepc 0x%08x\n", i, idle->env_tf.sepc);
		env_run(idle);
	}
	// sched_halt never returns
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	load_satp_asid(PADDR(kern_pgdir), 0);

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	atomic_raw_xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	//while(1);

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile (
		"mv fp, zero\n"		
		"mv sp, %0\n" \
		// Uncomment the following line after completing exercise 13
		"csrw  sscratch, zero\n"\
		"csrr  t0, sstatus\n"\
		"or    t0, t0, 0x2\n"
		"csrw  sstatus, t0\n"\
		"li    t0, 0x20\n"\
		"csrw  sie, t0\n" \
		"1:\n" \
		"nop\n" \
		"j 1b\n" \
	: : "r" (thiscpu->kern_sp));
}

