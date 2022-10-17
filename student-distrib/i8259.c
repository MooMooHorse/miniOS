/* i8259.c - Functions to interact with the 8259 interrupt controller
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void i8259_init(void) {
    master_mask = MASTER_MASK_INIT;
    slave_mask = SLAVE_MASK_INIT;
    
    outb(0xFF, MASTER_8259_PORT + 1); // mask out PIC 8259A-1
    outb(0xFF, SLAVE_8259_PORT + 1);   // mask out PIC 8259A-2

    outb(ICW1, MASTER_8259_PORT);            // ICW1: select 8259A-1 init
    outb(ICW2_MASTER, MASTER_8259_PORT + 1); // ICW2: 8259A-1 IR0-7 mapped to 0x20-0x27
    outb(ICW3_MASTER, MASTER_8259_PORT + 1); // ICW3: 8259A-1 (the master) has a slave on IR2
    outb(ICW4, MASTER_8259_PORT + 1);        // ICW4: 8259A-1 is a master

    outb(ICW1, SLAVE_8259_PORT);            // ICW1: select 8259A-2 init
    outb(ICW2_SLAVE, SLAVE_8259_PORT + 1);  // ICW2: 8259A-2 IR0-7 mapped to 0x28-0x2F
    outb(ICW3_SLAVE, SLAVE_8259_PORT + 1);  // ICW3: 8259A-2 is a slave on master's IR2
    outb(ICW4, SLAVE_8259_PORT + 1);        // ICW4: 8259A-2 is a slave

    // udelay(100); // wait for PIC to initialize

    outb(master_mask, MASTER_8259_PORT + 1); // restore master PIC mask
    outb(slave_mask, SLAVE_8259_PORT + 1);   // restore slave PIC mask

    // enable_irq(SLAVE_8259_PORT);
}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
    unsigned int mask=0x01;

    if (irq_num & 0x0008) {
        mask <<= (irq_num & 7);
        mask = ~mask;
        outb((inb(SLAVE_8259_PORT + 1) & mask), SLAVE_8259_PORT + 1);
    }
    else {
        mask <<= irq_num;
        mask = ~mask;
        outb((inb(MASTER_8259_PORT + 1) & mask), MASTER_8259_PORT + 1);
    }
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
    unsigned int mask=0x01;

    if (irq_num & 0x0008) {
        mask <<= (irq_num & 7);
        outb((inb(SLAVE_8259_PORT + 1) | mask), SLAVE_8259_PORT + 1);
    }
    else {
        mask <<= irq_num;
        outb((inb(MASTER_8259_PORT + 1) | mask), MASTER_8259_PORT + 1);
    }
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
    if (irq_num & 0x0008) {
        outb(EOI | (irq_num & 7), SLAVE_8259_PORT);
        outb(EOI | 2, MASTER_8259_PORT);
    }
    else {
        outb(EOI | irq_num, MASTER_8259_PORT);
    }
}
