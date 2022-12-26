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
    
    outb(0xFF, MASTER_8259_DATA);   // mask out PIC 8259A-1
    outb(0xFF, SLAVE_8259_DATA);    // mask out PIC 8259A-2

    outb(ICW1, MASTER_8259_PORT);        // ICW1: select 8259A-1 init
    outb(ICW2_MASTER, MASTER_8259_DATA); // ICW2: 8259A-1 IR0-7 mapped to 0x20-0x27
    outb(ICW3_MASTER, MASTER_8259_DATA); // ICW3: 8259A-1 (the master) has a slave on IR2
    outb(ICW4, MASTER_8259_DATA);        // ICW4: 8259A-1 is a master

    outb(ICW1, SLAVE_8259_PORT);        // ICW1: select 8259A-2 init
    outb(ICW2_SLAVE, SLAVE_8259_DATA);  // ICW2: 8259A-2 IR0-7 mapped to 0x28-0x2F
    outb(ICW3_SLAVE, SLAVE_8259_DATA);  // ICW3: 8259A-2 is a slave on master's IR2
    outb(ICW4, SLAVE_8259_DATA);        // ICW4: 8259A-2 is a slave

    // udelay(100); // wait for PIC to initialize

    outb(master_mask, MASTER_8259_DATA); // restore master PIC mask
    outb(slave_mask, SLAVE_8259_DATA);   // restore slave PIC mask

    // enable_irq(SLAVE_8259_PORT);
}

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
    uint8_t mask = 0x1;
    if (irq_num >= 16) { return; }  // Invalid IRQ #!

    if (irq_num & 8) { /* secondary pic */
        mask <<= (irq_num & 7); /* if it's 3rd on secondary pic, then it's 1<<2 */
        slave_mask &= ~mask; /* negate it, e.g. 3rd is 1111111011 */
        outb(slave_mask, SLAVE_8259_DATA);
    } else {
        mask <<= irq_num;
        master_mask &= ~mask;
        outb(master_mask, MASTER_8259_DATA);
    }
}

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
    uint8_t mask = 0x1;
    if (irq_num >= 16) { return; }  // Invalid IRQ #!

    if (irq_num & 8) {  // IRQ #8 ~ #15?
        mask <<= (irq_num & 7);
        slave_mask |= mask;
        outb(slave_mask, SLAVE_8259_DATA);
    } else {
        mask <<= irq_num;
        master_mask |= mask;
        outb(master_mask, MASTER_8259_DATA);
    }
}

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
    if (irq_num >= 16) { return; }  // Invalid IRQ #!

    if (irq_num & 8) {  // IRQ #8 ~ #15?
        outb(EOI | (irq_num & 7), SLAVE_8259_PORT);
        outb(EOI | 2, MASTER_8259_PORT);
    } else {
        outb(EOI | irq_num, MASTER_8259_PORT);
    }
}
