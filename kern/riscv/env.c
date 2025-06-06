/* See COPYRIGHT for copyright information. */

#include <inc/riscv/riscv.h>
#include <inc/riscv/mmu.h>
#include <inc/riscv/error.h>
#include <inc/riscv/string.h>
#include <inc/riscv/assert.h>
#include <inc/riscv/elf.h>
#include <inc/riscv/trap.h>

#include <kern/riscv/env.h>
#include <kern/riscv/pmap.h>
#include <kern/riscv/trap.h>
#include <kern/riscv/monitor.h>
#include <kern/riscv/cpu.h>
#include <kern/riscv/spinlock.h>
#include <kern/riscv/sched.h>

struct Env *envs = NULL;		// All environments
static struct Env *env_free_list;	// Free environment list
					// (linked by Env->env_link)

#define ENVGENSHIFT	12		// >= LOGNENV


//
// Converts an envid to an env pointer.
// If checkperm is set, the specified environment must be either the
// current environment or an immediate child of the current environment.
//
// RETURNS
//   0 on success, -E_BAD_ENV on error.
//   On success, sets *env_store to the environment.
//   On error, sets *env_store to NULL.
//
int
envid2env(envid_t envid, struct Env **env_store, bool checkperm)
{
	struct Env *e;

	// If envid is zero, return the current environment.
	if (envid == 0) {
		*env_store = curenv;
		return 0;
	}

	// Look up the Env structure via the index part of the envid,
	// then check the env_id field in that struct Env
	// to ensure that the envid is not stale
	// (i.e., does not refer to a _previous_ environment
	// that used the same slot in the envs[] array).
	e = &envs[ENVX(envid)];
	if (e->env_status == ENV_FREE || e->env_id != envid) {
		*env_store = 0;
		return -E_BAD_ENV;
	}

	// Check that the calling environment has legitimate permission
	// to manipulate the specified environment.
	// If checkperm is set, the specified environment
	// must be either the current environment
	// or an immediate child of the current environment.
	if (checkperm && e != curenv && e->env_parent_id != curenv->env_id) {
		*env_store = 0;
		return -E_BAD_ENV;
	}

	*env_store = e;
	return 0;
}

// Mark all environments in 'envs' as free, set their env_ids to 0,
// and insert them into the env_free_list.
// Make sure the environments are in the free list in the same order
// they are in the envs array (i.e., so that the first call to
// env_alloc() returns envs[0]).
//
void
env_init(void)
{
	// Set up envs array
	// LAB 3: Your code here.
	int i;
	struct Env *tail;
	for(i = 0; i < NENV ; i++){
		// set env_id member is 0
		envs[i].env_id = 0;
		// head insert to env_free_list
		if(i != 0) { 
			tail->env_link = (struct Env*)&envs[i];
			tail = (struct Env*)&envs[i];
		}else{// first time for speed branch 
			envs[i].env_link = env_free_list;
			env_free_list = (struct Env*)&envs[i];
			tail = env_free_list;
		}
	}	
	// Per-CPU part of the initialization
	env_init_percpu();
}

// Load GDT and segment descriptors.
// Riscv Arch  use tp as CpuInfo data /* FIXME: when the context is switched the tp is reset to zero */
void
env_init_percpu(void)
{
  set_status_sum(true);
}

//
// Initialize the kernel virtual memory layout for environment e.
// Allocate a page directory, set e->env_pgdir accordingly,
// and initialize the kernel portion of the new environment's address space.
// Do NOT (yet) map anything into the user portion
// of the environment's virtual address space.
//
// Returns 0 on success, < 0 on error.  Errors include:
//	-E_NO_MEM if page directory or table could not be allocated.
//
static int
env_setup_vm(struct Env *e)
{
	int i;
	struct PageInfo *p = NULL;

	// Allocate a page for the page directory
	if (!(p = page_alloc(ALLOC_ZERO)))
		return -E_NO_MEM;

	// Now, set e->env_pgdir and initialize the page directory.
	//
	// Hint:
	//    - The VA space of all envs is identical above UTOP
	//	(except at UVPT, which we've set below).
	//	See inc/memlayout.h for permissions and layout.
	//	Can you use kern_pgdir as a template?  Hint: Yes.
	//	(Make sure you got the permissions right in Lab 2.)
	//    - The initial VA below UTOP is empty.
	//    - You do not need to make any more calls to page_alloc.
	//    - Note: In general, pp_ref is not maintained for
	//	physical pages mapped only above UTOP, but env_pgdir
	//	is an exception -- you need to increment env_pgdir's
	//	pp_ref for env_free to work correctly.
	//    - The functions in kern/pmap.h are handy.

	// LAB 3: Your code here.
	// map the pageinfo to a page memory but virtually
	e->env_pgdir = (pde_t *)page2kva(p); //must be pa becasue of the satp reg needs to find the pa of the pgdir
	// increment the pp_ref for env_free work correctly
	p->pp_ref++;
	// use kern_pgdir as a template 
	// above UTOP but set correct permissions

	i = PD0X(UPAGES);
	e->env_pgdir[i] = kern_pgdir[i];
	i = PD0X(UENVS);
	e->env_pgdir[i] = kern_pgdir[i];

	//kernel stack do what? env has one single KERNEL_STACK
	//i = PD0X(KSTACKTOP-KSTKSIZE);
	i = PD0X(MMIOBASE);
	e->env_pgdir[i] = kern_pgdir[i];
	//KERNBASE REMAP
	for(i = PD0X(KERNBASE); i < 512; i++ ){
		e->env_pgdir[i] = kern_pgdir[i];
	}
	// except for above we don't set env page
	// UVPT maps the env's own page table read-only.
	// Permissions: kernel R, user R
	e->env_pgdir[PD0X(UVPT)] = PADDR(e->env_pgdir) | PTE_V;
	/* FIXME:  */

	return 0;
}

//
// Allocates and initializes a new environment.
// On success, the new environment is stored in *newenv_store.
//
// Returns 0 on success, < 0 on failure.  Errors include:
//	-E_NO_FREE_ENV if all NENV environments are allocated
//	-E_NO_MEM on memory exhaustion
//
int
env_alloc(struct Env **newenv_store, envid_t parent_id)
{
	int32_t generation;
	int r;
	struct Env *e;

	if (!(e = env_free_list))
		return -E_NO_FREE_ENV;

	// Allocate and set up the page directory for this environment.
	if ((r = env_setup_vm(e)) < 0)
		return r;

	// Generate an env_id for this environment.
	// /* FIXME: start form 1 */
	generation = (e->env_id + (1 << ENVGENSHIFT)) & ~(NENV - 1);
	if (generation <= 0)	// Don't create a negative env_id.
		generation = 1 << ENVGENSHIFT;
	e->env_id = generation | (e - envs);

	// Set the basic status variables.
	e->env_parent_id = parent_id;
	e->env_type = ENV_TYPE_USER;
	e->env_status = ENV_RUNNABLE;
	e->env_runs = 0;

	// Clear out all the saved register state,
	// to prevent the register values
	// of a prior environment inhabiting this Env structure
	// from "leaking" into our new environment.
	memset(&e->env_tf, 0, sizeof(e->env_tf));

	// Set up appropriate initial values for the segment registers.
	// GD_UD is the user data segment selector in the GDT, and
	// GD_UT is the user text segment selector (see inc/memlayout.h).
	// The low 2 bits of each segment register contains the
	// Requestor Privilege Level (RPL); 3 means user mode.  When
	// we switch privilege levels, the hardware does various
	// checks involving the RPL and the Descriptor Privilege Level
	// (DPL) stored in the descriptors themselves.
	e->env_tf.sp = USTACKTOP;
	e->env_tf.status = read_status() | 0x20;
	// You will set e->env_tf.tf_eip later.
	// In order to handler the kernel stack, we use tp to pointed the env it self
	// When the context is switched , then the tp is pointed another env of itself
	e->env_tf.tp = (unsigned long)e; 
	
	// Clear the user page fault handler until the user installs one
	e->env_pgfault_upcall = 0;
	// also clear the IPC recving flags
	e->env_ipc_recving = 0;
	// commit the allocation
	env_free_list = e->env_link;
	*newenv_store = e;

	cprintf("[%08x] new env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
	return 0;
}

//
// Allocate len bytes of physical memory for environment env,
// and map it at virtual address va in the environment's address space.
// Does not zero or otherwise initialize the mapped pages in any way.
// Pages should be writable by user and kernel.
// Panic if any allocation attempt fails.
//
static void
region_alloc(struct Env *e, void *va, size_t len)
{
	// LAB 3: Your code here.
	// (But only if you need it for load_icode.)
	//
	// Hint: It is easier to use region_alloc if the caller can pass
	//   'va' and 'len' values that are not page-aligned.
	//   You should round va down, and round (va + len) up.
	//   (Watch out for corner-cases!)
	va = ROUNDDOWN(va, PGSIZE);
	void * end = ROUNDUP(va + len, PGSIZE);

	// tlb_flush
	for(va; va < end; va += PGSIZE){
	  local_flush_tlb_page_asid((unsigned long)va, read_asid());
		// allocate one page 
		struct PageInfo * pg_info = page_alloc(0);
		if(pg_info == NULL){
			panic("page_alloc error, please check!\n");
		}
		// convert it to physical page
		int ret = page_insert(e->env_pgdir, pg_info, (void *)va, PTE_U | PTE_W | PTE_R | PTE_X);
		if(ret < 0){
			panic("region_alloc: %e", ret);
		}
		// the same for kern_pgdir
		kern_pgdir[PD0X(va)] = e->env_pgdir[PD0X(va)];
	}
}

//
// Set up the initial program binary, stack, and processor flags
// for a user process.
// This function is ONLY called during kernel initialization,
// before running the first user-mode environment.
//
// This function loads all loadable segments from the ELF binary image
// into the environment's user memory, starting at the appropriate
// virtual addresses indicated in the ELF program header.
// At the same time it clears to zero any portions of these segments
// that are marked in the program header as being mapped
// but not actually present in the ELF file - i.e., the program's bss section.
//
// All this is very similar to what our boot loader does, except the boot
// loader also needs to read the code from disk.  Take a look at
// boot/main.c to get ideas.
//
// Finally, this function maps one page for the program's initial stack.
//
// load_icode panics if it encounters problems.
//  - How might load_icode fail?  What might be wrong with the given input?
//
static void
load_icode(struct Env *e, uint8_t *binary)
{
	// Hints:
	//  Load each program segment into virtual memory
	//  at the address specified in the ELF segment header.
	//  You should only load segments with ph->p_type == ELF_PROG_LOAD.
	//  Each segment's virtual address can be found in ph->p_va
	//  and its size in memory can be found in ph->p_memsz.
	//  The ph->p_filesz bytes from the ELF binary, starting at
	//  'binary + ph->p_offset', should be copied to virtual address
	//  ph->p_va.  Any remaining memory bytes should be cleared to zero.
	//  (The ELF header should have ph->p_filesz <= ph->p_memsz.)
	//  Use functions from the previous lab to allocate and map pages.
	//
	//  All page protection bits should be user read/write for now.
	//  ELF segments are not necessarily page-aligned, but you can
	//  assume for this function that no two segments will touch
	//  the same virtual page.
	//
	//  You may find a function like region_alloc useful.
	//
	//  Loading the segments is much simpler if you can move data
	//  directly into the virtual addresses stored in the ELF binary.
	//  So which page directory should be in force during
	//  this function?
	//
	//  You must also do something with the program's entry point,
	//  to make sure that the environment starts executing there.
	//  What?  (See env_run() and env_pop_tf() below.)

	// LAB 3: Your code here.
	struct Elf *elf_header = (struct Elf *)(binary);
	struct Proghdr *ph, *eph;

	// check for magic number
	if(elf_header->e_magic != ELF_MAGIC)
	    goto bad;
	// load each program segment (ignores ph flags)
	ph = (struct Proghdr *)((uint8_t *)elf_header + elf_header->e_phoff);
	eph = ph + elf_header->e_phnum;
	// size of program header = 32byte program header number = 3
	//cprintf("ph:0x%08x eph:0x%08x\n", ph, eph);	
	for(; ph < eph; ph++){
		// only loads segment type is ELF_PROG_LOAD
		if(ph->p_type == ELF_PROG_LOAD){
			// ph->p_filesz <= ph->p_memsz			
			if( ph->p_filesz > ph->p_memsz)
				goto bad;
			// allocate lens memory to remap
			region_alloc(e, (void *)ph->p_va, ph->p_memsz);
			// copy to virtual memory by hardware mmu
			memmove((void *)ph->p_va, (void *)(binary + ph->p_offset), ph->p_filesz);
			if(ph->p_filesz != ph->p_memsz)
				memset((void *)ph->p_va + ph->p_filesz, 0 , ph->p_memsz - ph->p_filesz);
		}
		continue;
	}
	// construct Trapframe eip to entry of ELF binary file
	e->env_tf.sepc = (elf_header->e_entry);
	// cprintf("User ip address: 0x%08x\n", e->env_tf.tf_eip);
	// Now map one page for the program's initial stack
	// at virtual address USTACKTOP - PGSIZE.
	// LAB 3: Your code here.
	
	struct PageInfo *pg_info = page_alloc(0);
	//local_flush_tlb_page_asid((unsigned long)va, read_asid());
	page_insert(e->env_pgdir, pg_info, (void *)(USTACKTOP-PGSIZE), PTE_U | PTE_W | PTE_R);
	
	return ;

bad:
	// free memory and panic
	panic("bad in load_inode\n");
}

//
// Allocates a new env with env_alloc, loads the named elf
// binary into it with load_icode, and sets its env_type.
// This function is ONLY called during kernel initialization,
// before running the first user-mode environment.
// The new env's parent ID is set to 0.
//
void
env_create(uint8_t *binary, enum EnvType type)
{
	// LAB 3: Your code here.
	// 1.allocate a new env
	int parent_id = 0;
	struct Env *env = NULL;
	int ret = env_alloc(&env, parent_id);
	if(ret < 0 ){
		panic("env_alloc: %e", ret);
	}
	// 2.load binary into it
	set_status_sum(true);
	load_icode(env, binary);
	// 3.set its env_type
	env->env_type = type;

	if(type == ENV_TYPE_FS){
	  env->env_pgdir[PD0X(FLASH_MAP_ADDR)] = kern_pgdir[PD0X(FLASH_MAP_ADDR)];
	}
}

//
// Frees env e and all memory it uses.
//
void
env_free(struct Env *e)
{
	pte_t *pt;
	pte_t *pd0;
	pte_t *pd1;
	pte_t *pd2;
	uint64_t pdeno, pteno;
	uint64_t pde1no, pde2no;
	physaddr_t pa, pa1, pa2;

	// If freeing the current environment, switch to kern_pgdir
	// before freeing the page directory, just in case the page
	// gets reused.
	if (e == curenv)
	  load_satp_asid(PADDR(kern_pgdir), 0);

	// Note the environment's demise.
	cprintf("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, e->env_id);

	// Flush all mapped pages in the user portion of the address space
	static_assert(UTOP % PTSIZE == 0);
	// you have to free the 4-level pages recrusively
	// only look at mapped page tables
	// PD0X is for the user definitly

	pdeno = 0;
	if (e->env_pgdir[pdeno] & PTE_V){
	  // find the pa and va of the page table
	  pa = PTE_ADDR(e->env_pgdir[pdeno]);
	  pd1 = (pde_t*) KADDR(pa);

	  for(pde1no = 0; pde1no <= PD1X(~0); pde1no++){

	    // only look at mapped page tables
	    if (!(pd1[pde1no] & PTE_V))
		continue;

	    pa1   = PTE_ADDR(pd1[pde1no]);
	    pd2   = (pde_t*) KADDR(pa1);

	    for(pde2no = 0; pde2no <= PD2X(~0); pde2no++){

		// only look at mapped page tables
		if (!(pd2[pde2no] & PTE_V))
		    continue;


		pa2 = PTE_ADDR(pd2[pde2no]);
		pt  = (pde_t*) KADDR(pa2);

		// unmap all PTEs in this page table
		for (pteno = 0; pteno <= PTX(~0); pteno++) {
		    if (pt[pteno] & PTE_V)
		      page_remove(e->env_pgdir, PGADDR(pdeno, pde1no, pde2no, pteno, 0));
		}
		
		// free the page table itself
		pd2[pde2no] = 0;
		page_decref(pa2page(pa2));

	    }

	    // free the page table itself
	    pd1[pde1no] = 0;
	    page_decref(pa2page(pa1));

	  }

	// free the page table itself
	e->env_pgdir[pdeno] = 0;
	page_decref(pa2page(pa));
     }

    // free the page directory
    pa = PADDR(e->env_pgdir);
    e->env_pgdir = 0;
    page_decref(pa2page(pa));

    // return the environment to the free list
    e->env_status = ENV_FREE;
    e->env_link = env_free_list;
    env_free_list = e;
}

//
// Frees environment e.
// If e was the current env, then runs a new environment (and does not return
// to the caller).
//
void
env_destroy(struct Env *e)
{
        // If e is currently running on other CPUs, we change its state to
        // ENV_DYING. A zombie environment will be freed the next time
        // it traps to the kernel.
        if (e->env_status == ENV_RUNNING && curenv != e) {
                e->env_status = ENV_DYING;
                return;
        }

        env_free(e);

        if (curenv == e) {
                curenv = NULL;
                sched_yield(0);
        }
}


//
// Restores the register values in the Trapframe with the 'iret' instruction.
// This exits the kernel and starts executing some environment's code.
//
// This function does not return.
//
void
env_pop_tf(struct Trapframe *tf)
{
  
	// Record the CPU we are running on for user-space debugging
	curenv->env_cpunum = cpunum();
	unlock_kernel();

	asm volatile(
		"\tmv sp, %0\n"
		: : "g" (tf) : "memory");
	
	_alltraps_ret();
	
	panic("sret failed");  /* mostly to placate the compiler */
}


//
// Context switch from curenv to env e.
// Note: if this is the first call to env_run, curenv is NULL.
//
// This function does not return.
//
void
env_run(struct Env *e)
{
	// Step 1: If this is a context switch (a new environment is running):
	//	   1. Set the current environment (if any) back to
	//	      ENV_RUNNABLE if it is ENV_RUNNING (think about
	//	      what other states it can be in),
	//	   2. Set 'curenv' to the new environment,
	//	   3. Set its status to ENV_RUNNING,
	//	   4. Update its 'env_runs' counter,
	//	   5. Use lcr3() to switch to its address space.
	// Step 2: Use env_pop_tf() to restore the environment's
	//	   registers and drop into user mode in the
	//	   environment.

	// Hint: This function loads the new environment's state from
	//	e->env_tf.  Go back through the code you wrote above
	//	and make sure you have set the relevant parts of
	//	e->env_tf to sensible values.

	// LAB 3: Your code here.
	// Step 1:
	if(curenv != NULL){
	  // a context switch  /* FIXME: fix bug of ENV_NOT_RUNNABLE -> ENV_RUNABLE */
	  if (curenv->env_status == ENV_RUNNING) {
		curenv->env_status = ENV_RUNNABLE;
	  }
	}
	curenv = e;
	curenv->env_status = ENV_RUNNING;
	curenv->env_runs ++;

	load_satp_asid(PADDR(curenv->env_pgdir), curenv->env_id & 0x3ff);
	// Step 2:
	//
	env_pop_tf(&curenv->env_tf);
	// never gonna return

	panic("env_run not yet implemented");
}
