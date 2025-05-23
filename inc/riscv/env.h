/* See COPYRIGHT for copyright information. */

#ifndef _INC_ENV_H
#define _INC_ENV_H

#ifndef __ASSEMBLER__

#include <inc/riscv/types.h>
#include <inc/riscv/trap.h>
#include <inc/riscv/memlayout.h>

typedef int32_t envid_t;

// An environment ID 'envid_t' has three parts:
//
// +1+---------------21-----------------+--------10--------+
// |0|          Uniqueifier             |   Environment    |
// | |                                  |      Index       |
// +------------------------------------+------------------+
//                                       \--- ENVX(eid) --/
//
// The environment index ENVX(eid) equals the environment's index in the
// 'envs[]' array.  The uniqueifier distinguishes environments that were
// created at different times, but share the same environment index.
//
// All real environments are greater than 0 (so the sign bit is zero).
// envid_ts less than 0 signify errors.  The envid_t == 0 is special, and
// stands for the current environment.

#define LOG2NENV		10
#define NENV			(1 << LOG2NENV)
//#define ENVX(envid)		((envid) & (NENV - 1))
// env_id 0 as the init kernel thread which we didn't implement or so
// TODO:
//#define ENVX(envid)		(((envid) & (NENV - 1)) - 1)
#define ENVX(envid)		(((envid) & (NENV - 1)))

// Values of env_status in struct Env
enum {
	ENV_FREE = 0,
	ENV_DYING,
	ENV_RUNNABLE,
	ENV_RUNNING,
	ENV_NOT_RUNNABLE
};
/* FIXME: 0 1 2 3 4 */
// Special environment types
enum EnvType {
	ENV_TYPE_USER = 0,
	ENV_TYPE_FS,            // File system server
	ENV_TYPE_NS,            // network server

};

struct Env {
	struct Trapframe env_tf;	// Saved registers
	struct Env *env_link;		// Next free Env
	envid_t env_id;			// Unique environment identifier
	envid_t env_parent_id;		// env_id of this env's parent
	enum EnvType env_type;		// Indicates special system environments
	unsigned env_status;		// Status of the environment
	uint32_t env_runs;		// Number of times environment has run
	int env_cpunum;			// The CPU that the env is running on

	// Address space
	pde_t *env_pgdir;		// Kernel virtual address of page dir

	// Exception handling
	void *env_pgfault_upcall;	// Page fault upcall entry point

	// Lab 4 IPC
	bool env_ipc_recving;		// Env is blocked receiving
	void *env_ipc_dstva;		// VA at which to map received page
	uint64_t env_ipc_value;		// Data value sent to us
	envid_t env_ipc_from;		// envid of the sender
	int env_ipc_perm;		// Perm of page mapping received
};

#endif /* !__ASSEMBLER__ */

#define ENV_TF_OFFSET     0
#define ENV_TF_SIZE       PT_SIZE

#define ENV_LINK_OFF      ENV_TF_SIZE
#define ENV_ID_OFF        ENV_LINK_OFF + 8
#define ENV_PARID_OFF     ENV_ID_OFF   + 4
#define ENV_TYPE_OFF      ENV_PARID_OFF + 4
#define ENV_STAT_OFF      ENV_TYPE_OFF + 4 /* FIXME: enum expand 32bit */
#define ENV_RUNS_OFF      ENV_STAT_OFF + 4
#define ENV_CPUNUM_OFF    ENV_RUNS_OFF + 4
#define ENV_PGDIR_OFF     ENV_CPUNUM_OFF + 4

#endif // !_INC_ENV_H
