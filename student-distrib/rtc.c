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

/**
 * @brief Initialize the RTC
 *
 */
void rtc_init(void) {
    // disable all interrupts
    cli();

    // ENABLE PERIODIC INTERRUPTS
    // Default rate of 1024 Hz
    outb(RTC_REG_B, RTC_PORT);
    char prev = inb(RTC_PORT + 1);

    outb(RTC_REG_B, RTC_PORT);
    outb(prev | 0x40, RTC_PORT + 1);  // Turn on bit 6 of register B.

    // enable IRQ8
    enable_irq(RTC_IRQ);

    // enable interrupts

    sti();
}

/**
 * @brief RTC interrupt handler
 *
 */
void rtc_handler(void) {
    uint32_t flags;

    cli_and_save(flags);

    send_eoi(RTC_IRQ);  // Disable interrupts and save flags.

    // Handle RTC interrupt.
    test_interrupts();  // Only for testing purposes.

    restore_flags(flags);  // Restore flags. (Also the IF bit.)
}
