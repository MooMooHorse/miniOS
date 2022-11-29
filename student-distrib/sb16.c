#include "sb16.h"
#include "i8259.h"
#include "lib.h"
#include "tests.h"
#include "x86_desc.h"

volatile uint8_t sb16_busy = 0;
volatile uint8_t sb16_interrupted = 0;

static int32_t sb16_open(file_t* file, const uint8_t* buf, int32_t nbytes);
static int32_t sb16_read(file_t* file, void* buf, int32_t nbytes);
static int32_t sb16_write(file_t* file, const void* buf, int32_t nbytes);
static int32_t sb16_close(file_t* file);

/**
 * @brief Initialize the SB16 driver, enable DMA playback, and enable
 * interrupts.
 * 
 * @return int32_t Retun code; 0 for success, -1 for failure.
 * SIDE EFFECTS : Enables DMA channel 1, enables IRQ 6, and sets the
 *              sample rate.
 */
int32_t sb16_init(void) {
    // cli();

    // Only one instance can exist at once.
    if (sb16_busy) {
        // sti();
        return -1;
    }

    // RESETTING DSP

    outb(1, SB16_RESET_PORT);

    // wait for reset
    // how do we do that LMAO?? for loop and given estimated clock speed?

    outb(0, SB16_RESET_PORT);

    // Check if card is initialized
    // If SB16_READY status code is not given, abort!
    uint8_t data = inb(SB16_READ_PORT);
    if (data != SB16_READY) {
        // sti();
        return -1;
    }

    // Set busy flag
    sb16_busy = 1;

    // SETTING IRQ
    enable_irq(SB16_IRQ);

    // PROGRAMMING DMA (8 BIT)
    outb(0x01 + 0x04, 0x0A);
    outb(0x01, 0x0C);
    outb(0x0B, 0x48 + 0x01);
    // NEED TO CHECK IF USING PAGE CORRECTLY
    outb((uint8_t)SB16_PAGE_ADDRESS >> 16, 0x83);
    outb((uint8_t)SB16_PAGE_ADDRESS, 0x02);
    outb((uint8_t)SB16_PAGE_ADDRESS >> 8, 0x02);

    outb((uint8_t)(SB16_DATA_LENGTH), 0x03);
    outb((uint8_t)(SB16_DATA_LENGTH) >> 8, 0x03);


    // ENABLE CHANNEL 1
    outb(0x01, 0x0A);

    // PROGRAMMING DMA (16 BIT)
    outb(0x01 + 0x04, 0xD4);
    outb(0x01, 0xD8);
    outb(0x48 + 0x01, 0xD6);
    // NEED TO CHECK IF USING PAGE CORRECTLY
    outb((uint8_t)SB16_PAGE_ADDRESS >> 16, 0x8B);
    outb((uint8_t)SB16_PAGE_ADDRESS, 0xC4);
    outb((uint8_t)SB16_PAGE_ADDRESS >> 8, 0xC4);

    outb((uint8_t)(SB16_DATA_LENGTH), 0xC6);
    outb((uint8_t)(SB16_DATA_LENGTH) >> 8, 0xC6);

    return 0;
}

/**
 * @brief Same as init().
 * 
 * @return int32_t Return code from init().
 */
int32_t sb16_open(file_t* file, const uint8_t* buf, int32_t nbytes) {
    return sb16_init();
}

/**
 * @brief Waits for next SB16 interrupt.
 * 
 * @param inode unused
 * @param buf unused
 * @param nbytes unused
 * @return int32_t Return code; 0 for success, -1 for failure.
 */
int32_t sb16_read(file_t* file, void* buf, int32_t nbytes) {
    if (!sb16_busy) {
        // SB_16 must be initialized before read
        return -1;
    }

    uint8_t prev_id = sb16_interrupted;

    // Wait for interrupt
    while (prev_id == sb16_interrupted) {
        // Do nothing
    }

    return 1;
}

/**
 * @brief Write data to the SB16 buffer to be read.
 * 
 * @param file unused
 * @param buf first byte of buf contains offset, rest of buf contains data
 * @param nbytes size of data transferred
 * @return int32_t 
 */
int32_t sb16_write(file_t* file, const void* buf, int32_t nbytes) {
    // SANITY CHECKS
    if (!sb16_busy) {
        // if sb16 is not initialized, abort!
        return -1;
    }

    // if there is no data in the buffer, abort!
    if (buf == NULL) {
        return -1;
    }

    if (nbytes > SB16_DATA_LENGTH) {
        // if the data is too long, abort!
        return -1;
    } else {
        // copy data to buffer
        memcpy((uint8_t*)SB16_PAGE_ADDRESS, buf, nbytes);
    }

    return nbytes;
}

/**
 * @brief Close the SB16 driver.
 * 
 * @param file unused
 * @return int32_t Return code; 0 for success, -1 for failure.
 * SIDE EFFECTS : SB16 is no longer considered busy.
 */
int32_t sb16_close(file_t* file) {
    if (!sb16_busy) {
        // SB16 must be initialized before close
        return -1;
    }
    sb16_busy = 0;

    return 0;
}

void sb16_interrupt_handler() {
    sb16_interrupted++;     // increment interrupt counter
    inb(SB16_READ_PORT);    // dispose of data
    send_eoi(SB16_IRQ);     // send EOI
}

int32_t sb16_command(int32_t command, int32_t argument) {
    // SANITY CHECKS
    if (!sb16_busy) {
        // if sb16 is not initialized, abort!
        return -1;
    }

    switch (command) {
        case SB16_16_BIT_PAUSE:
            // 8-bit pause
            outb(SB16_16_BIT_PAUSE, SB16_WRITE_PORT);
            break;
        
        case SB16_16_BIT_RESUME:
            // 8-bit resume
            outb(SB16_16_BIT_RESUME, SB16_WRITE_PORT);
            break;

        case SB16_MASTER_VOLUME:
            // master volume
            outb(SB16_MASTER_VOLUME, SB16_WRITE_PORT);
            outb(argument, SB16_WRITE_PORT);
            break;
        default:
            return -1;
    }

    return 0;
}