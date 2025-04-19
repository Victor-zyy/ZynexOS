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

	// environment
	env_init();
	trap_init();

#if defined (TEST)
	// Don't touch !
	ENV_CREATE(TEST, ENV_TYPE_USER);
#else
	ENV_CREATE(user_hello, ENV_TYPE_USER);
	//ENV_CREATE(user_divzero, ENV_TYPE_USER);
#endif

	env_run(&envs[0]);

	
	while(1)
	  monitor(NULL);
}



/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
const char *panicstr;

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
