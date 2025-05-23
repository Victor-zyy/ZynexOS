
#include "fs.h"

// Return the virtual address of this disk block.
void*
diskaddr(uint32_t blockno)
{
	if (blockno == 0 || (super && blockno >= super->s_nblocks))
		panic("bad block number %08x in diskaddr", blockno);
	return (char*) (uintptr_t)(DISKMAP + blockno * BLKSIZE);
}

// Is this virtual address mapped?
bool
va_is_mapped(void *va)
{
  //return (uvpd[PD0X(va)] & PTE_V) && (uvpt[PGNUM(va)] & PTE_V);
	return (sys_uvpt_pte(va) & PTE_V) != 0;
}

// Is this virtual address dirty?
bool
va_is_dirty(void *va)
{
	return (sys_uvpt_pte(va) & PTE_D) != 0;
}

// Fault any disk block that is read in to memory by
// loading it from disk.
static void
bc_pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t blockno = ((uint64_t)addr - DISKMAP) / BLKSIZE;
	int r;
	// Check that the fault was within the block cache region
	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
		panic("page fault in FS: sepc %08x, va %08x, err %04x",
		      utf->utf_epc, addr, utf->utf_cause);

	// Sanity check the block number.
	if (super && blockno >= super->s_nblocks)
		panic("reading non-existent block %08x\n", blockno);

	// Allocate a page in the disk map region, read the contents
	// of the block from the disk into that page.
	// Hint: first round addr to page boundary. fs/ide.c has code to read
	// the disk.
	//
	// LAB 5: you code here:
	// Step 1. round the addr to page aligned
	addr = ROUNDDOWN(addr, BLKSIZE); //
	// Step 2. allocate one page from kernel
	if((r = sys_page_alloc(sys_getenvid(), (void *)addr, PTE_U | PTE_W | PTE_V | PTE_R)) < 0){
		panic("in bc_pgfault, sys_page_alloc: %e", r);
	}
	// Step 3. read the block from disk
	// BLKSIZE 4096 page SECSIZE 512 BLKSECTS
	//#define SECTSIZE	512			// bytes per disk sector
	//#define BLKSECTS	(BLKSIZE / SECTSIZE)	// sectors per block
	// 4096 / 512 = 8
	// blockno
	cfi_flash_read(BLKSECTS*blockno, addr, BLKSECTS);

	// Clear the dirty bit for the disk block page since we just read the
	// block from disk
	if( (r = sys_page_clear_dirty(0, addr, 0, addr) ) < 0 )
		panic("in bc_pgfault, sys_page_map: %e", r);

	// Check that the block we read was allocated. (exercise for
	// the reader: why do we do this *after* reading the block
	// in?)
	if (bitmap && block_is_free(blockno))
		panic("reading free block %08x\n", blockno);
}

// Flush the contents of the block containing VA out to disk if
// necessary, then clear the PTE_D bit using sys_page_map.
// If the block is not in the block cache or is not dirty, does
// nothing.
// Hint: Use va_is_mapped, va_is_dirty, and ide_write.
// Hint: Use the PTE_SYSCALL constant when calling sys_page_map.
// Hint: Don't forget to round addr down.
void
flush_block(void *addr)
{
	uint32_t blockno = ((uint64_t)addr - DISKMAP) / BLKSIZE;
	int r;

	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
		panic("flush_block of bad va %08x", addr);
	// LAB 5: Your code here.
	// Step 1. Rounddown the page addr to PGSIZE
	addr = ROUNDDOWN(addr, BLKSIZE);
	// Step 2. do dome necessary check
	if(!va_is_mapped(addr)){
	        //cprintf("flush unmap va \n");
		return;
	}
	if(!va_is_dirty(addr)){
	        //cprintf("flush undirty va \n");
		return;
	}
	// Step 3. flush out
	cfi_flash_write(BLKSECTS * blockno, addr, BLKSECTS);
	// Step 4. clear PTE_D bit
	// Clear the dirty bit for the disk block page since we just read the
	// block from disk
	//if ((r = sys_page_map(0, addr, 0, addr, uvpt[PGNUM(addr)] & PTE_SYSCALL)) < 0)
	//	panic("in bc_pgfault, sys_page_map: %e", r);
	if( (r = sys_page_clear_dirty(0, addr, 0, addr) ) < 0 )
		panic("in bc_pgfault, sys_page_map: %e", r);

	return;

	panic("flush_block not implemented");
}

// Test that the block cache works, by smashing the superblock and
// reading it back.
static void
check_bc(void)
{
	struct Super backup;

	// back up super block
	memmove(&backup, diskaddr(1), sizeof backup);

	// smash it
	strcpy(diskaddr(1), "OOPS!\n");
	flush_block(diskaddr(1));
	assert(va_is_mapped(diskaddr(1)));
	assert(!va_is_dirty(diskaddr(1)));

	// clear it out
	sys_page_unmap(0, diskaddr(1));
	assert(!va_is_mapped(diskaddr(1)));

	// read it back in
	assert(strcmp(diskaddr(1), "OOPS!\n") == 0);

	// fix it
	memmove(diskaddr(1), &backup, sizeof backup);
	flush_block(diskaddr(1));

	// Now repeat the same experiment, but pass an unaligned address to
	// flush_block.

	// back up super block
	memmove(&backup, diskaddr(1), sizeof backup);

	// smash it
	strcpy(diskaddr(1), "OOPS!\n");

	// Pass an unaligned address to flush_block.
	flush_block(diskaddr(1) + 20);
	assert(va_is_mapped(diskaddr(1)));

	// Skip the !va_is_dirty() check because it makes the bug somewhat
	// obscure and hence harder to debug.
	//assert(!va_is_dirty(diskaddr(1)));

	// clear it out
	sys_page_unmap(0, diskaddr(1));
	assert(!va_is_mapped(diskaddr(1)));

	// read it back in
	assert(strcmp(diskaddr(1), "OOPS!\n") == 0);

	// fix it
	memmove(diskaddr(1), &backup, sizeof backup);
	flush_block(diskaddr(1));

	cprintf("block cache is good\n");
}

void
bc_init(void)
{
	struct Super super;
	set_pgfault_handler(bc_pgfault);
	check_bc();

	// cache the super block by reading it once
	memmove(&super, diskaddr(1), sizeof super);
}

