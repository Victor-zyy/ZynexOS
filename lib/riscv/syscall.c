// System call stubs.

#include <inc/riscv/syscall.h>
#include <inc/riscv/lib.h>

static inline int32_t
syscall(int num, int check, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
	int32_t ret = 0;

	// Generic system call: pass system call number in AX,
	// up to five parameters in DX, CX, BX, DI, SI.
	// Interrupt kernel with T_SYSCALL.
	//
	// The "volatile" tells the assembler not to optimize
	// this instruction away just because we don't use the
	// return value.
	//
	// The last clause tells the assembler that this can
	// potentially change the condition codes and arbitrary
	// memory locations.
        register uintptr_t a0 asm ("a0") = (uintptr_t)(num);
        register uintptr_t a1_ asm ("a1") = (uintptr_t)(a1);
        register uintptr_t a2_ asm ("a2") = (uintptr_t)(a2);
        register uintptr_t a3_ asm ("a3") = (uintptr_t)(a3);
        register uintptr_t a4_ asm ("a4") = (uintptr_t)(a4);
        register uintptr_t a5_ asm ("a5") = (uintptr_t)(a5);

        asm volatile ("ecall"
                      : "+r" (ret)
                      : "r" (a0), "r" (a1_), "r" (a2_), "r" (a3_), "r" (a4_), "r" (a5_)
                      : "memory");

	if(check && ret > 0)
		panic("syscall %d returned %d (> 0)", num, ret);

	return ret;
}

void
sys_cputs(const char *s, size_t len)
{
	syscall(SYS_cputs, 0, (uint64_t)s, len, 0, 0, 0);
}

int
sys_cgetc(void)
{
	return syscall(SYS_cgetc, 0, 0, 0, 0, 0, 0);
}

int
sys_env_destroy(envid_t envid)
{
	return syscall(SYS_env_destroy, 1, envid, 0, 0, 0, 0);
}

envid_t
sys_getenvid(void)
{
	 return syscall(SYS_getenvid, 0, 0, 0, 0, 0, 0);
}

