#include <inc/riscv/csr.h>

	# riscv64 without no opensbi firmware
	# start from 0x1000 MROM Space
	# then we jump into the pflash area from 0x20000000
	# in the real hardware we have to initialize the DDR controller
	# after set up the stack in SRAM 0x8000
	# finally we jump into the 0x80200000 address in DDR

	.section .text, "ax", %progbits
	.align 3
	.globl start
start:

	/* Mask all interrupts */
	csrw CSR_MIE, zero
	csrw CSR_MIP, zero

	/* Setup temporary trap handler */
	lla	s4, _start_hang
	csrw	CSR_MTVEC, s4

	mv	tp, zero

	#setup stack for 4096bytes
	/* Setup stack */
	la	a3, 0x8000
	li	a4, 0x1000
	add	sp, a3, a4

	#initialize the DDR controller
	call _ddr_initialize	

	#jump to bootmain
	csrr	a0, 	mhartid
	call   bootmain
	

	.section .text, "ax", %progbits
	.align 3
	.globl _start_hang
_start_hang:
	wfi
	j	_start_hang


	.section .text, "ax", %progbits
	.align 3
	.globl _ddr_initialize
_ddr_initialize:
	ret			;
