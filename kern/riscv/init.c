/* See COPYRIGHT for copyright information. */

#include <riscv/stdio.h>
#include <riscv/string.h>
#include <riscv/assert.h>
#include <riscv/console.h>
#include <riscv/string.h>

void
riscv_init(void)
{

	extern char edata[], end[];

	// Before doing anything else, complete the ELF loading process.
	// Clear the uninitialized global data (BSS) section of our program.
	// This ensures that all static/global variables start out zero.
	memset(edata, 0, end - edata);

	// Initialize the console.
	// Can't call cprintf until after we do this!
	cons_init();

	cprintf("6828 decimal is %o octal!\n", 6828);
	cprintf("Hello, ZynexOS!\n");
	while(1);
}

