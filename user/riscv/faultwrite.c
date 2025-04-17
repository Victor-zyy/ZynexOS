// buggy program - faults with a write to location zero

#include <inc/riscv/lib.h>

void
umain(int argc, char **argv)
{
	*(unsigned*)0x08000000 = 0;
}

