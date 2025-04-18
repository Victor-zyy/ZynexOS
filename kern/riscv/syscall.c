/* See COPYRIGHT for copyright information. */

#include <inc/riscv/riscv.h>
#include <inc/riscv/error.h>
#include <inc/riscv/string.h>
#include <inc/riscv/assert.h>

#include <kern/riscv/env.h>
#include <kern/riscv/pmap.h>
#include <kern/riscv/trap.h>
#include <kern/riscv/syscall.h>
#include <kern/riscv/console.h>

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 3: Your code here.
	user_mem_assert(curenv, (void *)s, len, PTE_U);

	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if (e == curenv)
		cprintf("[%08x] exiting gracefully\n", curenv->env_id);
	else
		cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint64_t syscallno, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.
	//panic("syscall not implemented");
	int32_t ret = 0;	
	switch (syscallno) {
	case SYS_cputs: sys_cputs((char *)a1, a2); ret = 0; break;
	case SYS_cgetc: ret = sys_cgetc(); break;
	case SYS_getenvid: ret = sys_getenvid(); break;
	case SYS_env_destroy: ret = sys_env_destroy(a1); break;
				
	default:
		return -E_INVAL;
	}
	return ret;
}

