// Main public header file for our user-land support library,
// whose code lives in the lib directory.
// This library is roughly our OS's version of a standard C library,
// and is intended to be linked into all user-mode applications
// (NOT the kernel or boot loader).

#ifndef JOS_INC_LIB_H
#define JOS_INC_LIB_H 1

#include <inc/riscv/types.h>
#include <inc/riscv/stdio.h>
#include <inc/riscv/stdarg.h>
#include <inc/riscv/string.h>
#include <inc/riscv/error.h>
#include <inc/riscv/assert.h>
#include <inc/riscv/env.h>
#include <inc/riscv/memlayout.h>
#include <inc/riscv/syscall.h>
#include <inc/riscv/fs.h>
#include <inc/riscv/fd.h>
#include <inc/riscv/args.h>
#include <inc/riscv/malloc.h>
#include <inc/riscv/ns.h>

#define USED(x)		(void)(x)

// main user program
void	umain(int argc, char **argv);

// libmain.c or entry.S
extern const char *binaryname;
extern const volatile struct Env *thisenv;
extern const volatile struct Env envs[NENV];
extern const volatile struct PageInfo pages[];

// exit.c
void	exit(void);

// pgfault.c
void    set_pgfault_handler(void (*handler)(struct UTrapframe *utf));

// fork.c
#define PTE_SHARE	0x200	
envid_t	fork(void);
envid_t	sfork(void); //chanllenge

// readline.c
char*	readline(const char *buf);

// syscall.c
void	sys_cputs(const char *string, size_t len);
int	sys_cgetc(void);
envid_t	sys_getenvid(void);
int	sys_env_destroy(envid_t);

void	sys_yield(void);
static envid_t sys_exofork(void);
int	sys_env_set_status(envid_t env, int status);
int     sys_env_set_trapframe(envid_t envid, struct Trapframe *tf);
int	sys_env_set_pgfault_upcall(envid_t env, void *upcall);
int	sys_page_alloc(envid_t env, void *pg, int perm);
int	sys_page_map(envid_t src_env, void *src_pg,
		     envid_t dst_env, void *dst_pg, int perm);
int	sys_page_unmap(envid_t env, void *pg);
int	sys_ipc_try_send(envid_t to_env, uint64_t value, void *pg, int perm);
int64_t	sys_ipc_recv(void *rcv_pg);
int     sys_page_clear_dirty(envid_t srcenv, void *srcva, envid_t dstenv, void *dstva);
int     sys_uvpt_pte(void *srcva);
int     sys_copy_shared_pages(envid_t child);
int     sys_disable_irq(void);
int     sys_enable_irq(void);
unsigned int sys_time_msec(void);
int sys_pack_send(const char *data, int len);
int sys_pack_recv(const char *data, int *len);

// This must be inlined.  Exercise for reader: why?
// don't let the sys_exofork has stack pushes or pop etc.
// always inline means ecall like insert instruction
static inline envid_t __attribute__((always_inline))
sys_exofork(void)
{
        register envid_t ret asm ("a0") = SYS_exofork;
        asm volatile("ecall\n"
                     : "+r" (ret)
                     : "r" (ret));
        return ret;
}
// ipc.c
void    ipc_send(envid_t to_env, uint64_t value, void *pg, int perm);
int64_t ipc_recv(envid_t *from_env_store, void *pg, int *perm_store);
envid_t ipc_find_env(enum EnvType type);

// fd.c
int	close(int fd);
ssize_t	read(int fd, void *buf, size_t nbytes);
ssize_t	write(int fd, const void *buf, size_t nbytes);
int	seek(int fd, off_t offset);
void	close_all(void);
ssize_t	readn(int fd, void *buf, size_t nbytes);
int	dup(int oldfd, int newfd);
int	fstat(int fd, struct Stat *statbuf);
int	stat(const char *path, struct Stat *statbuf);

// file.c
int	open(const char *path, int mode);
int	ftruncate(int fd, off_t size);
int	remove(const char *path);
int	sync(void);

// pageref.c
int	pageref(void *addr);

// sockets.c
int     accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int     bind(int s, struct sockaddr *name, socklen_t namelen);
int     shutdown(int s, int how);
int     connect(int s, const struct sockaddr *name, socklen_t namelen);
int     listen(int s, int backlog);
int     socket(int domain, int type, int protocol);

// nsipc.c
int     nsipc_accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int     nsipc_bind(int s, struct sockaddr *name, socklen_t namelen);
int     nsipc_shutdown(int s, int how);
int     nsipc_close(int s);
int     nsipc_connect(int s, const struct sockaddr *name, socklen_t namelen);
int     nsipc_listen(int s, int backlog);
int     nsipc_recv(int s, void *mem, int len, unsigned int flags);
int     nsipc_send(int s, const void *buf, int size, unsigned int flags);
int     nsipc_socket(int domain, int type, int protocol);

// spawn.c
envid_t	spawn(const char *program, const char **argv);
envid_t	spawnl(const char *program, const char *arg0, ...);

// console.c
void	cputchar(int c);
int	getchar(void);
int	iscons(int fd);
int	opencons(void);

// pipe.c
int	pipe(int pipefds[2]);
int	pipeisclosed(int pipefd);

// wait.c
void	wait(envid_t env);


/* File open modes */
#define	O_RDONLY	0x0000		/* open for reading only */
#define	O_WRONLY	0x0001		/* open for writing only */
#define	O_RDWR		0x0002		/* open for reading and writing */
#define	O_ACCMODE	0x0003		/* mask for above modes */

#define	O_CREAT		0x0100		/* create if nonexistent */
#define	O_TRUNC		0x0200		/* truncate to zero length */
#define	O_EXCL		0x0400		/* error if already exists */
#define O_MKDIR		0x0800		/* create directory, not regular file */

#endif	// !JOS_INC_LIB_H
