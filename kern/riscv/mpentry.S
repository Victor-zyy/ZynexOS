/* See COPYRIGHT for copyright information. */

#include <inc/riscv/mmu.h>
#include <inc/riscv/csr.h>
#include <inc/riscv/memlayout.h>

###################################################################
# entry point for APs
###################################################################

# Each non-boot CPU ("AP") is started up in response to a STARTUP
# IPI from the boot CPU.  Section B.4.2 of the Multi-Processor
# Specification says that the AP will start in real mode with CS:IP
# set to XY00:0000, where XY is an 8-bit value sent with the
# STARTUP. Thus this code must start at a 4096-byte boundary.
#
# Because this code sets DS to zero, it must run from an address in
# the low 2^16 bytes of physical memory.
#
# boot_aps() (in init.c) copies this code to MPENTRY_PADDR (which
# satisfies the above restrictions).  Then, for each AP, it stores the
# address of the pre-allocated per-core stack in mpentry_kstack, sends
# the STARTUP IPI, and waits for this code to acknowledge that it has
# started (which happens in mp_main in init.c).
#
# This code is similar to boot/boot.S except that
#    - it does not need to enable A20
#    - it uses MPBOOTPHYS to calculate absolute addresses of its
#      symbols, rather than relying on the linker to fill them

#define RELOC(x) ((x) - KERNBASE)
#define MPBOOTPHYS(s) ((s) - mpentry_start + MPENTRY_PADDR)
#define KERNEL_BASE	0x80200000
#define OFFSET (KERNEL_BASE - MPENTRY_PADDR)
	// Now we are in s-mode after ipi envoked in opensbi 
.globl mpentry_start
mpentry_start:


	# clear the interrupt
	csrw CSR_SIE, zero
	csrw CSR_SIP, zero

	# Set up initial page table. We cannot use kern_pgdir yet because
	lla	a6, (mpentry_start) ;
	lla	a4, (entry)
	lla     a5, (entry_pgdir0)
	sub	a4, a5, a4
	li	t0,	0x10
	slli	t0,	t0, 16
	add	a4, a4, t0
	add	a6, a6, a4

	srli    a6, a6, 12
	li      t0, 0
	or      t0, t0, a6
	li 	t1, SATP_SV48
	or      t0, t0, t1
	sfence.vma
	csrw    CSR_SATP, t0

	// reset the sscratch to zero means now we're in kernel / s-mode
	li	a2, 0
	csrw	sscratch, a2

	lla	a6, (mpentry_start) ;
	lla	a4, (entry)
	lla     a5, (jump_relocated)
	sub	a4, a5, a4
	// a4 store the offset 
	li	t0, 	0x8020
	slli	t0,	t0, 16

	li	t1,	(~1)
	srli	t1,	t1, 	0x20
	slli	t1,	t1, 	0x20

	or	t0,	t0, t1
	add	a6, 	a4, t0

	jr 	a6
jump_relocated:	


	// reset the satp vector
	la	a2, _alltraps
	csrw	CSR_STVEC, a2

	
	# Switch to the per-cpu stack allocated in boot_aps()
	# offset
	la     t0, mpentry_kstack
	ld	t0, 0(t0)
	add	sp, t0, zero

	# nuke frame pointer ;FIXME: fp pointer
	mv	fp, zero

	# Call mp_main().  (Exercise for the reader: why the indirect call?)
	call 	mp_main

	# If mp_main returns (it shouldn't), loop.
spin:
	j     spin


.globl mpentry_end
mpentry_end:
	nop
