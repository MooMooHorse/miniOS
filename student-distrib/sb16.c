#include "sb16.h"
#include "i8259.h"
#include "lib.h"
#include "tests.h"
#include "x86_desc.h"

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
void sb16_init(void) {
    // Only one instance can exist at once.
    if (sb16.sb16_busy) {
        return;
    }

    sb16.sb16_busy = -1;

    // RESETTING SB16
    // (OSDEV) 1.1 Send 1 to DSP reset port 
    // (OSDEV) 1.2 Wait 3 us (skipped)
    // (OSDEV) 1.3 Send 0 to DSP reset port 
    outb(0x1, SB16_RESET_PORT);
    outb(0x0, SB16_RESET_PORT);

    // CHECK CARD INITIALIZATION
    // (OSDEV) 1.4 Now should be in DSP read port 0xAA. 
    // If SB16_READY status code is not given, abort!
    uint8_t data = inb(SB16_READ_PORT);
    if (data != SB16_READY) {
        return;
    }

    // SETTING IRQ
    // (OSDEV) 2.1 Send 0x80 to Mixer port 
    // (OSDEV) 2.2 Send value of your IRQ (0x02) to Mixer data port
    outb(0x80, SB16_MIXER_PORT);
    outb(0x02, SB16_MIXER_DATA_PORT);
    enable_irq(SB16_IRQ); // set IRQ 5

    // PROGRAMMING DMA (8 BIT)
    // (OSDEV) 3.1 Disable channel by writing to port 0x0A value 0x05 (channel number + 0x04) 
    // (OSDEV) 3.2 Write value to flip-flop port 0x0C (any value e.g. 1) 
    outb(0x01 | 0x04, 0x0A);
    outb(0xFF, 0x0C);

    // Set Address
    // (OSDEV) 3.3 Send transfer mode to 0x0B (0x48 for single mode/0x58 for auto mode + channel number) 
    outb(0x58 + 0x01, 0x0B);
    
    // (OSDEV) 3.4 Send page number to 0x83(page port of channel 1) For example if you have sound data at 0x100450, page is 0x10
    // (OSDEV) 3.5 Send low bits of position to port 0x02(addr. port of channel 1) For example(see above) is 0x50.
    // (OSDEV) 3.6 Send high bits of position to port 0x02(addr. port of channel 1) For example(see above) is 0x04. 
    outb((uint8_t) (SB16_PAGE_ADDRESS >> 16) & 0xFF, 0x83);
    outb((uint8_t) (SB16_PAGE_ADDRESS) & 0xFF, 0x02);
    outb((uint8_t) (SB16_PAGE_ADDRESS >> 8) & 0xFF, 0x02);

    // (OSDEV) 3.7 Send low bits of length of data to port 0x03(count port of channel 1) For example if is length 0x0FFF, send 0xFF
    // (OSDEV) 3.8 Send high bits of length of data to port 0x03(count port of channel 1) For example if is length 0x0FFF, send 0x0F 
    outb((uint8_t) (SB16_DATA_LENGTH - 1) & 0xFF, 0x03);
    outb((uint8_t) ((SB16_DATA_LENGTH - 1) >> 8) & 0xFF, 0x03);


    // ENABLE CHANNEL 1
    // (OSDEV) 3.9 Enable channel by writing channel number to port 0x0A 
    outb(0x01, 0x0A);

    // // PROGRAMMING DMA (16 BIT)
    // outb(0x01 + 0x04, 0xD4);
    // outb(0xFF, 0xD8);
    // outb(0x48 + 0x01, 0xD6);

    // // Set page address
    // outb(((uint8_t)SB16_PAGE_ADDRESS >> 16) & 0xFF, 0x8B);
    // outb((uint8_t)SB16_PAGE_ADDRESS & 0xFF, 0xC4);
    // outb(((uint8_t)SB16_PAGE_ADDRESS >> 8) & 0xFF, 0xC4);

    // // Set data length
    // outb((uint8_t)(SB16_DATA_LENGTH) & 0xFF, 0xC6);
    // outb(((uint8_t)(SB16_DATA_LENGTH) >> 8) & 0xFF, 0xC6);

    // // ENABLE CHANNEL 1
    // outb(0x01, 0xD4);

    // Initialize SB16 struct with IOCTL and flags
    sb16.ioctl.open = sb16_open;
    sb16.ioctl.read = sb16_read;
    sb16.ioctl.write = sb16_write;
    sb16.ioctl.close = sb16_close;
    sb16.sb16_busy = 0;
    sb16.sb16_interrupted = 0;

    return;
}

/**
 * @brief Attaches relevant driver functions and marks device as active.
 * 
 * @return int32_t Return code from init().
 */
int32_t sb16_open(file_t* file, const uint8_t* buf, int32_t nbytes) {

    if (sb16.sb16_busy != 0) {
        return -1;
    }

    file->fops = sb16.ioctl;
    file->inode = 0;
    file->pos = 0;
    file->flags = DESCRIPTOR_ENTRY_SB16 | F_OPEN;
    // not sure how to instantiate file struct

    sb16.sb16_busy = 1;

    return 0;
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
    sti();
    if (!sb16.sb16_busy) {
        // SB_16 must be initialized before read
        return -1;
    }

    int32_t prev_id = sb16.sb16_interrupted;

    // Wait for interrupt
    while (prev_id == sb16.sb16_interrupted) {
        // do nothing
    }

    return 0;
}

/**
 * @brief Write data to the SB16 buffer to be read.
 * 
 * @param file unused
 * @param buf buf contains data
 * @param nbytes size of data transferred
 * @return int32_t 
 */
int32_t sb16_write(file_t* file, const void* buf, int32_t nbytes) {
    // SANITY CHECKS
    if (!sb16.sb16_busy) {
        // if sb16 is not initialized, abort!
        return -1;
    }

    // if there is no data in the buffer, abort!
    if (buf == NULL) {
        return -2;
    }

    if (nbytes > SB16_DATA_LENGTH) {
        // if the data is too long, abort!
        return -3;
    }

    // copy data to buffer
    memcpy((uint8_t*)SB16_PAGE_ADDRESS, (char*)buf, nbytes);

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
    if (!sb16.sb16_busy) {
        // SB16 must be initialized before close
        return -1;
    }

    file->fops.close = NULL;
    file->fops.read = NULL;
    file->fops.write = NULL;
    file->fops.open = NULL;
    file->inode = -1;
    file->pos = 0;
    file->flags = F_CLOSE;
    
    sb16.sb16_busy = 0;
    sb16.sb16_interrupted = 0;
    
    // outb(0x41, SB16_WRITE_PORT);

    return 0;
}

void sb16_handler(void) {
    // inb(SB16_INT_ACK_PORT);    // dispose of data
    inb(SB16_READ_STATUS_PORT);    // dispose of data
    sb16.sb16_interrupted++;     // increment interrupt counter
    send_eoi(SB16_IRQ);     // send EOI
}

int32_t sb16_command(int32_t command, int32_t argument) {
    // SANITY CHECKS
    if (!sb16.sb16_busy) {
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

        case SB16_SET_TIME_CONST:
            // set time constant
            outb(SB16_SET_TIME_CONST, SB16_WRITE_PORT);
            outb(argument, SB16_WRITE_PORT);
            break;

        case SB16_SET_SAMPLE_RATE:
            // set sample rate
            outb(SB16_SET_SAMPLE_RATE, SB16_WRITE_PORT);
            // need filtering
            outb(argument, SB16_WRITE_PORT);
            break;

        default:
            return -1;
    }

    return 0;
}


