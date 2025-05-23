// System call stubs.

#include <inc/riscv/syscall.h>
#include <inc/riscv/lib.h>

static inline int64_t
syscall(int num, int check, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
        register int64_t ret asm("a0");

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

void
sys_yield(void)
{
	syscall(SYS_yield, 0, 0, 0, 0, 0, 0);
}

int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	return syscall(SYS_page_alloc, 1, envid, (uint64_t) va, perm, 0, 0);
}

int
sys_page_map(envid_t srcenv, void *srcva, envid_t dstenv, void *dstva, int perm)
{
	return syscall(SYS_page_map, 1, srcenv, (uint64_t) srcva, dstenv, (uint64_t) dstva, perm);
}

int
sys_page_unmap(envid_t envid, void *va)
{
	return syscall(SYS_page_unmap, 1, envid, (uint64_t) va, 0, 0, 0);
}

// sys_exofork is inlined in lib.h

int
sys_env_set_status(envid_t envid, int status)
{
	return syscall(SYS_env_set_status, 1, envid, status, 0, 0, 0);
}

int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	return syscall(SYS_env_set_trapframe, 1, envid, (uint64_t) tf, 0, 0, 0);
}

int
sys_env_set_pgfault_upcall(envid_t envid, void *upcall)
{
	return syscall(SYS_env_set_pgfault_upcall, 1, envid, (uint64_t) upcall, 0, 0, 0);
}

int
sys_ipc_try_send(envid_t envid, uint64_t value, void *srcva, int perm)
{
	return syscall(SYS_ipc_try_send, 0, envid, value, (uint64_t) srcva, perm, 0);
}

int64_t
sys_ipc_recv(void *dstva)
{
	return syscall(SYS_ipc_recv, 1, (uint64_t)dstva, 0, 0, 0, 0);
}


int
sys_page_clear_dirty(envid_t srcenv, void *srcva, envid_t dstenv, void *dstva)
{
	return syscall(SYS_page_clear_dirty, 1, srcenv, (uint64_t) srcva, dstenv, (uint64_t) dstva, 0);
}

int
sys_uvpt_pte(void *srcva)
{
        return syscall(SYS_uvpt_pte, 0, (uint64_t)srcva, 0, 0, 0, 0);
}
	
int
sys_copy_shared_pages(envid_t child)
{
        return syscall(SYS_copy_shared_pages, 1, child, 0, 0, 0, 0);
}


int
sys_disable_irq(void){
        return syscall(SYS_disable_irq, 0, 0, 0, 0, 0, 0);
}

int
sys_enable_irq(void){
        return syscall(SYS_enable_irq, 0, 0, 0, 0, 0, 0);
}

unsigned int
sys_time_msec(void)
{
	return (unsigned int) syscall(SYS_time_msec, 0, 0, 0, 0, 0, 0);
}

int
sys_pack_send(const char *data, int len)
{
	return (int)syscall(SYS_pack_send, 1, (uint64_t)data, (uint64_t)len, 0, 0, 0);
}
int
sys_pack_recv(const char *data, int *len)
{
	//return (int)syscall(SYS_pack_recv, 0, (uint32_t)data, (uint32_t)len, 0, 0, 0);
	return (int)syscall(SYS_pack_recv, 1, (uint64_t)data, (uint64_t)len, 0, 0, 0);
}
