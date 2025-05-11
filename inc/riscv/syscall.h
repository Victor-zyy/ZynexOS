#ifndef JOS_INC_SYSCALL_H
#define JOS_INC_SYSCALL_H

/* system call numbers */
enum {
	SYS_cputs = 0,
	SYS_cgetc,
	SYS_getenvid,
	SYS_env_destroy,
	SYS_page_alloc,
	SYS_page_map,
	SYS_page_unmap,
	SYS_exofork,
	SYS_env_set_status,
	SYS_env_set_trapframe,
	SYS_yield,/* FIXME:  */
	SYS_env_set_pgfault_upcall,
	SYS_ipc_try_send,
	SYS_ipc_recv,
	SYS_page_clear_dirty,
	SYS_uvpt_pte,
	SYS_copy_shared_pages,
	SYS_disable_irq,
	SYS_enable_irq,
	NSYSCALLS
};

#endif /* !JOS_INC_SYSCALL_H */
