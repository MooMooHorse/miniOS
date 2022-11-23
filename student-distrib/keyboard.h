/**
 * @file keyboard.h
 * @author tufekci2
 * @brief keyboard related functions, including handler
 * @version 0.1
 * @date 2022-10-16
 *
 * @version 1.0
 * @author Scharfrichter Qian
 * @date 2022-10-21
 * @brief Refactored code to support special characters and input buffer.
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"
#include "i8259.h"
#include "syscall.h"
#include "lib.h"

#define KEYBOARD_IRQ    0x01
#define KEYBOARD_DATA   0x60
#define KEYBOARD_STAT   0x64
#define INPUT_SIZE      128
#define SCAN_MASK       0xFF
#define RELEASE_MASK    0x80
#define MAP_SIZE        256
#define ALT_BASE        128

#define SHIFT           (1 << 0)
#define CTRL            (1 << 1)
#define ALT             (1 << 2)
#define CAPSLOCK        (1 << 3)

/*!
 * @brief Circular buffer used to store keyboard input.
 */
typedef struct input {
    uint8_t buf[INPUT_SIZE];
    uint32_t r;  // Read index.
    uint32_t w;  // Write index.
    uint32_t e;  // Edit index.
} input_t;

// extern input_t input;

/* Initializes the keyboard IRQ. */
void keyboard_init(void);

/* Handles keyboard interrupts. */
void keyboard_handler(void);

#endif

