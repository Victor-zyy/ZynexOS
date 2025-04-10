/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */


#define TRAPHANDLER(name, num)						\
	.data;\
	.long name; \
	.text;\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */

#define TRAPHANDLER_NOEC(name, num)					\
	.data;\
	.long name; \
	.text;\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps	

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
.data
.global handlers
handlers:

TRAPHANDLER_NOEC(handler0, T_DIVIDE);
TRAPHANDLER_NOEC(handler1, T_DEBUG);
TRAPHANDLER_NOEC(handler2, T_NMI);
TRAPHANDLER_NOEC(handler3, T_BRKPT);
TRAPHANDLER_NOEC(handler4, T_OFLOW);
TRAPHANDLER_NOEC(handler5, T_BOUND);
TRAPHANDLER_NOEC(handler6, T_ILLOP);
TRAPHANDLER_NOEC(handler7, T_DEVICE);
TRAPHANDLER(handler8, T_DBLFLT);
TRAPHANDLER_NOEC(handler9, 9); //just for simplify 
TRAPHANDLER(handler10, T_TSS);
TRAPHANDLER(handler11, T_SEGNP);
TRAPHANDLER(handler12, T_STACK);
TRAPHANDLER(handler13, T_GPFLT);
TRAPHANDLER(handler14, T_PGFLT);
TRAPHANDLER(handler15, 15);					//just for simplify
TRAPHANDLER_NOEC(handler16, T_FPERR);
TRAPHANDLER_NOEC(handler17, T_ALIGN);
TRAPHANDLER_NOEC(handler18, T_MCHK);
TRAPHANDLER_NOEC(handler19, T_SIMDERR);

TRAPHANDLER_NOEC(handler48, T_SYSCALL);
// for device handler
TRAPHANDLER_NOEC(handler49, IRQ_OFFSET + IRQ_TIMER);
TRAPHANDLER_NOEC(handler50, IRQ_OFFSET + IRQ_KBD );
TRAPHANDLER_NOEC(handler51, IRQ_OFFSET + 2 );
TRAPHANDLER_NOEC(handler52, IRQ_OFFSET + 3);
TRAPHANDLER_NOEC(handler53, IRQ_OFFSET + IRQ_SERIAL);
TRAPHANDLER_NOEC(handler54, IRQ_OFFSET + 5);
TRAPHANDLER_NOEC(handler55, IRQ_OFFSET + 6);
TRAPHANDLER_NOEC(handler56, IRQ_OFFSET + IRQ_SPURIOUS);
TRAPHANDLER_NOEC(handler57, IRQ_OFFSET + 8);
TRAPHANDLER_NOEC(handler58, IRQ_OFFSET + 9);
TRAPHANDLER_NOEC(handler59, IRQ_OFFSET + 10);
TRAPHANDLER_NOEC(handler60, IRQ_OFFSET + 11);
TRAPHANDLER_NOEC(handler61, IRQ_OFFSET + 12);
TRAPHANDLER_NOEC(handler62, IRQ_OFFSET + 13);
TRAPHANDLER_NOEC(handler63, IRQ_OFFSET + IRQ_IDE);

/*
* generate a table for trap.c to use
*/


/*
 * Lab 3: Your code here for _alltraps
 */

_alltraps:
// push values to make the stack look like a struct frame
  pushl %ds;
	pushl %es;
	pushal;
	
// load GD_KD into %ds and %es	
	mov $0x10,%ax;
	mov %ax,  %ds;
	mov %ax,  %es;

// pushl %esp to pass a pointer as a argument
	pushl %esp;
	call trap;

// call trap ever return
_alltraps_ret:

	popal;
	popl %es;
	popl %ds;
// handler error code
	addl $0x08, %esp;

	iret;
