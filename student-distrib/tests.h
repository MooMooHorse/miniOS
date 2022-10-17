#ifndef TESTS_H
#define TESTS_H

#include "types.h"

#define RUN_TESTS

// #define RUN_TESTS_RTC
// #define RUN_TESTS_KEYBOARD
// test launcher
void launch_tests();

void rtc_test(uint32_t x);

#endif /* TESTS_H */
