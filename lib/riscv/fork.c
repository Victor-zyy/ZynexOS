// implement fork from user space

#include <inc/riscv/string.h>
#include <inc/riscv/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW	0x300	
#define debug 0
//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	if(debug)
	  cprintf("envid 0x%x pgfault cause = 0x%x epc = 0x%08x va : 0x%08lx\n", sys_getenvid(), utf->utf_cause, utf->utf_epc, addr);
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).
	// cprintf("pgfault addr : 0x%08lx \n", addr);
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//  align the address of page-fault
	addr = ROUNDDOWN(addr, PGSIZE);	
	// LAB 4: Your code here.
	if((r = sys_page_alloc(sys_getenvid(), UTEMP, PTE_V | PTE_U | PTE_W | PTE_X | PTE_R )) < 0){
		panic("sys_page_alloc: %e", r);
	}

	memmove(UTEMP, addr, PGSIZE);
	
	if((r = sys_page_map(sys_getenvid(), UTEMP, sys_getenvid(), addr, PTE_X | PTE_V | PTE_U | PTE_W | PTE_R)) < 0){
		panic("sys_page_map: %e", r);
	}

	if((r = sys_page_unmap(sys_getenvid(), UTEMP))< 0){
		panic("sys_page_unmap: %e", r);
	}
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, uint64_t pn)
{
	int r;
	int pte;
	// LAB 4: Your code here.
	// Step 1. map the page copy-on-write
	r = sys_page_map(sys_getenvid(), (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), PTE_COW | PTE_U | PTE_R | PTE_X | PTE_V);
	if( r < 0 ){
		panic("sys_page_map: %e", r);
	}
	// Step 2. remap the page copy-on-write
	// how to remap
	// cprintf("pn * PGSIZE = 0x%08x\n", pn * PGSIZE);

	r = sys_page_map(sys_getenvid(), (void *)(pn * PGSIZE), sys_getenvid(), (void *)(pn * PGSIZE), PTE_COW | PTE_U | PTE_R | PTE_X | PTE_V);

	if( r < 0 ){
		panic("sys_page_map: %e", r);
	}

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	int r;
	int pn = 0;
	uint64_t j = 0;
	int flag = 0;
	uint8_t *addr;
	extern unsigned char end[];

	// Step 1. install pgfault() handler function
	set_pgfault_handler(pgfault);

	// Step 2. call sys_exofork to create a child environment
	envid_t envid = sys_exofork();
	//cprintf("sys_getenvid : 0x%x  envid : 0x%x \n", sys_getenvid(), envid);
	if(envid < 0)
		panic("sys_exofork: %e", envid);
	if(envid == 0){
		// child process
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	// parent process 
	// Step 3.for each writable or copy-on-write page in its address space
	cprintf("address end 0x%08x\n", end);
	for(addr = (uint8_t*) UTEXT; addr < end; addr += PGSIZE){
	  j = (uint64_t)addr / PGSIZE;
	  duppage(envid, j);	
	}
	//
	// duppage for USTACKTOP
	j = (USTACKTOP-PGSIZE) / PGSIZE;
	duppage(envid, j);

	// duppage for file fd mapping
	// FILEDATA_TABLE
	if((r = sys_copy_shared_pages(envid)) < 0)
		panic("sys_copy_shared_pages: %e", r);

	// Step 4. allocate a new page for child exception stack
	if((r = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_W | PTE_U | PTE_R | PTE_V)) < 0)
		panic("sys_page_alloc: %e", r);

	//cprintf("fork ok\n");
	// Step 5. start the child environment running
	if((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);	

	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
