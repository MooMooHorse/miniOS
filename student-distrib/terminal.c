/*!
 * @file terminal.c
 * @author Scharfrichter Qian
 * @brief This file contains the implementation of the terminal operations.
 * @version 1.0
 * @date 2022-10-21
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "terminal.h"

/*!
 * @brief This function initializes the terminal to the expected initial state.
 * @param None.
 * @return None.
 * @sideeffect None.
 */
void
terminal_init(void) {
    // Nothing currently needs to be initialized.
}

/*!
 * @brief This function reads `n` character from the `input` buffer of the keyboard driver and
 * writes them to the input buffer `buf`.
 * @param buf is input buffer to be written with characters.
 * @param n is number of characters the caller intended to read.
 * @return actual number of characters read from `input` buffer.
 * @sideeffect Updates `input` buffer read index.
 */
int32_t
terminal_read(uint8_t* buf, int32_t n) {
    int32_t target = n;
    uint8_t c = '\0';

    if (NULL == buf || 0 > n) { return -1; }  // Invalid input parameter!

    while (0 < n && '\n' != c) {  // Stop when '\n' reached or `n` characters read.
        while (input.w == input.r);  // Waiting for new user input in input buffer.
        c = input.buf[input.r++ % INPUT_SIZE];  // NOTE: Circular buffer.
        *buf++ = c;
        --n;  // NOTE: Linefeed ('\n') is written to the buffer AND counted.
    }

    return target - n;
}

/*!
 * @brief This function writes `n` characters in the given buffer to the screen.
 * @param buf is the input buffer containing characters to be displayed.
 * @param n is number of characters to write to the screen.
 * @return number of characters successfully written to the screen.
 * @sideeffect Modifies video memory content.
 */
int32_t
terminal_write(const uint8_t* buf, int32_t n) {
    int i;

    if (NULL == buf) { return -1; }  // Invalid buffer!

    for (i = 0; i < n; ++i) {
        putc(buf[i]);
    }

    return n;
}

