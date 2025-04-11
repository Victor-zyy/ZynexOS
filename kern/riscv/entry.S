/* See COPYRIGHT for copyright information. */

#include <riscv/mmu.h>
#include <riscv/csr.h>
#include <riscv/memlayout.h>
#include <riscv/trap.h>

# Shift Right Logical 
#define SRL(val, shamt)		(((val) >> (shamt)) & ~(-1 << (32 - (shamt))))


###################################################################
# The kernel (this code) is linked at address ~(KERNBASE +  0x200000Meg), 
# but the bootloader loads it at address ~0x80200000 Meg.
#	
# RELOC(x) maps a symbol x from its link address to its actual
# location in physical memory (its load address).	 
###################################################################

#define	RELOC(x) ((x) - KERNBASE)


###################################################################
# entry point
###################################################################

.text


# '_start' specifies the ELF entry point.  Since we haven't set up
# virtual memory when the bootloader enters this code, we need the
# bootloader to jump to the *physical* address of the entry point.
.globl		_start
_start = RELOC(entry)

.globl entry
entry:

	# We haven't set up virtual memory yet, so we're running from
	# the physical address the boot loader loaded the kernel at: 1MB
	# (plus a few bytes).  However, the C code is linked to run at
	# KERNBASE+1MB.  Hence, we set up a trivial page directory that
	# translates virtual addresses [KERNBASE, KERNBASE+4MB) to
	# physical addresses [0x80000000, 4MB).  This 4MB region will be
	# sufficient until we set up our real page table in mem_init
	# in lab 2.

	# Set the stack pointer
	lla     t0, bootstacktop
	add	sp, t0, zero

	# Load the physical address of entry_pgdir into cr3.  entry_pgdir
	# is defined in entrypgdir.c.
	// temp page table for temp use
	call    _page_table_first_create

	# Now paging is enabled, but we're still running at physical 0x80000000 address
	# (why is this okay?) - identical mapping.  Jump up above KERNBASE before entering
	# C code.
	# refer to linux kernel in arch/riscv/kernel/head.S
	//call    relocated
	lla     a6, (entry_pgdir0)
	srli    a6, a6, 12
	li      t0, 0
	or      t0, t0, a6
	li 	t1, SATP_SV48
	or      t0, t0, t1
	sfence.vma
	csrw    CSR_SATP, t0

	la  	a2, jump_relocated
	jr 	a2
jump_relocated:	
	
	# Clear the frame pointer register (EBP)
	# so that once we get into debugging C code,
	# stack backtraces will be terminated properly.
	# nuke frame pointer ;FIXME: fp pointer

	# reset the stack
	la     t0, bootstacktop
	add	sp, t0, zero

	# now to C code
	call	riscv_init

	# Should never get here, but in case we do, just spin.
spin:	j	spin


	.align 3
relocated:
	/* Relocate return address */
	la 	t0, kernel_base
	ld	a1, 0(t0)
	add 	ra, ra, a1

	/* Point stvec to virtual address of intruction after satp write */
	la 	a2, 1f
	add	a2, a2, a1
	csrw 	CSR_STVEC, a2
	
	/* Compute satp for kernel page tables, but don't load it yet */
	lla     a6, (entry_pgdir0)
	srli    a6, a6, 12
	li      t0, 0
	or      t0, t0, a6
	li 	t1, SATP_SV48
	or      t0, t0, t1
	sfence.vma
	csrw    CSR_SATP, t0

.align 2
1:	
	/* Set trap vector to spin forever to help debug */
	la a0, _park_infinite
	csrw CSR_STVEC, a0

	ret

	.align 2
_park_infinite:
	wfi
	j _park_infinite
	
	.align 3
	.section .data
	.globl kernel_base
kernel_base:	
	.dword	KERNBASE

.data
###################################################################
# boot stack
###################################################################
	.p2align	PGSHIFT		# force page alignment
	.globl		bootstack
bootstack:
	.space		KSTKSIZE
	.globl		bootstacktop   
bootstacktop:
