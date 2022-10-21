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
    #ifdef RUN_TESTS_RTC
    rtc_test(virt_rtc);  // Only for testing purposes.
    #endif
    if (++virt_rtc == RTC_DEF_FREQ){
        virt_rtc = 0;  // Reset virtualization counter.
    }

    send_eoi(RTC_IRQ);

    outb(RTC_REG_C, RTC_PORT);
    inb(RTC_DATA_PORT);  // Discard contents of register C.
}



