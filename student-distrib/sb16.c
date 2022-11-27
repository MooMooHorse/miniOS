#include "sb16.h"
#include "i8259.h"
#include "x86_desc.h"
#include "lib.h"

volatile uint8_t sb16_busy = 0;
volatile uint8_t sb16_interrupted = 0;

int32_t sb16_init() {
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

    // PROGRAMMING DMA
    outb(0x01 + 0x04, 0x0A);
    outb(0x01, 0x0C);
    outb(0x0B, 0x48 + 0x01);
    // DO NOT RUN CODE, THESE PAGE ADDRESSES ARE UNDESIGNATED
    // WARNING: CHECK WITH TEAM ABOUT PAGE ADDRESS
    outb((uint8_t)SB16_PAGE_ADDRESS >> 16, 0x83);
    outb((uint8_t)SB16_PAGE_ADDRESS, 0x02);
    outb((uint8_t)SB16_PAGE_ADDRESS >> 8, 0x02);
    // WARNING: CHECK WITH TEAM ABOUT 
    outb((uint8_t)(SB16_DATA_LENGTH - 1), 0x03);
    outb((uint8_t)(SB16_DATA_LENGTH - 1) >> 8, 0x03);

    // ENABLE CHANNEL 1
    outb(0x01, 0x0A);

    return 0;
}

int32_t sb16_open(file_t* file, const uint8_t* buf, int32_t nbytes) {
    return sb16_init();
}

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

int32_t sb16_write(file_t* file, const void* buf, int32_t nbytes) {
    // write
}

int32_t sb16_close(file_t* file) {
    // close
}

void sb16_interrupt_handler() {
    // interrupt handler
}
