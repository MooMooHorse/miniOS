/* mmu.h - This file contains definitions for the x86 memory management unit.
 */

#ifndef _MMU_H
#define _MMU_H
  
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "types.h"
#include "debug.h"
#include "tests.h"

// Constants.
#define PGSIZE          4096                // Bytes per page.
#define PTXOFF          12                  // Offset of PTX in a virtual address.
#define PDXOFF          22                  // Offset of PDX in a virtual address.
#define PAGE_P          (1L << 0)           // Present
#define PAGE_RW         (1L << 1)           // Read/Write
#define PAGE_U          (1L << 2)           // User/Supervisor
#define PAGE_PWT        (1L << 3)           // Write-through
#define PAGE_PCD        (1L << 4)           // Cache disabled
#define PAGE_A          (1L << 5)           // Accessed
#define PAGE_D          (1L << 6)           // Dirty
#define PAGE_PS         (1L << 7)           // Page size (0 indicates 4 KB)
#define PAGE_PAT        (1L << 7)           // Page table attribute index
#define PAGE_G          (1L << 8)           // Global page (ignored)
#define VIDEO           0xB8000             // Copied from `lib.c`.

// Control registers setting.
#define CR0_PE          (1L << 0)           // Protection enable
#define CR0_WP          (1L << 16)          // Write protect
#define CR0_PG          (1L << 31)          // Paging
#define CR4_PSE         (1L << 4)           // Page-size extension

// Extract part of a PDE/PTE.
#define PDX(va)             (((uint32_t) (va) >> PDXOFF) & 0x3FF)
#define PTX(va)             (((uint32_t) (va) >> PTXOFF) & 0x3FF)
#define PAGE_ADDR(pte)      ((uint32_t) (pte) & ~0xFFF)        // Extract the index.
#define PAGE_FLAGS(pte)     ((uint32_t) (pte) & 0xFFF)         // Extract the flags.

// Assemble indices and offset into a virtual memory.
#define VA(d, t, o)         (((d) << PDXOFF) | (t) << PTXOFF | (o))

#define PGROUNDUP(x)        (((x) + PGSIZE - 1) & ~(PGSIZE - 1))
#define PGROUNDDOWN(x)      ((x) & ~(PGSIZE - 1))

typedef uint32_t pte_t;
typedef uint32_t pde_t;
typedef uint32_t* pgdir_t;      // 1024 PDEs or a 4MB page.
typedef uint32_t* pgtbl_t;      // 1024 PTEs.

/* Set up page dir and page table. Enable paging. */
void vm_init(void);


#endif /* _MMU_H */
