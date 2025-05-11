#include <inc/riscv/lib.h>

#define PTE_PADDR(la) ((la >> 10) << 12)
#define debug 0
int
pageref(void *v)
{
	pte_t pte;

	/* FIXME:  */
	pte = sys_uvpt_pte(v);

	// cprintf("pageref _v : 0x%08lx pte : 0x%08x\n", v, pte);
	if(debug){
	  if((uint64_t)v == 0xd0002000)
	    cprintf("pageref _v : 0x%08lx pte : 0x%08x real_phy : 0x%08x pgnum : %d pp_ref : %d\n",
		    v, pte, PTE_PADDR(pte), PGNUM(PTE_PADDR(pte)), pages[PGNUM(PTE_PADDR(pte))].pp_ref);
	}
	if( (pte & PTE_V) == 0)
	  return 0;

	return pages[PGNUM(PTE_PADDR(pte))].pp_ref;
}
