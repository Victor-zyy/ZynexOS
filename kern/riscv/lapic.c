// The local APIC manages internal (non-I/O) interrupts.
// See Chapter 8 & Appendix C of Intel processor manual volume 3.

#include <inc/riscv/types.h>
#include <inc/riscv/memlayout.h>
#include <inc/riscv/trap.h>
#include <inc/riscv/mmu.h>
#include <inc/riscv/stdio.h>
#include <inc/riscv/riscv.h>
#include <kern/riscv/pmap.h>
#include <kern/riscv/cpu.h>

// Local APIC registers, divided by 4 for use as uint32_t[] indices.
void
lapic_init(void)
{
  return;
}

