#include <inc/riscv/fs.h>
#include <inc/riscv/lib.h>

#define SECTSIZE	512			// bytes per disk sector
#define BLKSECTS	(BLKSIZE / SECTSIZE)	// sectors per block

/* Disk block n, when in memory, is mapped into the file system
 * server's address space at DISKMAP + (n*BLKSIZE). */
#define DISKMAP		0x10000000

/* Maximum disk size we can handle (3GB) */
#define DISKSIZE	0xC0000000

#ifdef FS_GLOBAL_VAR
struct Super *super;		// superblock
uint32_t *bitmap;		// bitmap blocks mapped in memory
#else
extern struct Super *super;		// superblock
extern uint32_t *bitmap;		// bitmap blocks mapped in memory
#endif
/* ide.c */
bool	ide_probe_disk1(void);
void	ide_set_disk(int diskno);
void	ide_set_partition(uint32_t first_sect, uint32_t nsect);
int	ide_read(uint32_t secno, void *dst, size_t nsecs);
int	ide_write(uint32_t secno, const void *src, size_t nsecs);
/* cfi_flash.c */
int	cfi_flash_read(uint32_t secno, void *dst, size_t nsecs);
int	cfi_flash_write(uint32_t secno, const void *src, size_t nsecs);
int     CfiFlashWrite(const uint32_t *buffer, uint32_t offset, uint32_t nbytes);
int     CfiFlashRead(uint32_t *buffer, uint32_t offset, uint32_t nbytes);
int     CfiFlashErase(uint32_t offset);
/* bc.c */
void*	diskaddr(uint32_t blockno);
bool	va_is_mapped(void *va);
bool	va_is_dirty(void *va);
void	flush_block(void *addr);
void	bc_init(void);

/* fs.c */
void	fs_init(void);
int	file_get_block(struct File *f, uint32_t file_blockno, char **pblk);
int	file_create(const char *path, struct File **f);
int	file_open(const char *path, struct File **f);
ssize_t	file_read(struct File *f, void *buf, size_t count, off_t offset);
int	file_write(struct File *f, const void *buf, size_t count, off_t offset);
int	file_set_size(struct File *f, off_t newsize);
void	file_flush(struct File *f);
int	file_remove(const char *path);
void	fs_sync(void);

/* int	map_block(uint32_t); */
bool	block_is_free(uint32_t blockno);
int	alloc_block(void);

/* test.c */
void	fs_test(void);


/* testflash.c */
void	test_flash(void);


