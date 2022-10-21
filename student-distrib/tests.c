#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "filesystem.h"
#include "terminal.h"

/* Include constants for testing purposes. */
#ifndef _MMU_H
#include "mmu.h"
#endif

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER     \
    printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)    \
    printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
    /* Use exception #15 for assertions, otherwise
       reserved by Intel */
    asm volatile("int $15");
}

/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
    TEST_HEADER;

    int i;
    int result = PASS;
    for (i = 0; i < 10; ++i){
        if ((idt[i].offset_15_00 == NULL) &&
            (idt[i].offset_31_16 == NULL)){
            assertion_failure();
            result = FAIL;
        }
    }

    return result;
}

// add more tests here
/**
 * @brief This is weekened version of idt test
 * Coverage IDT test
 * Output PASS/FAIL
 * @return ** int 
 */
int humble_idt_test(){
    TEST_HEADER;

    int i;
    int result=PASS;
    for(i=0;i<20;i++){
        if(((idt[i].offset_31_16<<16)|(idt[i].offset_15_00))==0){
            if(i!=9&&i!=15) return FAIL;
        }
    }
    return result;
}

/**
 * @brief test divide by zero exception
 * Coverage exception
 * OUTPUT : print exception message
 * @return ** int 
 */
int exception_test(){
    TEST_HEADER;
    volatile int x = 0;
    return 1 / x;
}

/**
 * @brief This is a test for system call installation
 * Coverage IDT, system call
 * OUTPUT PASS/FAIL
 * @return ** int 
 */
int syscall_inspection1(){
    TEST_HEADER;
    int i=0x80;
    int result=PASS;
    if(((idt[i].offset_31_16<<16)|(idt[i].offset_15_00))==0){
        assertion_failure();
        result = FAIL;
    }
    return result;
}
/**
 * @brief This is a test for system call calling
 * Coverage, system call
 * OUTPUT print system call message
 * @return ** int 
 */
int syscall_inspection2(){
    TEST_HEADER;
    asm volatile("int $0x80");
    return PASS;
}

/* Page fault Exception Test
 *
 * Dereference a pointer to an invalid virtual address (PRESENT == 0).
 * Inputs: None
 * Outputs: PASS or Page fault exception message.
 * Side Effects: None
 * Coverage: Paging initialization, pagedir & pgtbl setup.
 * Files: vm.c
 */
int pgfault_test(){
    TEST_HEADER;

    uint32_t data = *(uint32_t *) 3;  // Invalid virtual address!
    printf("data: %u\n", data);
    return FAIL;
}

/* VM Boundary Test 1
 *
 * Dereference pointers to a boundary (invalid) of initialized virtual address.
 * Inputs: None
 * Outputs: FAIL or Page fault exception message.
 * Side Effects: None
 * Coverage: Paging initialization, pagedir & pgtbl setup.
 * Files: vm.c
 */
int vm_bound_test1(){
    TEST_HEADER;
    unsigned char data = *(unsigned char*) (VIDEO - 1);  // One before video memory.

    printf("Successfully dereferenced: %u\n", data);

    return FAIL;
}

/* VM Boundary Test 2
 *
 * Dereference pointers to a boundary (invalid) of initialized virtual address.
 * Inputs: None
 * Outputs: FAIL or Page fault exception message.
 * Side Effects: None
 * Coverage: Paging initialization, pagedir & pgtbl setup.
 * Files: vm.c
 */
int vm_bound_test2(){
    TEST_HEADER;
    unsigned char data = *(unsigned char*) (VIDEO + PGSIZE);  // One past video memory.

    printf("Successfully dereferenced: %u\n", data);

    return FAIL;
}

/* VM Boundary Test 3
 *
 * Dereference pointers to a boundary (invalid) of initialized virtual address.
 * Inputs: None
 * Outputs: FAIL or Page fault exception message.
 * Side Effects: None
 * Coverage: Paging initialization, pagedir & pgtbl setup.
 * Files: vm.c
 */
int vm_bound_test3(){
    TEST_HEADER;
    unsigned char data = *(unsigned char*) ((1U << PDXOFF) - 1);  // One before 4M.

    printf("Successfully dereferenced: %u\n", data);

    return FAIL;
}

/* VM Boundary Test 4
 *
 * Dereference pointers to a boundary (invalid) of initialized virtual address.
 * Inputs: None
 * Outputs: FAIL or Page fault exception message.
 * Side Effects: None
 * Coverage: Paging initialization, pagedir & pgtbl setup.
 * Files: vm.c
 */
int vm_bound_test4(){
    TEST_HEADER;
    unsigned char data = *(unsigned char*) ((1U << PDXOFF) + PGSIZE * NUM_ENT);  // Past video memory.

    printf("Successfully dereferenced: %u\n", data);

    return FAIL;
}

/* VM Sanity Test
 *
 * Dereference pointers to valid virtual addresses.
 * Inputs: None
 * Outputs: PASS or Page fault exception message.
 * Side Effects: None
 * Coverage: Paging initialization, pagedir & pgtbl setup.
 * Files: vm.c
 */
int vm_sanity_test(){
    TEST_HEADER;
    unsigned char* ptr = (void*) VIDEO;  // Starting va of video memory.
    uint32_t sum = 0;
    int i;

    for (i = 0; i < PGSIZE; i++) {  // Test fails when we +/- 1.
        sum += *ptr++;
    }

    ptr = (void*) (1U << PDXOFF);  // Start of direct mapping at 4M.
    for (i = 0; i < PGSIZE * NUM_ENT; i++) {  // Test fails when we +/- 1.
        sum += *ptr++;
    }
    printf("Sum of video mem + 4M~8M: %u\n", sum);  // Overflow may occur.

    return PASS;
}

/* PDE/PTE flag Test
 *
 * Check flags of pde/pte entries.
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Paging initialization, pagedir & pgtbl setup.
 * Files: vm.c
 */
int page_flags_test(){
    TEST_HEADER;
    uint32_t* it = pgdir;  // Start of page directory.
    int i;
    if (!(PAGE_FLAGS(*it++) & (PAGE_P | PAGE_RW))) {
        printf("P/RW not set for PDE #0!\n");
        return FAIL;
    }
    if (!(PAGE_FLAGS(*it++) & (PAGE_P | PAGE_RW | PAGE_PS))) {
        printf("P/RW/PS not set for PDE #1!\n");
        return FAIL;
    }
    for (i = 2; i < NUM_ENT; i++) {
        if (PAGE_FLAGS(*it++) & PAGE_P) {
            printf("P incorrectly set for PDE #%d\n", i);
            return FAIL;
        }
    }

    it = pgtbl;  // Start of the first page table.
    for (i = 0; i < PTX(VIDEO); i++) {
        if (PAGE_FLAGS(*it++) & PAGE_P) {
            printf("P incorrectly set for PDE #0 --> PTE #%d\n", i);
            return FAIL;
        }
    }
    if (!(PAGE_FLAGS(*it++) & (PAGE_P | PAGE_RW))) {
        printf("P/RW not set for PDE #0 --> PTE #%d!\n", PTX(VIDEO));
        return FAIL;
    }
    for (i++; i < NUM_ENT; i++) {
        if (PAGE_FLAGS(*it++) & PAGE_P) {
            printf("P incorrectly set for PDE #0 --> PTE #%d\n", i);
            return FAIL;
        }
    }
    return PASS;
}

/**
 * @brief rtc test
 * INPUT : NONE
 * OUTPUT : every second, all the screen location is altered by once
 * Coverage : RTC, PIC
 * @return ** int 
 */
void rtc_test(uint32_t x){
    if(x==1023) test_interrupts();
}

/* Checkpoint 2 tests */

/**
 * @brief display dentry item
 * Internal use
 * @param dentry 
 * @return ** void 
 */
static void display_dentry(dentry_t* dentry){
    puts((int8_t*)dentry->filename);
    printf("filetype=%d inode_name=%d\n",dentry->filetype,dentry->inode_num);
}

/**
 * @brief test read dentry using index
 * INPUT : NONE
 * OUTPUT : PASS/FAIL + Display dentry info
 * Coverage: read dentry by index
 * @return ** int32_t 
 */
int32_t filesystem_test_read_dentry1(){
    // TEST_HEADER;

    // int i;
    int result = PASS;
    // uint8_t* buf[100];
    dentry_t ret;
    if(readonly_fs.f_rw.read_dentry_by_index(0,&ret)==-1){
        return FAIL;
    }
    display_dentry(&ret);
    return result;
}
/**
 * @brief test read dentry using name, txt file
 * INPUT : NONE
 * OUTPUT : PASS/FAIL + Display dentry info
 * Coverage: read dentry by name, txt file
 * @return ** int32_t 
 */
int32_t filesystem_test_read_dentry2(){
    // int i;
    int result = PASS;
    // uint8_t* buf[100];
    dentry_t ret;
    if(readonly_fs.f_rw.read_dentry_by_name((uint8_t*)"frame0.txt",&ret)==-1){
        return FAIL;
    }
    display_dentry(&ret);
    return result;
}
/**
 * @brief test read dentry using name, executable
 * INPUT : NONE
 * OUTPUT : PASS/FAIL + Display dentry info
 * Coverage: read dentry by name, executable
 * @return ** int32_t 
 */
int32_t filesystem_test_read_dentry3(){
    // int i;
    int result = PASS;
    // uint8_t* buf[100];
    dentry_t ret;
    if(readonly_fs.f_rw.read_dentry_by_name((uint8_t*)"hello",&ret)==-1){
        return FAIL;
    }
    display_dentry(&ret);
    return result;
}
/**
 * @brief test read file data
 * INPUT : NONE
 * OUTPUT : PASS/FAIL + read length + read info
 * Coverage : read file data, large length, offset
 * @return ** int32_t 
 */
int32_t filesystem_test_read1(){
    int i;
    int result = PASS;
    uint8_t buf[120];
    uint32_t f_len;
    // dentry_t ret;
    if((f_len=readonly_fs.f_rw.read_data(38,40,buf,100))==-1){
        return FAIL;
    }
    printf("read file length : %d\n",f_len);
    for(i=0;i<f_len;i++) putc(buf[i]);
    return result;
}


/**
 * @brief test read file data
 * INPUT : NONE
 * OUTPUT : PASS/FAIL + read length + read info (ELF)
 * Coverage : read file data, zero offset, ELF, executable
 * @return ** int32_t 
 */
int32_t filesystem_test_read2(){
    int i;
    int result = PASS;
    uint8_t buf[120];
    uint32_t f_len;
    // dentry_t ret;
    if((f_len=readonly_fs.f_rw.read_data(10,0,buf,10))==-1){
        return FAIL;
    }
    printf("read file length : %d\n",f_len);
    for(i=0;i<f_len;i++) putc(buf[i]);
    return result;
}
/**
 * @brief test read file data
 * INPUT : NONE
 * OUTPUT : PASS/FAIL + read length + read info (MAGIC NUMBER)
 * Coverage : read file data, zero offset, MAGIC NUMBER, executable
 * @return ** int32_t 
 */
int32_t filesystem_test_read3(){
    int i;
    int result = PASS;
    uint8_t buf[120];
    uint32_t f_len;
    // dentry_t ret;
    if((f_len=readonly_fs.f_rw.read_data(10,5309,buf,40))==-1){
        return FAIL;
    }
    printf("read file length : %d\n",f_len);
    for(i=0;i<f_len;i++) putc(buf[i]);
    return result;
}

/**
 * @brief test open file for file system
 * INPUT : NONE
 * OUTPUT : position of file at initialization, flag, inode number + PASS
 * Coverage : file system driver open
 * @return ** int32_t 
 */
int32_t filesystem_ioctl_test1(){
    fd_t file_descriptor_item;
    int result = PASS;

    if(readonly_fs.openr(&file_descriptor_item,(uint8_t*)".",0)==-1){
        return FAIL;
    }
    printf("pos=%d flag=%d ",file_descriptor_item.file_position,file_descriptor_item.flags);
    printf("inode=%d\n",file_descriptor_item.inode);
    return result;
}
/**
 * @brief test read directory for file system
 * INPUT : NONE
 * OUTPUT : a series of file name within directory
 * Coverage : file system driver read directory
 * @return ** int32_t 
 */
int32_t filesystem_ioctl_test2(){
    int i;
    fd_t file_descriptor_item;
    int result = PASS;
    if(readonly_fs.openr(&file_descriptor_item,(uint8_t*)".",0)==-1){
        return FAIL;
    }
    uint8_t buf[100];
    for(i=0;i<5;i++){
        file_descriptor_item.file_operation_jump_table.read(&file_descriptor_item,buf,32);
        puts((int8_t*)buf);
        putc(' ');
    }
    return result;
}

/**
 * @brief test read file for file system
 * INPUT : NONE
 * OUTPUT : a series of file content within directory
 * Coverage : file system driver read file, with large file name and large file length
 * @return ** int32_t 
 */
int32_t filesystem_ioctl_test3(){
    int i;
    fd_t file_descriptor_item;
    int result = PASS;
    if(readonly_fs.openr(&file_descriptor_item,(uint8_t*)"verylargetextwithverylongname.txt",0)==-1){
        return FAIL;
    }
    uint8_t buf[100];
    for(i=0;i<5;i++){
        file_descriptor_item.file_operation_jump_table.read(&file_descriptor_item,buf,10);
        puts((int8_t*)buf);
        putc(' ');
    }
    return result;
}

/*!
 * @brief This function repeatedly calls `terminal_read` and attempts to read 32 characters.
 * Then it outputs the characters actually read from the `input` buffer.
 * @param None.
 * @return FAIL or does not return (PASS).
 * @sideeffect Modifies input buffer and video memory contents.
 */
int32_t
terminal_io_test(void) {
    const int SIZE = 32;
    uint8_t buf[SIZE];
    int n;

    while (1) {
        n = terminal_read(buf, SIZE);
        printf("`terminal_read`: # read = %d\n", n);
        printf("`terminal_write`: ");
        if (n != terminal_write(buf, n)) {
            assertion_failure();
            return FAIL;
        }
        putc('\n');
    }

    return PASS;
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/**
 * @brief launching test function
 * @param None
 * @return ** void 
 */
void launch_tests(){
    /* Checkpoint 1 tests */
    // TEST_OUTPUT("syscall inspection",syscall_inspection2());
    // TEST_OUTPUT("syscall inspection",syscall_inspection1());
    // TEST_OUTPUT("idt_test",idt_test());
    // TEST_OUTPUT("exception_test",exception_test());
    // TEST_OUTPUT("idt_test", humble_idt_test());
    /* TEST_OUTPUT("pgfault_test", pgfault_test()); */
    /* TEST_OUTPUT("vm_bound_test1", vm_bound_test1()); */
    /* TEST_OUTPUT("vm_bound_test2", vm_bound_test2()); */
    /* TEST_OUTPUT("vm_bound_test3", vm_bound_test3()); */
    /* TEST_OUTPUT("vm_bound_test4", vm_bound_test4()); */
    /* TEST_OUTPUT("vm_sanity_test", vm_sanity_test()); */
    // TEST_OUTPUT("page_flags_test", page_flags_test());

    /* Checkpoint 2 tests */
    // TEST_OUTPUT("filesystem_test_read",filesystem_test_read_dentry1());
    // TEST_OUTPUT("filesystem_test_read",filesystem_test_read_dentry2());
    // TEST_OUTPUT("filesystem_test_read",filesystem_test_read_dentry3());
    // TEST_OUTPUT("filesystem_test_read",filesystem_test_read1());
    // TEST_OUTPUT("filesystem_test_read",filesystem_test_read2());
    // TEST_OUTPUT("filesystem_test_read",filesystem_test_read3());
    // TEST_OUTPUT("filesystem_ioctl_test",filesystem_ioctl_test1());
    // TEST_OUTPUT("filesystem_ioctl_test",filesystem_ioctl_test2());
    // TEST_OUTPUT("filesystem_ioctl_test",filesystem_ioctl_test3());
    TEST_OUTPUT("terminal_io_test", terminal_io_test());
}

