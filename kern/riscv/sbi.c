#include <riscv/sbi.h>

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
			unsigned long arg1, unsigned long arg2,
			unsigned long arg3, unsigned long arg4,
			unsigned long arg5)
{
	struct sbiret ret;

	register uintptr_t a0 asm ("a0") = (uintptr_t)(arg0);
	register uintptr_t a1 asm ("a1") = (uintptr_t)(arg1);
	register uintptr_t a2 asm ("a2") = (uintptr_t)(arg2);
	register uintptr_t a3 asm ("a3") = (uintptr_t)(arg3);
	register uintptr_t a4 asm ("a4") = (uintptr_t)(arg4);
	register uintptr_t a5 asm ("a5") = (uintptr_t)(arg5);
	register uintptr_t a6 asm ("a6") = (uintptr_t)(fid);
	register uintptr_t a7 asm ("a7") = (uintptr_t)(ext);
	asm volatile ("ecall"
		      : "+r" (a0), "+r" (a1)
		      : "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
		      : "memory");
	ret.error = a0;
	ret.value = a1;

	return ret;
}


/**
 * sbi_console_putchar() - Writes given character to the console device.
 * @ch: The data to be written to the console.
 *
 * Return: None
 */
void sbi_console_putchar(int ch)
{
	sbi_ecall(SBI_EXT_0_1_CONSOLE_PUTCHAR, 0, ch, 0, 0, 0, 0, 0);
}

/**
 * sbi_console_getchar() - Reads a byte from console device.
 *
 * Returns the value read from console.
 */
int sbi_console_getchar(void)
{
	struct sbiret ret;

	ret = sbi_ecall(SBI_EXT_0_1_CONSOLE_GETCHAR, 0, 0, 0, 0, 0, 0, 0);

	return ret.error;
}


/**
 * sbi_firmware_getstart() - Gets opensbi firmware start address
 *
 * Returns the value of the address.
 */
long sbi_firmware_getstart(void)
{
	struct sbiret ret;
	ret = sbi_ecall(SBI_EXT_BASE, SBI_EXT_FIRMWARE_START, 0, 0, 0, 0, 0, 0);

	return ret.value;
}

/**
 * sbi_firmware_getend() - Gets opensbi firmware end address
 *
 * Returns the value of the address.
 */
long sbi_firmware_getend(void)
{
	struct sbiret ret;
	ret = sbi_ecall(SBI_EXT_BASE, SBI_EXT_FIRMWARE_END, 0, 0, 0, 0, 0, 0);

	return ret.value;
}

/**
 * sbi_mem_getstart() - Gets opensbi firmware start address
 *
 * Returns the value of the address.
 */
long sbi_mem_getstart(void)
{
	struct sbiret ret;
	ret = sbi_ecall(SBI_EXT_BASE,SBI_EXT_BASE_GET_MEMSTART, 0, 0, 0, 0, 0, 0);

	return ret.value;
}

/**
 * sbi_firmware_getend() - Gets opensbi firmware end address
 *
 * Returns the value of the address.
 */
long sbi_mem_getsize(void)
{
	struct sbiret ret;
	ret = sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_GET_MEMSIZE, 0, 0, 0, 0, 0, 0);

	return ret.value;
}


static int __sbi_rfence_v01(int fid, const unsigned long *hart_mask,
                            unsigned long start, unsigned long size,
                            unsigned long arg4, unsigned long arg5)
{
        int result = 0;

        /* v0.2 function IDs are equivalent to v0.1 extension IDs */
        switch (fid) {
        case SBI_EXT_RFENCE_REMOTE_FENCE_I:
	    sbi_ecall(SBI_EXT_0_1_REMOTE_FENCE_I, 0,
			(unsigned long)hart_mask, 0, 0, 0, 0, 0);
	    break;
        case SBI_EXT_RFENCE_REMOTE_SFENCE_VMA:
	    sbi_ecall(SBI_EXT_0_1_REMOTE_SFENCE_VMA, 0,
			(unsigned long)hart_mask, start, size,
			0, 0, 0);
	    break;
        case SBI_EXT_RFENCE_REMOTE_SFENCE_VMA_ASID:
	    sbi_ecall(SBI_EXT_0_1_REMOTE_SFENCE_VMA_ASID, 0,
			(unsigned long)hart_mask, start, size,
			arg4, 0, 0);
	    break;
        default:
	  result = SBI_ERR_FAILURE;
        }

        return result;
}
/**
 * sbi_remote_sfence_vma() - Execute SFENCE.VMA instructions on given remote
 *                           harts for the specified virtual address range.
 * @hart_mask: A cpu mask containing all the target harts.
 * @start: Start of the virtual address
 * @size: Total size of the virtual address range.
 *
 * Return: 0 on success, appropriate linux error code otherwise.
 */
int sbi_remote_sfence_vma(const unsigned long *hart_mask,
                           unsigned long start,
                           unsigned long size)
{
        return __sbi_rfence_v01(SBI_EXT_RFENCE_REMOTE_SFENCE_VMA,
                            hart_mask, start, size, 0, 0);
}

/**
 * sbi_remote_sfence_vma_asid() - Execute SFENCE.VMA instructions on given
 * remote harts for a virtual address range belonging to a specific ASID.
 *
 * @hart_mask: A cpu mask containing all the target harts.
 * @start: Start of the virtual address
 * @size: Total size of the virtual address range.
 * @asid: The value of address space identifier (ASID).
 *
 * Return: 0 on success, appropriate linux error code otherwise.
 */
int sbi_remote_sfence_vma_asid(const unsigned long *hart_mask,
                                unsigned long start,
                                unsigned long size,
                                unsigned long asid)
{
        return __sbi_rfence_v01(SBI_EXT_RFENCE_REMOTE_SFENCE_VMA_ASID,
				hart_mask, start, size, asid, 0);
}

