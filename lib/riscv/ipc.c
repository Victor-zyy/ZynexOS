// User-level IPC library routines

#include <inc/riscv/lib.h>

// Receive a value via IPC and return it.
// If 'pg' is nonnull, then any page sent by the sender will be mapped at
//	that address.
// If 'from_env_store' is nonnull, then store the IPC sender's envid in
//	*from_env_store.
// If 'perm_store' is nonnull, then store the IPC sender's page permission
//	in *perm_store (this is nonzero iff a page was successfully
//	transferred to 'pg').
// If the system call fails, then store 0 in *fromenv and *perm (if
//	they're nonnull) and return the error.
// Otherwise, return the value sent by the sender
//
// Hint:
//   Use 'thisenv' to discover the value and who sent it.
//   If 'pg' is null, pass sys_ipc_recv a value that it will understand
//   as meaning "no page".  (Zero is not the right value, since that's
//   a perfectly valid place to map a page.)
int64_t
ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
{
	// LAB 4: Your code here.
	void *dstva;
	int r;
	if(pg == NULL){
		dstva = (void *)0xffffffffffffffff;
	}else{
		dstva = pg;
	}
	r = sys_ipc_recv(dstva);	
	// restore 
	if(from_env_store != NULL){
		*from_env_store = (r < 0) ? 0 : thisenv->env_ipc_from;
	}
	if(perm_store != NULL){
		*perm_store = (r < 0) ? 0 : thisenv->env_ipc_perm;
	}

	if(r < 0)
		return r;
	// success return the value send by the sender
	return thisenv->env_ipc_value;
	//panic("ipc_recv not implemented");
}

// Send 'val' (and 'pg' with 'perm', if 'pg' is nonnull) to 'toenv'.
// This function keeps trying until it succeeds.
// It should panic() on any error other than -E_IPC_NOT_RECV.
//
// Hint:
//   Use sys_yield() to be CPU-friendly.
//   If 'pg' is null, pass sys_ipc_try_send a value that it will understand
//   as meaning "no page".  (Zero is not the right value.)
void
ipc_send(envid_t to_env, uint64_t val, void *pg, int perm)
{
	// LAB 4: Your code here.
	int r;
	void *srcva;
	if(pg == NULL){
		srcva = (void *)0xffffffffffffffff;	
	}else{
		srcva = pg;
	}
	// keeps trying until it succeeds
	//cprintf("to_env : %x val : %d pg : 0x%08lx perm : %x\n", to_env,
	//	val, pg, perm);
	while((r = sys_ipc_try_send(to_env, val, srcva, perm)) < 0){
		if(r != -E_IPC_NOT_RECV){
		  panic("sys_ipc_send panic %e", r);
		}
		// cprintf("thisenv->envid : %x env_status : %d\n", thisenv->env_id, thisenv->env_status);
		// maybe other is now sending someting just yield the CPU 
		// if we regain the CPU control we kept trying to resend
		sys_yield();
	}
	
	//panic("ipc_send not implemented");
}

// Find the first environment of the given type.  We'll use this to
// find special environments.
// Returns 0 if no such environment exists.
envid_t
ipc_find_env(enum EnvType type)
{
	int i;
	for (i = 0; i < NENV; i++)
		if (envs[i].env_type == type)
			return envs[i].env_id;
	return 0;
}
