#ifndef _INC_TRAP_H
#define _INC_TRAP_H

// Trap numbers
// These are processor defined:
#define T_NALIGN     0		// instruction address not aligned exception
#define T_ACCESS     1		// instruction access exception
#define T_INVALID    2		// instruction invalid exception
#define T_BRKPT      3		// breakpoint exception
#define T_RSVE       4		// reserve no use
#define T_LACCESS    5		// load access exception
#define T_AMONALIGN  6		// AMO not aligned exception
#define T_AMOACCESS  7		// AMO access exception
#define T_DEVICE     7		// device not available

#define T_SYSCALL    8		// system call
//#define T_RSVE     9		// reserve no use
//#define T_RSVE     10		// reserve no use
//#define T_RSVE     11		// reserve no use

#define T_IPGFLT     12		// instruction page fault
#define T_LPGFLT     13		// load page fault
//#define T_RSVE     14		// reserve no use
#define T_SAMOPGFLT  15		// store/AMO page fault

#define TRAP_MASK    
// Hardware IRQ numbers. We receive these as (IRQ_OFFSET+IRQ_WHATEVER)
#define IRQ_TIMER        5
#define IRQ_KBD          1
#define IRQ_SERIAL       4
#define IRQ_SPURIOUS     7
#define IRQ_IDE         14
#define IRQ_ERROR       19


#include <inc/riscv/asm-offset.h>

#ifndef __ASSEMBLER__

#include <inc/riscv/types.h>

void _alltraps_ret(void)
__attribute__((noreturn));

struct PushRegs {


};
struct Trapframe {
        unsigned long sepc;
        unsigned long ra;
        unsigned long sp;
        unsigned long gp;
        unsigned long tp;
        unsigned long t0;
        unsigned long t1;
        unsigned long t2;
        unsigned long s0;
        unsigned long s1;
        unsigned long a0;
        unsigned long a1;
        unsigned long a2;
        unsigned long a3;
        unsigned long a4;
        unsigned long a5;
        unsigned long a6;
        unsigned long a7;
        unsigned long s2;
        unsigned long s3;
        unsigned long s4;
        unsigned long s5;
        unsigned long s6;
        unsigned long s7;
        unsigned long s8;
        unsigned long s9;
        unsigned long s10;
        unsigned long s11;
        unsigned long t3;
        unsigned long t4;
        unsigned long t5;
        unsigned long t6;
        /* Supervisor/Machine CSRs */
        unsigned long status;
        unsigned long stval;
        unsigned long scause;
        /* a0 value before the syscall */
        unsigned long orig_a0;
}__attribute__((packed));

struct SavedRegs {
        unsigned long ra;
        unsigned long sp;
        unsigned long gp;
        unsigned long tp;
        unsigned long t0;
        unsigned long t1;
        unsigned long t2;
        unsigned long s0;
        unsigned long s1;
        unsigned long a0;
        unsigned long a1;
        unsigned long a2;
        unsigned long a3;
        unsigned long a4;
        unsigned long a5;
        unsigned long a6;
        unsigned long a7;
        unsigned long s2;
        unsigned long s3;
        unsigned long s4;
        unsigned long s5;
        unsigned long s6;
        unsigned long s7;
        unsigned long s8;
        unsigned long s9;
        unsigned long s10;
        unsigned long s11;
        unsigned long t3;
        unsigned long t4;
        unsigned long t5;
        unsigned long t6;
} __attribute__((packed));

struct UTrapframe {
	/* information about the fault */
	uint64_t utf_fault_va;	/* va for T_PGFLT, 0 otherwise */
	/* trap-time return state */
	struct SavedRegs utf_regs;
	uint64_t utf_cause;
	uintptr_t utf_epc;
	/* the trap-time stack to return to */
	uintptr_t utf_sp;
} __attribute__((packed));

#endif /* !__ASSEMBLER__ */

#endif /* !JOS_INC_TRAP_H */
