#ifndef TESTS_H
#define TESTS_H

#include "types.h"
#include "filesystem.h"

#define RUN_TESTS

// #define RUN_TESTS_OPEN
// #define RUN_TESTS_CLOSE

// #define RUN_TESTS_RTC
// #define RUN_TESTS_KEYBOARD
// test launcher
void launch_tests();

void rtc_test(uint32_t x);

#ifdef RUN_TESTS_OPEN
extern void test_open(file_t* checked_item);
#endif 

#ifdef RUN_TESTS_CLOSE
extern void test_close(file_t* checked_item);
#endif

#endif /* TESTS_H */
