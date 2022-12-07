#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "rtc.h"
#include "keyboard.h"
#include "terminal.h"
#include "cursor.h"
#include "kmalloc.h"


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

static inline void assertion_failure() {
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
int idt_test() {
    TEST_HEADER;

    int i;
    int result = PASS;
    for (i = 0; i < 10; ++i) {
        if ((idt[i].offset_15_00 == NULL) &&
            (idt[i].offset_31_16 == NULL)) {
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
int humble_idt_test() {
    TEST_HEADER;

    int i;
    int result = PASS;
    for (i = 0; i < 20; i++) {
        if (((idt[i].offset_31_16 << 16) | (idt[i].offset_15_00)) == 0) {
            if (i != 9 && i != 15) return FAIL;
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
int exception_test() {
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
int syscall_inspection1() {
    TEST_HEADER;
    int i = 0x80;
    int result = PASS;
    if (((idt[i].offset_31_16 << 16) | (idt[i].offset_15_00)) == 0) {
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
int syscall_inspection2() {
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
int pgfault_test() {
    TEST_HEADER;

    uint32_t data = *(uint32_t*) 3;  // Invalid virtual address!
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
int vm_bound_test1() {
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
int vm_bound_test2() {
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
int vm_bound_test3() {
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
int vm_bound_test4() {
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
int vm_sanity_test() {
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
int page_flags_test() {
    TEST_HEADER;
    uint32_t * it = kpgdir;  // Start of page directory.
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

    it = (uint32_t*) PAGE_ADDR(kpgdir[0]);  // Start of the first page table.
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
void rtc_test(uint32_t x) {
    if (x == 1023) test_interrupts();
}

/* Checkpoint 2 tests */

/**
 * @brief display dentry item
 * Internal use
 * @param dentry 
 * @return ** void 
 */
static void display_dentry(dentry_t* dentry) {
    puts((int8_t*) dentry->filename);
    printf("filetype=%d inode_name=%d\n", dentry->filetype, dentry->inode_num);
}

/**
 * @brief test read dentry using index
 * INPUT : NONE
 * OUTPUT : PASS/FAIL + Display dentry info
 * Coverage: read dentry by index
 * @return ** int32_t 
 */
int32_t filesystem_test_read_dentry1() {
    // TEST_HEADER;

    // int i;
    int result = PASS;
    // uint8_t* buf[100];
    dentry_t ret;
    if (fs.f_rw.read_dentry_by_index(0, &ret) == -1) {
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
int32_t filesystem_test_read_dentry2() {
    // int i;
    int result = PASS;
    // uint8_t* buf[100];
    dentry_t ret;
    if (fs.f_rw.read_dentry_by_name((uint8_t*) "frame0.txt", &ret) == -1) {
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
int32_t filesystem_test_read_dentry3() {
    // int i;
    int result = PASS;
    // uint8_t* buf[100];
    dentry_t ret;
    if (fs.f_rw.read_dentry_by_name((uint8_t*) "hello", &ret) == -1) {
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
int32_t filesystem_test_read1() {
    int i;
    int result = PASS;
    uint8_t buf[120];
    uint32_t f_len;
    // dentry_t ret;
    if ((f_len = fs.f_rw.read_data(38, 40, buf, 100)) == -1) {
        return FAIL;
    }
    printf("read file length : %d\n", f_len);
    for (i = 0; i < f_len; i++) putc(buf[i]);
    return result;
}

/**
 * @brief test read file data
 * INPUT : NONE
 * OUTPUT : PASS/FAIL + read length + read info (ELF)
 * Coverage : read file data, zero offset, ELF, executable
 * @return ** int32_t 
 */
int32_t filesystem_test_read2() {
    int i;
    int result = PASS;
    uint8_t buf[120];
    uint32_t f_len;
    // dentry_t ret;
    if ((f_len = fs.f_rw.read_data(10, 0, buf, 10)) == -1) {
        return FAIL;
    }
    printf("read file length : %d\n", f_len);
    for (i = 0; i < f_len; i++) putc(buf[i]);
    return result;
}
/**
 * @brief test read file data
 * INPUT : NONE
 * OUTPUT : PASS/FAIL + read length + read info (MAGIC NUMBER)
 * Coverage : read file data, zero offset, MAGIC NUMBER, executable
 * @return ** int32_t 
 */
int32_t filesystem_test_read3() {
    int i;
    int result = PASS;
    uint8_t buf[120];
    uint32_t f_len;
    // dentry_t ret;
    if ((f_len = fs.f_rw.read_data(10, 5309, buf, 40)) == -1) {
        return FAIL;
    }
    printf("read file length : %d\n", f_len);
    for (i = 0; i < f_len; i++) putc(buf[i]);
    return result;
}
/**
 * @brief test open file for file system
 * INPUT : NONE
 * OUTPUT : position of file at initialization, flag, inode number + PASS
 * Coverage : file system driver open file, file close
 * @return ** int32_t 
 */
int32_t filesystem_ioctl_test0() {
    file_t file;
    int result = PASS;

    if (fs.openr(&file, (uint8_t*) "fish", 0) == -1) {
        return FAIL;
    }
    printf("pos=%d flag=%d ", file.pos, file.flags);
    printf("inode=%d\n", file.inode);
    if (-1 == fs.f_ioctl.close(&file)) {
        return FAIL;
    }
    return result;
}

/**
 * @brief test open file for file system
 * INPUT : NONE
 * OUTPUT : position of file at initialization, flag, inode number + PASS
 * Coverage : file system driver open, directory close
 * @return ** int32_t 
 */
int32_t filesystem_ioctl_test1() {
    file_t file;
    int result = PASS;

    if (fs.openr(&file, (uint8_t*) ".", 0) == -1) {
        return FAIL;
    }
    printf("pos=%d flag=%d ", file.pos, file.flags);
    printf("inode=%d\n", file.inode);
    if (-1 == fs.d_ioctl.close(&file)) {
        return FAIL;
    }
    return result;
}
/**
 * @brief test read directory for file system
 * INPUT : NONE
 * OUTPUT : a series of file name within directory
 * Coverage : file system driver read directory
 * @return ** int32_t 
 */
int32_t filesystem_ioctl_test2() {
    int i;
    file_t file;
    int result = PASS;
    if (fs.openr(&file, (uint8_t*) ".", 0) == -1) {
        return FAIL;
    }
    uint8_t buf[100];
    dentry_t dentry;
    for (i = 0; i < fs.file_num; i++) {
        file.fops.read(&file, buf, 32);
        puts((int8_t*) buf);
        fs.f_rw.read_dentry_by_name((uint8_t*) buf, &dentry);
        printf("          filetype=%d    inode=%d\n", dentry.filetype, dentry.inode_num);
    }
    return result;
}

/**
 * @brief test read file for file system
 * INPUT : NONE
 * OUTPUT : a series of file content within directory
 * Coverage : file system driver read file, with large file name and large file length file close
 * @return ** int32_t 
 */
int32_t filesystem_ioctl_test3() {
    int i;
    file_t file;
    int result = PASS;
    if (fs.openr(&file, (uint8_t*) "verylargetextwithverylongname.tx", 0) == -1) {
        return FAIL;
    }
    uint8_t buf[40001];
    uint32_t nbytes_read;
    nbytes_read = file.fops.read(&file, buf, 40000);
    if (nbytes_read == -1) {
        return FAIL;
    }
    printf("%u\n", nbytes_read);
    for (i = 0; i < nbytes_read; i++) putc(buf[i]);
    if (fs.f_ioctl.close(&file) == -1) {
        return FAIL;
    }
    return result;
}
/**
 * @brief test read executable tail
 * INPUT : NONE
 * OUTPUT : a series of file content within executable file +  PASS/FAIL
 * Coverage : file system driver read file, with executable
 * @return ** int32_t 
 */
int32_t filesystem_ioctl_test4() {
    int i;
    file_t file;
    int result = PASS;
    if (fs.openr(&file, (uint8_t*) "hello", 0) == -1) {
        return FAIL;
    }
    uint8_t buf[40001];
    uint32_t nbytes_read;
    nbytes_read = file.fops.read(&file, buf, 40000);
    if (nbytes_read == -1) {
        return FAIL;
    }
    printf("%u\n", nbytes_read);
    for (i = 0; i < nbytes_read; i++) putc(buf[i]);
    //     putc(' ');
    // }
    return result;
}
/**
 * @brief test read executable tail
 * INPUT : NONE
 * OUTPUT : a series of file content within executable file+  PASS/FAIL
 * Coverage : file system driver read file, with executable
 * @return ** int32_t 
 */
int32_t filesystem_ioctl_test5() {
    int i;
    file_t file;
    int result = PASS;
    if (fs.openr(&file, (uint8_t*) "hello", 0) == -1) {
        return FAIL;
    }
    uint8_t buf[40001];
    uint32_t nbytes_read;
    nbytes_read = file.fops.read(&file, buf, 20);
    if (nbytes_read == -1) {
        return FAIL;
    }
    printf("%u\n", nbytes_read);
    for (i = 0; i < nbytes_read; i++) putc(buf[i]);
    //     putc(' ');
    // }
    return result;
}

/**
 * @brief test read file for file system
 * INPUT : NONE
 * OUTPUT : PASS/FAIL
 * Coverage : file system driver read file, with large file name and large file length with illegal input filename
 * @return ** int32_t 
 */
int32_t filesystem_ioctl_test6() {
    int i;
    file_t file;
    if (fs.openr(&file, (uint8_t*) "verylargetextwithverylongname.txt", 0) == -1) {
        return PASS;
    }
    uint8_t buf[40001];
    uint32_t nbytes_read;
    nbytes_read = file.fops.read(&file, buf, 40000);
    if (nbytes_read == -1) {
        return FAIL;
    }
    printf("%u\n", nbytes_read);
    for (i = 0; i < nbytes_read; i++) putc(buf[i]);
    return FAIL;
}

/**
 * @brief test read file for file system
 * INPUT : NONE
 * OUTPUT : a series of file content within directory
 * Coverage : file system driver read file
 * @return ** int32_t 
 */
int32_t filesystem_ioctl_test7() {
    int i;
    file_t file;
    int result = PASS;
    if (fs.openr(&file, (uint8_t*) "frame0.txt", 0) == -1) {
        return FAIL;
    }
    uint8_t buf[40001];
    uint32_t nbytes_read;
    nbytes_read = file.fops.read(&file, buf, 40000);
    if (nbytes_read == -1) {
        return FAIL;
    }
    printf("%u\n", nbytes_read);
    for (i = 0; i < nbytes_read; i++) putc(buf[i]);
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
    const int32_t SIZE = 32;
    file_t file;  // Unused.
    uint8_t buf[SIZE];
    int n;

    if (0 != terminal_open(&file, NULL, 0)) {  // Unused parameters.
        return FAIL;  // Failed to open the terminal.
    }

    while (1) {
        n = terminal_read(&file, buf, SIZE);
        printf("`terminal_read`: # read = %d\n", n);
        printf("`terminal_write`: ");
        if (n != terminal_write(&file, buf, n)) {
            assertion_failure();
            return FAIL;
        }
        putc('\n');
    }

    if (0 != terminal_close(&file)) {  // Unused parameters.
        return FAIL;  // Failed to close the terminal.
    }

    return PASS;
}

/**
 * @brief Test the RTC write and read functions by running a print test with
 * increasing frequency.
 * 
 * INPUT : NONE
 * OUTPUT : PASS/FAIL
 * SIDE EFFECT : Virtual RTC 0 will be instantiated.
 * @return int32_t 
 */
int32_t rtc_test_open_read() {
    file_t file;

    rtc[0].ioctl.open(&file, (uint8_t*) "RTC0", 0); /* second arg discarded so arbitrary */
    uint32_t buf[100];

    int i, j, k;
    k = 0;

    // iterate through different frequencies
    for (i = 1; i < 10; i++) {
        // loop 10 times
        for (j = 0; j < 10; j++) {
            rtc[0].ioctl.read(&file, buf, 0);
            printf("!");
        }
        printf("\n");
        buf[0] = (2 << i);
        rtc[0].ioctl.write(&file, buf, 4);
    }

    rtc[0].ioctl.close(&file);

    return PASS;
}

/**
 * @brief Test the RTC write function with legal and illegal frequencies.
 * 
 * INPUT : NONE
 * OUTPUT : PASS/FAIL
 * SIDE EFFECT : Virtual RTC 0 will be instantiated.
 * @return int32_t 
 */
int32_t rtc_test_write() {
    file_t file;
    rtc[0].ioctl.open(&file, (uint8_t*) "RTC0", 0);
    uint32_t buf[100];

    printf("RTC test write 2 Hz\n");
    buf[0] = 2;
    int32_t test1 = rtc[0].ioctl.write(&file, buf, 4);

    printf("RTC test write 2048 Hz\n");
    buf[0] = 2048;
    int32_t test2 = rtc[0].ioctl.write(&file, buf, 4);

    printf("RTC test write 31 Hz\n");
    buf[0] = 31;
    int32_t test3 = rtc[0].ioctl.write(&file, buf, 4);

    // Test 1 should pass, all else should fail.
    if (test1 != -1 || test2 == -1 || test3 == -1) {
        return PASS;
    } else {
        return FAIL;
    }

    rtc[0].ioctl.close(&file);

    return 0;
}

/**
 * @brief Test sanity check for the RTC driver.
 * 
 * INPUT : NONE
 * OUTPUT : PASS/FAIL
 * SIDE EFFECT : Virtual RTC 0 will be instantiated.
 * 
 */
int32_t rtc_sanity_check() {

    printf("NULL FILE STRUCT TEST\n");
    int32_t test1 = rtc[0].ioctl.open(NULL, (uint8_t*) "RTC0", 0);
    uint8_t buf[100];
    buf[0] = 4;
    int32_t test2 = rtc[0].ioctl.read(NULL, buf, 0);
    int32_t test3 = rtc[0].ioctl.write(NULL, buf, 4);
    int32_t test4 = rtc[0].ioctl.close(NULL);

    if (test1 == -1 && test2 == -1 && test3 == -1 && test4 == -1) {
        printf("We have gone insane. I mean...sanity checks blocked bad inputs...which is good.\n");
        return PASS;
    } else {
        printf("We are sane. Sanity checks allowed bad inputs...which is bad.\n");
        return FAIL;
    }
}


/* Checkpoint 3 tests */

/**
 * @brief Try to have a divide by zero exception
 * @param none
 * Input : none
 * Output : If shell activates normally, then squashing program for exception suceed, otherwise
 * it failed. 
 * @return ** not important 
 */
int32_t
exception_squash_program_test() {
    volatile int x = 0;
    return 1 / x;
}

/**
 * @brief test if terminal open success
 * 
 * @param checked_item - terminal STDIN
 * SIDE-EFFECT : print 
 * @return ** void 
 */
void test_open(file_t* checked_item) {
    if (checked_item->flags & F_OPEN) {
        printf("open success\n");
    } else {
        printf("open failed\n");
    }
}
/**
 * @brief test if terminal close success
 * 
 * @param checked_item - termina STDOUT
 * SIDE-EFFECT : print 
 * @return ** void 
 */
void test_close(file_t* checked_item) {
    if (checked_item->flags == F_CLOSE) {
        printf("close success\n");
    } else {
        printf("close failed\n");
    }
}

/**
 * @brief test if file creation succeed
 * OUTPUT: PASS/FAIL
 * @return ** int32_t 
 * PASS on success
 * FAIL on failure
 */
int32_t test_file_create() {
    if (-1 == fs.f_rw.create_file((uint8_t*) "testcreate.txt", strlen("testcreate.txt"))) {
        return FAIL;
    } else {
        return PASS;
    }
}

/**
 * @brief test if file removal succeed
 * OUTPUT: PASS/FAIL
 * @return ** int32_t 
 * PASS : when there is file to remove -> success
 * when there is none -> fail
 * FAIL : when there is file to remove -> fail
 * when there is none -> success
 */
int32_t test_remove_file() {
    if (-1 == fs.f_rw.remove_file((uint8_t*) "testcreate.txt", strlen("testcreate.txt"))) {
        return FAIL;
    } else {
        return PASS;
    }
}

/**
 * @brief test if file rename succeed
 * OUTPUT: PASS/FAIL
 * @return ** int32_t 
 * PASS : when there is file to rename -> success
 * when there is none -> fail
 * FAIL : when there is file to rename -> fail
 * when there is none -> success
 */
int32_t test_rename_file() {
    if (-1 == fs.f_rw.rename_file((uint8_t*) "testcreate.txt", (uint8_t*) "haha.txt", strlen("haha.txt"))) {
        return FAIL;
    } else {
        return PASS;
    }
}

/**
 * @brief test write file success or not
 * OUTPUT: PASS/FAIL
 * @return ** int32_t 
 */
int32_t test_write_file() {
    int32_t i = 0;
    if (-1 == fs.f_rw.create_file((uint8_t*) "testcreate.txt", strlen("testcreate.txt"))) {
        return FAIL;
    }
    file_t file;
    if (-1 == fs.openr(&file, (uint8_t*) "testcreate.txt", 0)) {
        return FAIL;
    }
    for (i = 0; i < 4000; i++) {
        if (-1 == fs.f_rw.write_data(file.inode,
                                     i * strlen("hello,world\n"),
                                     (uint8_t*) "hello,world\n",
                                     strlen("hello,world\n"))) {
            return FAIL;
        }
    }
    return PASS;
}

int cursor_test(void) {
    uint16_t pos;
    pos = get_cursor();
    printf("cursor position: (%u, %u)\n", pos % VGA_WIDTH, pos / VGA_WIDTH);
    set_cursor(37, 63);
    pos = get_cursor();
    if (37 == pos % VGA_WIDTH && 63 == pos / VGA_WIDTH) {
        return PASS;
    }
    assertion_failure();
    return FAIL;
}

int bool_test(void) {
    bool b = false;
    if (b) {
        assertion_failure();
        return FAIL;
    }
    b = true;
    if (!b) {
        assertion_failure();
        return FAIL;
    }
    printf("Size of bool: %u\n", sizeof(b));
    return PASS;
}

int rand_test(void) {
    int32_t i;
    for (i = 0; i < 20; ++i) {
        printf("%u\n", rand());
    }
    return PASS;
}

/*!
 * @brief Unit test for buddy allocator.
 */
int buddy_allocator_init_test(void) {
    buddy_init((void*) 0x4000000, 4 << 20, 1);
    buddy_init((void*) 0x4000000, 1 << 20, 1);
    // Succeeded if no panics.
    return PASS;
}

int buddy_block_split_test(void) {
    buddy_init((void*) 0x4000000, 4 << 20, 1);
    buddy_traverse();
    buddy_split(buddy_allocator.start, 1 << 18);
    buddy_traverse();

    // Succeeded if no panics & correct statistics.
    return PASS;
}

int buddy_block_search_test(void) {
    buddy_init((void*) 0x4000000, 4 << 20, 1);
    (void) buddy_search(1 << 18);
    buddy_block_t* res = buddy_search(1 << 17);
    buddy_traverse();
    printf("\nres: \n", res);
    buddy_print_block(res);
    printf("\n");

    // Succeeded if no panics & correct statistics.
    return PASS;
}

int buddy_block_coalesce_test(void) {
    uint32_t i;
    buddy_init((void*) 0x4000000, 4 << 20, 1);
    for (i = 0; i < 10000; ++i) {
        (void) buddy_search(rand() % ((1 << 20) + 1 - (4 << 10)) + (4 << 10));
    }
    buddy_traverse();
    buddy_coalesce();
    buddy_traverse();

    // Succeeded if no panics & correct statistics.
    return PASS;
}

int buddy_malloc_stress_test(void) {
    uint32_t i, j;
    uint32_t alloc_cnt = 0;
    uint32_t dealloc_cnt = 0;
    static const int32_t round_size = 1000;  // Cannot be too large -- the PCB may get corrupted.
    static const int32_t round_cnt = 1000;  // Stress test.
    void* res[round_size];

    for (j = 0; j < round_cnt; ++j) {
        memset(res, NULL, sizeof(res) / sizeof(res[0]));
        for (i = 0; i < round_size; ++i) {
            if (NULL != (res[i] = buddy_malloc(rand() % ((4 << 19) + 1 - (1 << 10)) + (1 << 10)))) {
                alloc_cnt++;
            }
        }
        for (i = 0; i < round_size; ++i) {
            if (NULL == res[i]) {
                continue;
            }
            if (0 == buddy_free(res[i])) {
                dealloc_cnt++;
            }
        }
        printf("\nIn round %d:\n", j);
        printf("Successfully allocated %d blocks of memory.\n", alloc_cnt);
        printf("Successfully reclaimed %d blocks of memory.\n", dealloc_cnt);

        if (alloc_cnt != dealloc_cnt) {
            assertion_failure();
            return FAIL;
        }
    }
//    buddy_coalesce();
//    buddy_traverse();
    // Succeeded if no panics & correct statistics.
    return PASS;
}

/**
 * @brief launching test function
 * @param None
 * @return ** void 
 */
void launch_tests() {
    // launch your tests here
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
    // TEST_OUTPUT("filesystem_ioctl_test",filesystem_ioctl_test0());
    // TEST_OUTPUT("filesystem_ioctl_test",filesystem_ioctl_test1());
    // TEST_OUTPUT("filesystem_ioctl_test",filesystem_ioctl_test2());
    // TEST_OUTPUT("filesystem_ioctl_test",filesystem_ioctl_test3());
    // TEST_OUTPUT("filesystem_ioctl_test",filesystem_ioctl_test4());
    // TEST_OUTPUT("filesystem_ioctl_test",filesystem_ioctl_test5());
    // TEST_OUTPUT("filesystem_ioctl_test",filesystem_ioctl_test6());
    // TEST_OUTPUT("filesystem_ioctl_test",filesystem_ioctl_test7());
    // TEST_OUTPUT("terminal_io_test", terminal_io_test());
    // TEST_OUTPUT("rtc_test_open_read", rtc_test_open_read());
    // TEST_OUTPUT("rtc_test_write", rtc_test_write());
    // TEST_OUTPUT("rtc_sanity_check", rtc_sanity_check());
    // TEST_OUTPUT("test_create_file",test_file_create());
    // TEST_OUTPUT("test_rename_file",test_rename_file());
    // TEST_OUTPUT("test_remove_file",test_remove_file());
    // TEST_OUTPUT("test_write_file",test_write_file());
    // TEST_OUTPUT("exception_squash_program_check", exception_squash_program_test());
    /* TEST_OUTPUT("cursor_test", cursor_test()); */
    // TEST_OUTPUT("bool_test", bool_test());
    // TEST_OUTPUT("rand_test", rand_test());
    // TEST_OUTPUT("buddy_allocator_init_test", buddy_allocator_init_test());
    // TEST_OUTPUT("buddy_block_split_test", buddy_block_split_test());
    // TEST_OUTPUT("buddy_block_search_test", buddy_block_search_test());
    // TEST_OUTPUT("buddy_block_coalesce_test", buddy_block_coalesce_test());
    TEST_OUTPUT("buddy_malloc_stress_test", buddy_malloc_stress_test());
}

