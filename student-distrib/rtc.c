/**
 * @file rtc.c
 * @author erik
 * @brief RTC related functions, including handler
 * @version 0.1
 * @date 2022-10-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "rtc.h"
#include "i8259.h"
#include "lib.h"
#include "tests.h"
#include "x86_desc.h"

int32_t
fd_sanity_check(fd_t *fd) {
    // Check if fd_t pointer is valid
    if (fd == NULL) {
        printf("RTC Error: File descriptor pointer is NULL.\n");
        return -1;
    }
    return 0;
}


/**
 * @brief Initialize the RTC
 * Should be ran when CLI is called 
 */
void 
rtc_init(void) {
    // ENABLE PERIODIC INTERRUPTS
    // NOTE: The default rate is 1024 Hz!
    outb(RTC_REG_B, RTC_PORT);
    char prev = inb(RTC_DATA_PORT);

    outb(RTC_REG_B, RTC_PORT);
    outb(prev | 0x40, RTC_DATA_PORT);  // Turn on bit 6 of register B.

    // INIT RTC's
    int i;
    for (i = 0; i < RTC_num; i++) {
        rtc[i].ioctl.open = rtc_open;
        rtc[i].ioctl.close = rtc_close;
        rtc[i].ioctl.read = rtc_read;
        rtc[i].ioctl.write = rtc_write;
        rtc[i].freq = -1;
        rtc[i].count = 0;
    }

    // enable IRQ8
    enable_irq(RTC_IRQ);
}

/**
 * @brief RTC interrupt handler
 *
 */
void 
rtc_handler(void) {
    // Handle RTC interrupt.
    // #ifdef RUN_TESTS_RTC
    // rtc_test(virt_rtc);  // Only for testing purposes.
    // #endif
    
    // if (++rtc->freq == RTC_DEF_FREQ){
    //     rtc->freq = 0;  // Reset virtualization counter.
    // }

    int i;
    for (i = 0; i < RTC_num; i++){
        rtc[i].count++;
        if (rtc[i].count > (1024 / rtc[i].freq)) {
            // printf("RESET");
            rtc[i].count = 0;
        }
    }

    send_eoi(RTC_IRQ);

    outb(RTC_REG_C, RTC_PORT);
    inb(RTC_DATA_PORT);  // Discard contents of register C.
}

/**
 * @brief Open RTC by intitializing the file descriptor table entry, frequency,
 * and tick counter in the rtc_t struct.
 * 
 * INPUT : fd_t* fd, const uint8_t* buf, int32_t nbytes
 * OUTPUT : 0 on success, -1 on failure
 * SIDE EFFECTS : Insatntiates RTC to usable state.
 * 
 * @param fd File descriptor table entry pointer
 * @param buf Buffer to read from
 * @param nbytes Number of bytes to read
 * @return int32_t 
 */
int32_t
rtc_open(fd_t* fd, const uint8_t* buf, int32_t nbytes) {

    // Check fd_t pointer validity
    if (fd_sanity_check(fd) == -1) {
        printf("RTC Error: Sanity check failed.\n");
        return -1;
    }

    if (nbytes < 0) {
        printf("RTC Error: Invalid index value.\n");
        return -1;
    }

    // unsure if FD initialization is correct
    fd->file_operation_jump_table = rtc[nbytes].ioctl;
    fd->inode = nbytes;
    fd->file_position = 0;
    fd->flags = DESCRIPTOR_ENTRY_RTC;

    rtc[fd->inode].freq = 2; // Default frequency is 2 Hz.

    return 0;
}

/**
 * @brief Read RTC by waiting for the next interrupt based on virtual RTC
 * frequency and tick counter.
 * 
 * INPUT : fd_t* fd, uint8_t* buf, int32_t nbytes
 * OUTPUT : 0 on success, -1 on failure
 * SIDE EFFECTS : Resets virtual RTC tick counter.
 * 
 * @param fd File descriptor table entry pointer
 * @param buf Buffer to read from
 * @param nbytes Number of bytes to read
 * @return int32_t 
 */
int32_t rtc_read(fd_t* fd, void* buf, int32_t nbytes) {

    // Check fd_t pointer validity
    if (fd_sanity_check(fd) == -1) {
        printf("RTC Error: Sanity check failed.\n");
        return -1;
    }

    rtc[fd->inode].count = 0;
    // Wait for next interrupt.
    while (rtc[fd->inode].count != (1024 / rtc[fd->inode].freq)) {
        // Do nothing.
    }
    
    rtc[fd->inode].count = 0;  // Reset counter.

    return 0;
}

/**
 * @brief Write RTC by setting the virtual RTC frequency. Input frequency must
 * be a power of two between 2 and 1024 Hz.
 * 
 * INPUT : fd_t* fd, const uint8_t* buf, int32_t nbytes
 * OUTPUT : Value written to virtual RTC on success , -1 on failure
 * SIDE EFFECTS : Sets virtual RTC frequency.
 * 
 * @param fd File descriptor table entry pointer
 * @param buf Buffer to read from
 * @param nbytes Number of bytes to read
 * @return int32_t 
 */
int32_t rtc_write(fd_t* fd, void* buf, int32_t nbytes){

    // Check fd_t pointer validity
    if (fd_sanity_check(fd) == -1) {
        printf("RTC Error: Sanity check failed.\n");
        return -1;
    }

    uint32_t new_freq = nbytes;
    
    // Check if freq is a power of 2 and in valid range.
    if ((new_freq < 2 || new_freq > 1024) || (new_freq & (new_freq - 1)) != 0) {
        printf("RTC Error: Bad frequency!\n");
        return -1;
    }

    // Set the frequency.
    rtc[fd->inode].freq = new_freq;
    return nbytes;
}

/**
 * @brief Close RTC.
 * 
 * INPUT : fd_t* fd
 * OUTPUT : 0 on success, -1 on failure
 * SIDE EFFECTS : None.
 * 
 * @param fd 
 * @return int32_t 
 */
int32_t rtc_close(fd_t* fd) {
    // Check fd_t pointer validity
    if (fd_sanity_check(fd) == -1) {
        printf("RTC Error: Sanity check failed.\n");
        return -1;
    }
    return 0;
}




