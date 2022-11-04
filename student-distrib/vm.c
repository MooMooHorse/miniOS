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
vm_init(void)
{
    pgdir[0] |= (unsigned long) pgtbl | PAGE_P | PAGE_RW;  // Map the first page table.
    pgdir[1] |= (1U << PDXOFF) | PAGE_P | PAGE_RW | PAGE_PS | PAGE_G;  // PDE #1 --> 4M ~ 8M
    pgtbl[PTX(VIDEO)] |= VIDEO | PAGE_P | PAGE_RW;   // Map PTE: 0xB8000 ~ 0xB9000

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
 * @brief Open paging for program image
 * @param addr - physical address where you want to map from 128MB (0x08000000) to
 * @return ** int32_t - -1 on illegal memory (besides 8 MB and 12MB)
 * 0 on open page success 
 */
int32_t
open_page(uint32_t addr){
    if(addr!=(8<<20)&&addr!=(12<<20)){
        return -1;
    }
    pgdir[PDX(VPROG_START_ADDR)]=addr|PAGE_P|PAGE_RW|PAGE_PS|PAGE_U; /* remap address starting from 128MB */
    lcr3((uint32_t)pgdir); /* flush TLB */
    return 0;
}

