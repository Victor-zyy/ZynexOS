// buggy program - faults with a write to a kernel location

#include <inc/riscv/lib.h>

void
umain(int argc, char **argv)
{
	*(unsigned*)0xfffffffff0000000 = 0;
}

