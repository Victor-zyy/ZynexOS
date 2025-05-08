#ifndef _INC_MMU_H
#define _INC_MMU_H

/*
 * This file contains definitions for the riscv memory management unit (MMU),
 * including paging- and segmentation-related data structures and constants,
 * the satp registers, and traps.
 */

/*
 *
 *	Part 1.  Paging data structures and constants.
 *
 */

// A linear address 'la' has a five-part structure as follows:
//
// +------16------+------9-----+-----9-----+-------9----------+--------9-------+---------12----------+
// |Reserve no use|   L0 Index |  L1 Index | L2 Page Directory| L3 Page Table  | Offset within Page  |
// |              |            |           |     Index        |     Index      |                     |
// +--------------+------------+-----------+------------------+----------------+---------------------+
//                 \-PD0X(la)-/ \-PD1X(la)-/ \----PD2X(la)----/\--- PTX(la) --/ \---- PGOFF(la) ----/
//                 \--------------------------- PGNUM(la) ---------------------/
//
// The PDX, PTX, PGOFF, and PGNUM macros decompose linear addresses as shown.
// To construct a linear address la from PDX(la), PTX(la), and PGOFF(la),
// use PGADDR(PDX(la), PTX(la), PGOFF(la)).

// page number field of address
#define PHYMASK         0x000000007fffffff
#define VADDRMASK       0x0000ffffffffffff
// 0x801ff000
#define PGNUM(la)	(((uintptr_t) ((uintptr_t)la & (uintptr_t)PHYMASK)) >> PTXSHIFT)
#define TPGNUM(la)	(((uintptr_t) ((uintptr_t)la & (uintptr_t)VADDRMASK)) >> PTXSHIFT)

// page directory index
#define PD0X(la)		((((uintptr_t) (la)) >> PDX0SHIFT) & 0x1FF)
#define PD1X(la)		((((uintptr_t) (la)) >> PDX1SHIFT) & 0x1FF)
#define PD2X(la)		((((uintptr_t) (la)) >> PDX2SHIFT) & 0x1FF)

// page table index
#define PTX(la)		        ((((uintptr_t) (la)) >> PTXSHIFT) & 0x1FF)

// offset in page
#define PGOFF(la)	(((uintptr_t) (la)) & 0xFFF)

// construct linear address from indexes and offset
//#define PGADDR(d, t, o)	((void*) ((d) << PD2XSHIFT | (t) << PTXSHIFT | (o)))
#define PGADDR(d0, d1, d2, t, o)	((void*) ((d0) << PDX0SHIFT | (d1) << PDX1SHIFT | (d2) << PDX2SHIFT | (t) << PTXSHIFT | (o)))

// Page directory and page table constants.
#define NPDENTRIES	512		// page directory entries per page directory
#define NPTENTRIES	512		// page table entries per page table

#define PGSIZE		4096		// bytes mapped by a page
#define PGSHIFT		12		// log2(PGSIZE)

// PTSIZE 2M
#define PTSIZE		(PGSIZE*NPTENTRIES) // bytes mapped by a page directory entry
#define PTSHIFT		22		// log2(PTSIZE)

#define PTXSHIFT	12		// offset of PTX in a linear address
#define PDX2SHIFT	21		// offset of PDX in a linear address
#define PDX1SHIFT	30		// offset of PDX in a linear address
#define PDX0SHIFT	39		// offset of PDX in a linear address

// Page table/directory entry flags.
#define PTE_V		0x001	// Present/Valid
#define PTE_R		0x002	// Readable
#define PTE_W		0x004	// Writeable
#define PTE_X		0x008	// Exectuable
#define PTE_U		0x010	// User
#define PTE_G		0x020	// Global
#define PTE_A		0x040	// Accessed
#define PTE_D		0x080	// Dirty
#define PTE_IO          0x4000000000000000    // IO cache disable strong memory-consistence
#define PTE_MEM         0x2000000000000000    // normal memeory cache disable thin memory-consistence
#define PTE_N           0x8000000000000000 // Sequential entry

// The PTE_AVAIL bits aren't used by the kernel or interpreted by the
// hardware, so user processes are allowed to set them arbitrarily.
#define PTE_AVAIL	0x300	// Available for software use

// Flags in PTE_SYSCALL may be used in system calls.  (Others may not.)
#define PTE_SYSCALL	(PTE_AVAIL | PTE_X |PTE_V | PTE_W | PTE_R |  PTE_U)

// Address in page table or page directory entry
#define PTE_ADDR(pte)	PTE_PHY(((physaddr_t) (pte) & ~0xff800000000003FF))
#define PDE_ADDR(pte)	PTE_PHY(((physaddr_t) (pte) & ~0xff800000000003FF))

// SATP register
#define SATP_SV48  0x9000000000000000
#define SATP_ASID  0x0ffff00000000000
#define SATP_PPN   0x00000fffffffffff
// Eflags register

#ifdef __ASSEMBLER__
#else
#include <inc/riscv/types.h>
#endif
// Page fault error codes

/*
 *
 *	Part 2.  Segmentation data structures and constants.
 *
 */


// Application segment type bits

/*
 *
 *	Part 3.  Traps.
 *
 */



#endif /* !_INC_MMU_H */
