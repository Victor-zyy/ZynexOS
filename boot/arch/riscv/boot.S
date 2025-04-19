#include <inc/riscv/csr.h>

// load data from src_start to [dst_start - dst_end]
.macro load_data,_src_start,_dst_start,_dst_end
	bgeu	\_dst_start, \_dst_end, 2f
1:
	lw      t0, (\_src_start)
	sw      t0, (\_dst_start)
	addi    \_src_start, \_src_start, 4
	addi    \_dst_start, \_dst_start, 4
	bltu    \_dst_start, \_dst_end, 1b
2:
.endm
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
	/* Find Preferred boot Hart ID*/
	li	a6, -1
	li	a7, -1
	beq 	a6, a7, _try_lottery

_try_lottery:
	
	lla	a6, _relocate_lottery
	li	a7, 1
	amoadd.w a6, a7, (a6)
	bnez	a6, _wait_boot_done
	
	/* Mask all interrupts */
	csrw CSR_MIE, zero
	csrw CSR_MIP, zero

	/* Move .data section from flash to sram */
        lla      a3,     _sidata         //a0 = _sidata source data address FLASH
        lla      a4,     _sdata          //a1 = dst relocate address
        lla      a5,     _edata          //a2 = end relocate address data VMA
        load_data       a3,     a4,     a5 

	/* Setup temporary trap handler */
	lla	s4, _start_hang
	csrw	CSR_MTVEC, s4

	mv	tp, zero

	#setup stack for 4096bytes
	/* Setup stack */
	/* SRAM 0x8000 - 0x10000 */
	/* We set the sp at the top level */
	/* In this way, the .data section don't worry */
	la	a3, 0x10000
	add	sp, a3, zero

	#initialize the DDR controller
	call _ddr_initialize	

	#jump to bootmain
	csrr	a0, 	mhartid
	call   bootmain
	
_wait_boot_done:
	la	t0, _boot_status
	li	t1, 1 // 1 means _boot_status == 1
1:	ld	t2, (t0)
	bgt 	t1, t2, 1b
	// jump opensbi firmware
	li 	t0, 0x800
	slli	t0, t0, 20
	csrr	a0, mhartid
	li	a1, 0x802
	slli	a1, a1, 20
	jr	t0
	
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

	.section .data
	.align 3
	.globl _boot_status
	.type _boot_status, %object
_boot_status:
	.dword	1

	.type _relocate_lottery, %object
_relocate_lottery:	
	.dword	0
