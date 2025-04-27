// test evil pointer for user-level fault handler

#include <inc/riscv/lib.h>

void
umain(int argc, char **argv)
{
	sys_page_alloc(0, (void*) (UXSTACKTOP - PGSIZE), PTE_V|PTE_U|PTE_W|PTE_R);
	sys_env_set_pgfault_upcall(0, (void*) 0xF0100020);
	*(int*)0 = 0;
}
