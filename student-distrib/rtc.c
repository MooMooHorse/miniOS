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

/**
 * @brief Initialize the RTC
 *
 */
void rtc_init(void) {
    // disable all interrupts
    cli();

    // // SET RTC TO 2 HZ AND ENABLE OSCILLATOR
    // outb(RTC_REG_A, RTC_PORT);
    // char prev = inb(RTC_PORT + 1);
    // outb(RTC_REG_A, RTC_PORT);
    // outb((prev & 0x80) | 0x2F, RTC_PORT + 1); // set the rate to 2Hz, enable oscillator

    // ENABLE PERIODIC INTERRUPTS
    // Default rate of 1024 Hz
    outb(RTC_REG_B, RTC_PORT);
    char prev = inb(RTC_PORT + 1);

    outb(RTC_REG_B, RTC_PORT);
    outb(prev | 0x40, RTC_PORT + 1);

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
    // disable interrupt
    cli();

    // read the register C to reset the interrupt
    outb(RTC_REG_C, RTC_PORT);
    inb(RTC_PORT + 1);

    // printf("a");

    // send EOI
    send_eoi(RTC_IRQ);

    // enable interrupts
    sti();
}
