/* See COPYRIGHT for copyright information. */

#include <inc/riscv/stdio.h>
#include <inc/riscv/string.h>
#include <inc/riscv/assert.h>
#include <kern/riscv/console.h>
#include <kern/riscv/monitor.h>
#include <inc/riscv/string.h>
#include <inc/riscv/riscv.h>
#include <inc/riscv/sbi.h>
#include <kern/riscv/pmap.h>
#include <kern/riscv/env.h>
#include <kern/riscv/trap.h>
#include <inc/riscv/env.h>

#include <kern/riscv/cpu.h>
#include <kern/riscv/clint.h>
#include <kern/riscv/spinlock.h>
#include <kern/riscv/sched.h>
#include <kern/riscv/time.h>
#include <kern/riscv/pci.h>

static void boot_aps(void);

#define BANNER						     \
     "   ______                            _____  _____  \n" \
     "  |___  /                           |  _  |/  ___| \n" \
     "     / /  _   _  _ __    ___ __  __ | | | |\\ `--.  \n" \
     "    / /  | | | || '_ \\  / _ \\\\ \\/ / | | | | `--. \\ \n" \
     "  ./ /___| |_| || | | ||  __/ >  <  \\ \\_/ //\\__/ / \n" \
     "  \\_____/ \\__, ||_| |_| \\___|/_/\\_\\  \\___/ \\____/  \n" \
     "          __/ |                                    \n" \
     "          |___/                              \n\n"


void
riscv_init(unsigned int hartid)
{


	extern char edata[], end[];

	// Before doing anything else, complete the ELF loading process.
	// Clear the uninitialized global data (BSS) section of our program.
	// This ensures that all static/global variables start out zero.
	memset(edata, 0, end - edata);

	// Initialize the console.
	// Can't call cprintf until after we do this!
	cons_init();
	//cprintf("6828 decimal is %o octal!\n", 6828);


	cprintf("Hello, ZynexOS!\n");
	cprintf(BANNER);

	// memory management
	mem_init();

	cprintf("BSP boothart id : %d\n", hartid);
	// environment
	env_init();
	trap_init();

	// SMP configuration
	mp_init(hartid);
	lapic_init();

	// pci and time init
	time_init();
	pci_init();
	

	// lock kernel
	lock_kernel();

	// After lock the kernel, now we strap the aps
	boot_aps();

        // Start fs.
        ENV_CREATE(fs_fs, ENV_TYPE_FS);

#if !defined(TEST_NO_NS)
	// Start ns.
	ENV_CREATE(net_ns, ENV_TYPE_NS);
#endif

#if defined (TEST)
	// Don't touch !
	ENV_CREATE(TEST, ENV_TYPE_USER);
#else
	//ENV_CREATE(user_dumbfork, ENV_TYPE_USER); /* FIXME: envid 0 */
	//ENV_CREATE(user_yield, ENV_TYPE_USER); /* FIXME: envid 0 */
	//ENV_CREATE(user_hello, ENV_TYPE_USER); /* FIXME: envid 0 */
	//ENV_CREATE(user_yield, ENV_TYPE_USER); /* FIXME: envid 0 */
	//ENV_CREATE(user_spawnhello, ENV_TYPE_USER); 
	//ENV_CREATE(user_spawnhello, ENV_TYPE_USER); 
	//ENV_CREATE(user_spawnhello, ENV_TYPE_USER); 
	//ENV_CREATE(user_spawnhello, ENV_TYPE_USER); 
#endif

	// enable timer interrupts and etc.
	clint_init();

	sched_yield(0);
}

// While boot_aps is booting a given CPU, it communicates the per-core
// stack pointer that should be loaded by mpentry.S to that CPU in
// this variable.
void *mpentry_kstack;

// Start the non-boot (AP) processors.
static void
boot_aps(void)
{
	extern unsigned char mpentry_start[], mpentry_end[];
	void *code;
	struct CpuInfo *c;
	uint8_t i = 0;
	uint8_t ret = 0;

	if(1 == ncpu)
	  return;
	// Write entry code to unused memory at MPENTRY_PADDR
	code = KADDR(MPENTRY_PADDR);
	memmove(code, mpentry_start, mpentry_end - mpentry_start);

	// Boot each AP one at a time
	ret = cpu_mask;
	c   = cpus;
	for (i = 0; i < 8; i++){
	  if(ret & 0x1){
	    c->cpu_id = i;
	    // Tell mpentry.S what stack to use 
	    mpentry_kstack = percpu_kstacks[i] + KSTKSIZE;
	    sbi_boot_ap(c->cpu_id, MPENTRY_PADDR, 1); 
	    // Wait for the CPU to finish some basic setup in mp_main()
	    while(c->cpu_status != CPU_STARTED)
		    ;
	  }
	  c++;
	  ret = ret >> 1;
	}
}

// Setup code for APs
void
mp_main(void)
{
	// We are in high EIP now, safe to switch to kern_pgdir 
        load_satp(PADDR(kern_pgdir));
	cprintf("SMP: CPU %d starting\n", cpunum());

	lapic_init();
	clint_init();
	env_init_percpu();
	trap_init_percpu();
	atomic_raw_xchg(&thiscpu->cpu_status, CPU_STARTED);

	// Now that we have finished some basic setup, call sched_yield()
	// to start running processes on this CPU.  But make sure that
	// only one CPU can enter the scheduler at a time!
	//
	// Your code here:
	lock_kernel();

	sched_yield(0);

	// Remove this after you finish Exercise 6
	// for (;;);

}


/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
const char *panicstr ;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then enters the kernel monitor.
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	if (panicstr)
		goto dead;
	panicstr = fmt;

	va_start(ap, fmt);
	cprintf("kernel panic on CPU %d at %s:%d: ", cpunum(), file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);

dead:
	/* break into the kernel monitor */
	while (1)
		monitor(NULL);
}

/* like panic, but don't */
void
_warn(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	cprintf("kernel warning at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);
}
