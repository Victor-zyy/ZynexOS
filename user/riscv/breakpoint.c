// program to cause a breakpoint trap

#include <inc/riscv/lib.h>

void
umain(int argc, char **argv)
{
	asm volatile("ebreak");
}

