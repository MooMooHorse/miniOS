/**
 * @file vm.c
 * @author Scharfrichter Qian
 * @brief Function implementations for virtual memory management.
 * @version 0.1
 * @date 2022-10-14
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "mmu.h"

/* vm_init
 * 
 * Sets up the page directory, the first page table, creates direct mapping
 * for the second 4M-page in physical memory, and maps video memory. Then it
 * set control registers to enable paging.
 * Inputs: None
 * Outputs: None
 * Side Effects: Modifies `cr0`, `cr3`, `cr4`. Set up paging mechanism.
 */
void
vm_init(void) {
    pgdir[0] = (uint32_t) pgtbl | PAGE_P | PAGE_RW;  // Map the first page table.
    pgdir[1] = (1U << PDXOFF) | PAGE_P | PAGE_RW | PAGE_PS | PAGE_G;  // PDE #1 --> 4M ~ 8M
    pgtbl[PTX(VIDEO)] = VIDEO | PAGE_P | PAGE_RW;   // Map PTE: 0xB8000 ~ 0xB9000

    // Turn on page size extension for 4MB pages.
    asm volatile(
        "movl %%cr4, %%eax      \n\t\
         orl %0, %%eax          \n\t\
         movl %%eax, %%cr4      \n\t\
         "
         :
         : "r" (CR4_PSE)
         : "eax"
    );

    // Set page directory.
    lcr3((uint32_t) pgdir);

    // Turn on paging.
    asm volatile(
        "movl %%cr0, %%eax      \n\t\
         orl %0, %%eax          \n\t\
         movl %%eax, %%cr0      \n\t\
         "
         :
         : "r" (CR0_PG | CR0_WP)  // WP set to facilitate COW fork.
         : "eax"
    );
}


/**
 * @brief This function maps 1 extended page (4MB) for program image.
 * @param pa is *4MB-aligned* physical address where user virtual memory starting at 128MB (0x08000000) is mapped to.
 * @return 0 if succeeded, -1 otherwise (`pa` is not 4MB-aligned).
 */
int32_t
uvmmap_ext(uint32_t pa) {
    if (pa << (PTESIZE - PDXOFF)) {
        return -1;  // Physical address not 4MB-aligned.
    }
    pgdir[PDX(IMG_START)] = pa | PAGE_P | PAGE_RW | PAGE_PS | PAGE_U;  // Map virtual address starting from 128MB.
    lcr3((uint32_t) pgdir);  // Flush TLB.
    return 0;
}

/*!
 * @brief This function maps video memory in the user page table so that user programs can modify video memory in
 * their virtual address space. It also writes the start of user virtual address of video memory to the pointer
 * passed in.
 * @param screen_start is pointer to user buffer which holds starting virtual address of video memory.
 * @return 0 if successful, non-zero otherwise.
 * @sideeffect It modifies page table to enable user access of video memory.
 */
int32_t
uvmmap_vid(uint8_t** screen_start) {
    uint32_t va = (uint32_t) screen_start;

    // Check input pointer validity.
    if (UVM_START > va || UVM_START + UVM_SIZE - sizeof(uint32_t*) < va) {
        return -1;
    }

    // Commit changes to user pointer.
    *screen_start = (uint8_t*) (UVM_START + UVM_SIZE);

    pgdir[PDX(*screen_start)] = (uint32_t) pgtbl_vid | PAGE_P | PAGE_RW | PAGE_U;
    pgtbl_vid[PTX(*screen_start)] = VIDEO | PAGE_P | PAGE_RW | PAGE_U;

    lcr3((uint32_t) pgdir);  // Flush TLB.

    return 0;
}

/*!
 * @brief This function undoes the changes to the paging mechanism performed by `uvmmap_vid`.
 * @param None.
 * @return 0 on success.
 * @sideeffect It modifies page table to unmap user video memory mapping.
 */
int32_t
uvmunmap_vid(void) {
    uint32_t va = UVM_START + UVM_SIZE;

    // Undo user video memory mapping.
    pgdir[PDX(va)] &= ~PAGE_P;
    pgtbl_vid[PTX(va)] &= ~PAGE_P;

    lcr3((uint32_t) pgdir);  // Flush TLB.

    return 0;
}
