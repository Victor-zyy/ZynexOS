#include <inc/riscv/mmu.h>
#include <inc/riscv/memlayout.h>

.data
// Define the global symbols 'envs', 'pages', 'uvpt', and 'uvpd'
// so that they can be used in C as if they were ordinary global arrays.
	.globl envs
	.set envs, UENVS
	.globl pages
	.set pages, UPAGES
	// we do not use the uvpt in user-mode 
	/*
	.globl uvpt
	.set uvpt, UVPT
	.globl uvpd
	.set uvpd, (UVPT+(UVPT>>12)*4)
	*/


// Entrypoint - this is where the kernel (or our parent environment)
// starts us running when we are initially loaded into a new environment.
.text
.globl _start
_start:
	# nuke frame pointer ;FIXME: fp pointer
	mv	fp, zero
	// See if we were started with arguments on the stack
	li   t0, USTACKTOP
	sub  t0, t0, sp
	bnez t0, args_exist

	// If not, push dummy argc/argv arguments.
	// This happens when we are loaded by the kernel,
	// because the kernel does not know about passing arguments.
	li   a0, 0 // for argc
	li   a1, 0 // for argv

	call libmain
1:	j 1b

args_exist:
	// set up the argc and argvs
	// a0 points to argc
	// a1 points t0 argv
	ld	a0,     0(sp)
	ld	a1,     8(sp)
	addi	sp,	sp,    16

	call libmain
1:	j 1b

