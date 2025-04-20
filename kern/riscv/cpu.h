#ifndef _INC_CPU_H
#define _INC_CPU_H

#include <inc/riscv/types.h>
#include <inc/riscv/memlayout.h>
#include <inc/riscv/mmu.h>
#include <inc/riscv/env.h>

// Maximum number of CPUs
#define NCPU  8


// Values of status in struct Cpu
enum {
	CPU_UNUSED = 0,
	CPU_STARTED,
	CPU_HALTED,
};

// Per-CPU state
struct CpuInfo {
	uint8_t cpu_id;                 // Local APIC ID; index into cpus[] below
	volatile unsigned cpu_status;   // The status of the CPU
	struct Env *cpu_env;            // The currently-running environment.
  //struct Taskstate cpu_ts;        // Used by x86 to find stack for interrupt
};

// Initialized in mpconfig.c
extern struct CpuInfo cpus[NCPU];
extern int ncpu;                    // Total number of CPUs in the system
extern uint8_t cpu_mask;            // CPU bit mask only 8 bit for total number of cpus
extern struct CpuInfo *bootcpu;     // The boot-strap processor (BSP)
extern physaddr_t lapicaddr;        // Physical MMIO address of the local APIC

// Per-CPU kernel stacks
extern unsigned char percpu_kstacks[NCPU][KSTKSIZE];

#define cpunum() sbi_cpunum()
#define thiscpu (&cpus[cpunum()])

void mp_init(unsigned long hartid);
void lapic_init(void);
void lapic_startap(uint8_t apicid, uint32_t addr);
void lapic_eoi(void);
void lapic_ipi(int vector);


#endif
