// Search for and parse the multiprocessor configuration table
// See http://developer.intel.com/design/pentium/datashts/24201606.pdf

#include <inc/riscv/types.h>
#include <inc/riscv/string.h>
#include <inc/riscv/memlayout.h>
#include <inc/riscv/riscv.h>
#include <inc/riscv/mmu.h>
#include <inc/riscv/env.h>
#include <kern/riscv/cpu.h>
#include <kern/riscv/pmap.h>

struct CpuInfo cpus[NCPU];
struct CpuInfo *bootcpu;
int ismp;
int ncpu;
uint8_t cpu_mask;

// Per-CPU kernel stacks
unsigned char percpu_kstacks[NCPU][KSTKSIZE]
__attribute__ ((aligned(PGSIZE)));

static uint8_t
sum(void *addr, int len)
{
	int i, sum;

	sum = 0;
	for (i = 0; i < len; i++)
		sum += ((uint8_t *)addr)[i];
	return sum;
}

void
mp_init(unsigned long hartid)
{
  // initialize the cpu_mask cpus bit
  cpu_mask = 0;
  // use sbi_ecall to get the total number of cpus
  // the opensbi firmware can parse the fdt tree to
  // cal the counts
  ncpu = sbi_hartcount();
  // Set flag bit when the ncpu is greater than 1
  if(ncpu > 1)
    ismp = 1;
  // set the cpu_mask bit
  // cpu0 1
  // cpu0 cpu1 00000011  -> 0x3
  // cpu0 cpu1 cpu2 cpu3 -> 00001111 0xf
  switch(ncpu){
    case 1: cpu_mask = 1; break;
    case 2: cpu_mask = 3; break;
    case 3: cpu_mask = 7; break;
    case 4: cpu_mask = 15; break;
    case 5: cpu_mask = 31; break;
    case 6: cpu_mask = 63; break;
    case 7: cpu_mask = 127; break;
    case 8: cpu_mask = 255; break;
    default: cpu_mask = 1; break;
  }

  // do some necessary check
  assert(hartid <= ncpu);
  // assign the bsp cpuinfo
  bootcpu = &cpus[hartid];
}
