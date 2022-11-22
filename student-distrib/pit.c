#include "pit.h"

void
pit_init(void) {
    uint32_t divisor = PIT_FREQ / SCHED_FREQ;

    // Set PIT mode.
    outb(PIT_BIN | PIT_SQR | PIT_LH | PIT_CH0, PIT_CMD);

    // Send divisor (lo/hi bytes);
    outb(divisor, PIT_DATA);
    outb(divisor >> 8, PIT_DATA);

    enable_irq(PIT_IRQ);
}

void
pit_handler(void) {
    send_eoi(PIT_IRQ);
    scheduler();
}
