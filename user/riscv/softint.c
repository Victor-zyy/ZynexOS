// buggy program - causes an illegal software interrupt

#include <inc/riscv/lib.h>

void
umain(int argc, char **argv)
{
	asm volatile("ebreak");	// page fault
}

