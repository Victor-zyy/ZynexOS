/* Simple linker script for the JOS kernel.
   See the GNU ld 'info' manual ("info ld") to learn the syntax. */

OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
	/* Link the kernel at this address: "." means the current address */
	. = 0xFFFFFFFF80200000;

	/* AT(...) gives the load address of this section, which tells
	   the boot loader where to load the kernel in physical memory */
	.text : AT(0x80200000) {
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}

	PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */

	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

  	. = ALIGN(4);
    	.got : {
    	 __got_start = .;
    	  *(.got.plt) *(.got)
     	  __got_end = .;
        }

	/* Adjust the address for the data segment to the next page */
	. = ALIGN(0x1000);

 
	/* The data segment */
	.data : {
		*(.data) *(.data.rel.ro*) *(.data*)
	}
 
	. = ALIGN(0x1000);
	PROVIDE(edata = .);

	.bss : {
		*(.bss)
	}

	. = ALIGN(0x1000);
	PROVIDE(end = .);

	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack)
	}
}
