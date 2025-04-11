// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if((err & FEC_WR) == 0){
		panic("user page fault not writable");
	}
	if((uvpt[(uint32_t)addr / PGSIZE] & PTE_COW) == 0){
		panic("user page fault not COW");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//  align the address of page-fault
	addr = ROUNDDOWN(addr, PGSIZE);	
	// LAB 4: Your code here.
	if((r = sys_page_alloc(sys_getenvid(), UTEMP, PTE_P | PTE_U | PTE_W)) < 0){
		panic("sys_page_alloc: %e", r);
	}

	//cprintf("err : %x addr : 0x%08x UTEMP : 0x%08x\n", err, (uint32_t)addr, UTEMP);
	memmove(UTEMP, addr, PGSIZE);
	
	if((r = sys_page_map(sys_getenvid(), UTEMP, sys_getenvid(), addr, PTE_P | PTE_U | PTE_W)) < 0){
		panic("sys_page_map: %e", r);
	}

	if((r = sys_page_unmap(sys_getenvid(), UTEMP))< 0){
		panic("sys_page_unmap: %e", r);
	}

	//panic("pgfault not implemented");
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
duppage(envid_t envid, unsigned pn)
{
	int r = 0;
	int perm;
	perm = uvpt[pn] & PTE_SYSCALL;
	// LAB 4: Your code here.
	if(uvpt[pn] & PTE_SHARE){
		r = sys_page_map(sys_getenvid(), (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), PTE_SHARE | PTE_U | perm);
		if(r < 0)
			goto error;
		return 0;
	}

	if((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW)){
		// Step 1. map the page copy-on-write
		r = sys_page_map(sys_getenvid(), (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), PTE_COW | PTE_U);
		if(r < 0)
			goto error;
		// Step 2. remap the page copy-on-write
		r = sys_page_map(sys_getenvid(), (void *)(pn * PGSIZE), sys_getenvid(), (void *)(pn * PGSIZE), PTE_COW | PTE_U);
		if(r < 0)
			goto error;
		return 0;
	}
	// how to remap
	//
	// read-only page just map
	if(uvpt[pn] & PTE_P){
		r = sys_page_map(sys_getenvid(), (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), PTE_U | perm);
		if(r < 0)
			goto error;
		return 0;
	}

error:
	panic("sys_page_map: %e", r);
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
	int r = 0;
	int pn = 0;
	int j = 0;
	int flag = 0;
	//
	// Step 1. install pgfault() handler function
	set_pgfault_handler(pgfault);
	// Step 2. call sys_exofork to create a child environment
	envid_t envid = sys_exofork();
	if(envid < 0)
		panic("sys_exofork: %e", envid);
	if(envid == 0){
		// child process
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	// parent process 
	// Step 3.for each writable or copy-on-write page in its address space
	// below UTOP, the parent calls duppage
	// search using uvpt and uvpd
	for(pn = 0; pn < 1024; pn++){
		if(uvpd[pn] & PTE_P){
			// page table exists then search ptes 
			for(j = 1024 * pn; j < 1024 * (pn + 1); j++){
				// for each writable or copy-con-write below UTOP
				if(j * PGSIZE >= USTACKTOP){
					flag = 1;
					break;
				}
				// just use one function to eliminate the complexity of fork
				if(uvpt[j] & PTE_P){
					duppage(envid, j);	
				}
			}
		}	
		if(flag){
			break;
		}
	}
	//
	// Step 4. allocate a new page for child exception stack
	if((r = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_W | PTE_U)) < 0)
		panic("sys_page_alloc: %e", r);
	// Step 5. start the child environment running
	if((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);	

	// cprintf("fork ok\n");
	return envid;
	panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
