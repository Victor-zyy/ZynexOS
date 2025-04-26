/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_TRAP_H
#define JOS_KERN_TRAP_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/riscv/trap.h>
#include <inc/riscv/mmu.h>

/* The kernel's interrupt descriptor table */

void trap_init(void);
void trap_init_percpu(void);
void print_regs(struct Trapframe *regs);
void print_trapframe(struct Trapframe *tf);
void page_fault_handler(struct Trapframe *);
void backtrace(struct Trapframe *);

#endif /* JOS_KERN_TRAP_H */
