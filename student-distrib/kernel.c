/* kernel.c - the C part of the kernel
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "mmu.h"
#include "pit.h"
#include "rtc.h"
#include "debug.h"
#include "tests.h"
#include "keyboard.h"
#include "cursor.h"
#include "psmouse.h"
#include "filesystem.h"
#include "syscall.h"
#include "terminal.h"
#include "process.h"
#include "signal.h"
#include "ata.h"
#include "vga.h"
#include "sb16.h"

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags, bit)   ((flags) & (1 << (bit)))

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void entry(unsigned long magic, unsigned long addr) {

    multiboot_info_t *mbi;
    int32_t i;

    /* Clear the screen. */
    clear();

    /* Am I booted by a Multiboot-compliant boot loader? */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printf("Invalid magic number: 0x%#x\n", (unsigned)magic);
        return;
    }

    /* Set MBI to the address of the Multiboot information structure. */
    mbi = (multiboot_info_t *) addr;

    /* Print out the flags. */
    printf("flags = 0x%#x\n", (unsigned)mbi->flags);

    /* Are mem_* valid? */
    if (CHECK_FLAG(mbi->flags, 0))
        printf("mem_lower = %uKB, mem_upper = %uKB\n", (unsigned)mbi->mem_lower, (unsigned)mbi->mem_upper);

    /* Is boot_device valid? */
    if (CHECK_FLAG(mbi->flags, 1))
        printf("boot_device = 0x%#x\n", (unsigned)mbi->boot_device);

    /* Is the command line passed? */
    if (CHECK_FLAG(mbi->flags, 2))
        printf("cmdline = %s\n", (char *)mbi->cmdline);
    /* Module 0 : File System Image */
    if (CHECK_FLAG(mbi->flags, 3)) {
        int mod_count = 0;
        int i;
        module_t* mod = (module_t*)mbi->mods_addr;
        while (mod_count < mbi->mods_count) {
            /* start of file system address for Module 0 */
            if(mod_count==FILESYS_MOD){ /* open file system given module address */
                fs.open_fs((uint32_t)mod);
            }
            printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start); 
            /* end of file system address for Module 0 */
            printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
            printf("First few bytes of module:\n");
            for (i = 0; i < 16; i++) {
                printf("0x%x ", *((char*)(mod->mod_start+i)));
            }
            printf("\n");
            mod_count++;
            mod++;
        }
    }
    /* Bits 4 and 5 are mutually exclusive! */
    if (CHECK_FLAG(mbi->flags, 4) && CHECK_FLAG(mbi->flags, 5)) {
        printf("Both bits 4 and 5 are set.\n");
        return;
    }

    /* Is the section header table of ELF valid? */
    if (CHECK_FLAG(mbi->flags, 5)) {
        elf_section_header_table_t *elf_sec = &(mbi->elf_sec);
        printf("elf_sec: num = %u, size = 0x%#x, addr = 0x%#x, shndx = 0x%#x\n",
                (unsigned)elf_sec->num, (unsigned)elf_sec->size,
                (unsigned)elf_sec->addr, (unsigned)elf_sec->shndx);
    }

    /* Are mmap_* valid? */
    if (CHECK_FLAG(mbi->flags, 6)) {
        memory_map_t *mmap;
        printf("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
                (unsigned)mbi->mmap_addr, (unsigned)mbi->mmap_length);
        for (mmap = (memory_map_t *)mbi->mmap_addr;
                (unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length;
                mmap = (memory_map_t *)((unsigned long)mmap + mmap->size + sizeof (mmap->size)))
            printf("    size = 0x%x, base_addr = 0x%#x%#x\n    type = 0x%x,  length    = 0x%#x%#x\n",
                    (unsigned)mmap->size,
                    (unsigned)mmap->base_addr_high,
                    (unsigned)mmap->base_addr_low,
                    (unsigned)mmap->type,
                    (unsigned)mmap->length_high,
                    (unsigned)mmap->length_low);
    }

    if(CHECK_FLAG(mbi->flags,7)){
        drive_t* drive_info=(drive_t*)mbi->drives_addr;
        // printf("drive number %d\n",drive_info->drive_number);
        printf("drive : head = %d cylinders = %d sectors = %d\n",
        drive_info->drive_heads,drive_info->drive_cylinders,drive_info->drive_sectors);
    }

    /* Construct an LDT entry in the GDT */
    {
        seg_desc_t the_ldt_desc;
        the_ldt_desc.granularity = 0x0;
        the_ldt_desc.opsize      = 0x1;
        the_ldt_desc.reserved    = 0x0;
        the_ldt_desc.avail       = 0x0;
        the_ldt_desc.present     = 0x1;
        the_ldt_desc.dpl         = 0x0;
        the_ldt_desc.sys         = 0x0;
        the_ldt_desc.type        = 0x2;

        SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
        ldt_desc_ptr = the_ldt_desc;
        lldt(KERNEL_LDT);
    }

    /* Construct a TSS entry in the GDT */
    {
        seg_desc_t the_tss_desc;
        the_tss_desc.granularity   = 0x0;
        the_tss_desc.opsize        = 0x0;
        the_tss_desc.reserved      = 0x0;
        the_tss_desc.avail         = 0x0;
        the_tss_desc.seg_lim_19_16 = TSS_SIZE & 0x000F0000;
        the_tss_desc.present       = 0x1;
        the_tss_desc.dpl           = 0x0;
        the_tss_desc.sys           = 0x0;
        the_tss_desc.type          = 0x9;
        the_tss_desc.seg_lim_15_00 = TSS_SIZE & 0x0000FFFF;

        SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

        tss_desc_ptr = the_tss_desc;

        tss.ldt_segment_selector = KERNEL_LDT;
        tss.ss0 = KERNEL_DS;
        tss.esp0 = 0x800000;
        ltr(KERNEL_TSS);
    }

    /* Initialize devices, memory, filesystem, enable device interrupts on the
     * PIC, any other initialization stuff... */
    vm_init();
    
    /* Init the PIC */
    i8259_init();

    /* Initialize devices */
    pit_init();
    rtc_init();
    keyboard_init();
    cursor_init();
    sb16_init();
    psmouse_init();

    dump_fs();

    terminal_index=1; /* default : terminal 1 */
    terminal[1].open(1,(int32_t*)get_terbuf_addr(terminal_index)); /* open active terminal */
    init_pcb();
    for(i=2;i<=3;i++){/* debugging version : support 3 terminals */
        terminal_index=i;
        terminal[i].open(i,(int32_t*)get_terbuf_addr(terminal_index));
        _execute((uint8_t*)"shell",i,0); /* open pcb for shell i with terminal i parent pid 0 */
    }
    terminal_index=1;
    /* note that now, kernel stack for shell 2 and shell 3 are both EMPTY */
    /* You have to manually check base shell in context switch */
    
    /* Enable interrupts */
    /* Do not enable the following until after you have set up your
     * IDT correctly otherwise QEMU will triple fault and simple close
     * without showing you any output */
    // sti();

    #ifdef RUN_TESTS
        /* Run tests */
        launch_tests();
    #endif


    /* Execute the first program ("shell") ... */
    
    // Brief clear_screen() and handle_verticle_scoll() tests -- REMOVE LATER
    // printf("Lord have mercy, I'm about to vertical scroll!\n");
    // handle_vertical_scroll();

    // printf("Lord have mercy, I'm about to clear the screen!\n");
    // clear_screen();
    // while(1){   
    //     execute((uint8_t*)"shell");
    // }


    execute((uint8_t*)"shell");
    /* Spin (nicely, so we don't chew up cycles) */
    asm volatile (".1: hlt; jmp .1;");
}
