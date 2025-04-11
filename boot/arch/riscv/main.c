#include <riscv/riscv.h>
#include <riscv/types.h>
#include <riscv/elf.h>

/**********************************************************************
 * This a dirt simple boot loader, whose sole job is to boot
 * an ELF kernel image from the CFI-nor-flash.
 *
 * DISK LAYOUT
 *  * This program(boot.S and main.c) is the bootloader.  It should
 *    be stored in the first sector of the flash (512KB).
 *
 *  * The 1024 block onward holds the kernel image.
 *
 *  * The kernel image must be in ELF format.
 *
 * BOOT UP STEPS
 *  * when the CPU boots it first run the MROM firmware and then jump to norflash to execute it
 *
 *  * the nor-flash bootcode intializes devices like DDR, sets of the interrupt routines, and
 *    reads the first sector of the boot device(e.g., nor-flash, hard-drive)
 *    into memory and jumps to it.
 *
 *  * Assuming this boot loader is stored in the first sector of the
 *    nor-flash, hard-drive, this code takes over...
 *
 *  * control starts in boot.S -- which sets up temporary interupt vector entry,
 *    and a stack so C code then run, then calls bootmain()
 *
 *  * bootmain() in this file takes over, reads in the kernel and jumps to it.
 **********************************************************************/


/**********************************************************************
 *
 * ----------------Nor-Flash Memory Map -------------------------------
 *   512KB            512KB          3M              remain
 *|----------------|-------------|------------------|---------------------------|
 *|   bootloader   |   opensbi   |    kernel(elf)   |         rootfs            |
 *|----------------|-------------|------------------|---------------------------|
 **********************************************************************/
#define BLOCKSIZE	1024 // 1KB 0x1000
#define ELFHDR		((struct Elf *) 0x801fe000) // scratch space
#define SBI_FIRMWARE    (0x80000000)
#define NOR_FALSH_BASE  (0x20000000)

void readblock(unsigned long, unsigned long, unsigned long);

void
bootmain(unsigned int hartid, void *fdt)
{
	struct Proghdr *ph, *eph;
	
	void (*firmware_entry)(int hartid, void *fdt);

	firmware_entry = (void (*)(int, void*))SBI_FIRMWARE;

	// read 1st page off disk
	readblock((unsigned long) SBI_FIRMWARE, BLOCKSIZE*512, 0x80000);

	readblock((unsigned long) ELFHDR, BLOCKSIZE*8, 0x100000);
	// is this a valid ELF?
	if (ELFHDR->e_magic != ELF_MAGIC)
		goto bad;

	// load each program segment (ignores ph flags)
	ph = (struct Proghdr *) ((uint8_t *) ELFHDR + ELFHDR->e_phoff);
	eph = ph + ELFHDR->e_phnum;
	for (; ph < eph; ph++){
		// p_pa is the load address of this segment (as well
		// as the physical address)
	  if(ph->p_type == ELF_PROG_LOAD)
	    readblock(ph->p_pa, ph->p_memsz, ph->p_offset + 0x100000);
	}

	// call the entry point from the ELF header
	// note: does not return!
	//((void (*)(void)) (ELFHDR->e_entry))();
	firmware_entry(hartid, fdt);

bad:
	while (1)
		/* do nothing */;
}

// Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
// Might copy more than asked
void
readblock(unsigned long pa, unsigned long count, unsigned long offset)
{
	unsigned long end_pa;
	unsigned long flash_base = (unsigned long)NOR_FALSH_BASE;
	unsigned long src = flash_base + offset;
	end_pa = pa + count;

	// round down to sector boundary
	pa &= ~(BLOCKSIZE - 1);

	// translate from bytes to block one block per page 4096Bytes
	// offset = (offset / BLOCKSIZE);

	// If this is too slow, we could read lots of sectors at a time.
	// We'd write more to memory than asked, but it doesn't matter --
	// we load in increasing order.
	while (pa < end_pa) {
		// Since we haven't enabled paging yet and we're using
		// an identity segment mapping (see boot.S), we can
		// use physical addresses directly.  This won't be the
		// case once JOS enables the MMU.
	        *(unsigned long *)pa = *(unsigned long *)src;
		++pa;
		++src;
	}
}


