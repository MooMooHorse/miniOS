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

static int32_t rtc_open(file_t* file, const uint8_t* buf, int32_t nbytes);
static int32_t rtc_read(file_t* file, void* buf, int32_t nbytes);
static int32_t rtc_write(file_t* file, const void* buf, int32_t nbytes);
static int32_t rtc_close(file_t* file);
static void do_rtc(uint32_t i);

/**
 * @brief virtualized RTC handler
 * 
 * @param i 
 * @return ** void 
 */
static void
do_rtc(uint32_t i) {
    rtc[i].count++;
}

/**
 * @brief Initialize the RTC
 * Should be ran when CLI is called 
 */
void
rtc_init(void) {
    int i;
    // ENABLE PERIODIC INTERRUPTS
    // NOTE: The default rate is 1024 Hz!
    outb(RTC_REG_B, RTC_PORT);
    char prev = inb(RTC_DATA_PORT);

    outb(RTC_REG_B, RTC_PORT);
    outb(prev | 0x40, RTC_DATA_PORT);  // Turn on bit 6 of register B.

    // INIT RTC's
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
    virt_rtc++;
    int i;
    for (i = 0; i < RTC_num; i++) {
        if (rtc[i].freq > 0 && (virt_rtc % (1024 / rtc[i].freq)) == 0) {
            do_rtc(i);
        }
    }

    send_eoi(RTC_IRQ);

    outb(RTC_REG_C, RTC_PORT);
    inb(RTC_DATA_PORT);  // Discard contents of register C.
}

/**
 * @brief Open RTC by intitializing the file struct table entry, frequency,
 * and tick counter in the rtc_t struct.
 * 
 * INPUT : file_t* file, const uint8_t* buf, int32_t nbytes
 * OUTPUT : 0 on success, -1 on failure
 * SIDE EFFECTS : Insatntiates RTC to usable state.
 * 
 * @param file file struct table entry pointer
 * @param buf Buffer to read from
 * @param nbytes Number of bytes to read
 * @return int32_t 
 */
static int32_t
rtc_open(file_t* file, const uint8_t* buf, int32_t nbytes) {

    // Check file_t pointer validity
    if (NULL == file) {
        printf("RTC Error: Sanity check failed.\n");
        return -1;
    }

    if (nbytes < 0) {
        printf("RTC Error: Invalid index value.\n");
        return -1;
    }

    // unsure if file struct initialization is correct
    file->file_operation_jump_table = rtc[nbytes].ioctl;
    file->inode = nbytes;
    file->file_position = 0;
    file->flags = DESCRIPTOR_ENTRY_RTC | F_OPEN;

    rtc[file->inode].freq = 2; // Default frequency is 2 Hz.

    return 0;
}

/**
 * @brief Read RTC by waiting for the next interrupt based on virtual RTC
 * frequency and tick counter.
 * 
 * INPUT : file_t* file, uint8_t* buf, int32_t nbytes
 * OUTPUT : 0 on success, -1 on failure
 * SIDE EFFECTS : Resets virtual RTC tick counter.
 * 
 * @param file file struct table entry pointer
 * @param buf Buffer to read from
 * @param nbytes Number of bytes to read
 * @return int32_t 
 */
static int32_t
rtc_read(file_t* file, void* buf, int32_t nbytes) {

    // Check fd_t pointer validity
    if (NULL == file) {
        printf("RTC Error: Sanity check failed.\n");
        return -1;
    }

    rtc[file->inode].count = 0;
    // Wait for next interrupt.
    while (rtc[file->inode].count == 0) {
        // Do nothing.
    }

    rtc[file->inode].count = 0;  // Reset counter.

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
 * @param file file struct table entry pointer
 * @param buf Buffer to read from
 * @param nbytes Number of bytes to read (can only be 0,1,2,4)
 * @return int32_t 
 */
static int32_t
rtc_write(file_t* file, const void* buf, int32_t nbytes) {
    uint32_t new_freq;
    // Check fd_t pointer validity
    if (NULL == file) {
        printf("RTC Error: Sanity check failed.\n");
        return -1;
    }
    if (nbytes == 0) return 0;
    else {
        if (nbytes == 1) new_freq = ((uint8_t*) buf)[0];
        else if (nbytes == 2) new_freq = ((uint16_t*) buf)[0];
        else if (nbytes == 4) new_freq = ((uint32_t*) buf)[0];
        else return -1;
    }


    // Check if freq is a power of 2 and in valid range.
    if ((new_freq < 2 || new_freq > 1024) || (new_freq & (new_freq - 1)) != 0) {
        printf("RTC Error: Bad frequency!\n");
        return -1;
    }

    // Set the frequency.
    rtc[file->inode].freq = new_freq;
    return nbytes;
}

/**
 * @brief Close RTC.
 * 
 * INPUT : fd_t* fd
 * OUTPUT : 0 on success, -1 on failure
 * SIDE EFFECTS : None.
 * 
 * @param file
 * @return int32_t 
 */
static int32_t
rtc_close(file_t* file) {
    // Check fd_t pointer validity
    if (NULL == file) {
        printf("RTC Error: Sanity check failed.\n");
        return -1;
    }
    file->file_operation_jump_table.close = NULL;
    file->file_operation_jump_table.read = NULL;
    file->file_operation_jump_table.write = NULL;
    file->file_operation_jump_table.open = NULL;
    file->inode = -1;
    file->file_position = 0;
    file->flags = F_CLOSE;
    return 0;
}




