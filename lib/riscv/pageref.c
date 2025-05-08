#include <inc/riscv/lib.h>

#define PTE_PADDR(la) ((la >> 10) << 12)
int
pageref(void *v)
{
	pte_t pte;

	/* FIXME:  */
	pte = sys_uvpt_pte(v);

	// cprintf("pageref _v : 0x%08lx pte : 0x%08x\n", v, pte);

	if( (pte & PTE_V) == 0)
	  return 0;

	return pages[PGNUM(PTE_PADDR(pte))].pp_ref;
}
