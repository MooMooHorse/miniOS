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

terminal_t terminal = {
    .ioctl.open = terminal_open,
    .ioctl.close = terminal_close,
    .ioctl.read = terminal_read,
    .ioctl.write = terminal_write
};

static int32_t _terminal_read(uint8_t* buf, int32_t n);
static int32_t _terminal_write(const uint8_t* buf, int32_t n);

/*!
 * @brief This function initializes the terminal to the expected initial state.
 * @param None.
 * @return None.
 * @sideeffect Initializes `input` buffer indices.
 */
void
terminal_init(void) {
    input.r = input.w = input.e = 0;
}

/*!
 * @brief Device driver interface. Used to open the terminal.
 * @param Determined by OS device driver interface.
 * @return 0 on success, non-zero otherwise.
 * @sideeffect Initializes `input` buffer indices.
 */
int32_t
terminal_open(__attribute__((unused)) fd_t* fd, __attribute__((unused)) const uint8_t* buf, __attribute((unused)) int32_t nbytes) {
    terminal_init();
    return 0;
}

/*!
 * @brief Device driver interface. Used to close the terminal.
 * @param Determined by OS device driver interface.
 * @return 0 on success, non-zero otherwise.
 * @sideeffect Discard remaining input characters in the `input` buffer.
 */
int32_t
terminal_close(fd_t* fd) {
    fd->file_operation_jump_table.close=NULL;
    fd->file_operation_jump_table.read=NULL;
    fd->file_operation_jump_table.write=NULL;
    fd->file_operation_jump_table.open=NULL;
    fd->flags=F_CLOSE;
    fd->file_position=0;
    fd->inode=-1;
    input.e = input.w;  // Discard unused characters in the input buffer.
    return 0;
}

/*!
 * @brief Device driver interface. Used to read `nbytes` bytes from the terminal.
 * @param fd is file descriptor (unused).
 * @param buf is pointer to input buffer.
 * @param nbytes is number of characters the caller intended to read.
 * @return number of characters actually read.
 * @sideeffect See `_terminal_read` below.
 */
int32_t
terminal_read(__attribute__((unused)) fd_t* fd, void* buf, int32_t nbytes) {
    return _terminal_read((uint8_t*) buf, nbytes);
}

/*!
 * @brief Device driver interface. Used to write `nbytes` bytes to the terminal.
 * @param fd is file descriptor (unused).
 * @param buf is pointer to input buffer.
 * @param nbytes is number of character the caller intended to write.
 * @return number of characters actually wrote.
 * @sideeffect See `_terminal_write` below.
 */
int32_t
terminal_write(__attribute__((unused)) fd_t* fd, const void* buf, int32_t nbytes) {
    return _terminal_write((const uint8_t*) buf, nbytes);
}

/*!
 * @brief This function reads `n` character from the `input` buffer of the keyboard driver and
 * writes them to the input buffer `buf`.
 * @param buf is input buffer to be written with characters.
 * @param n is number of characters the caller intended to read.
 * @return actual number of characters read from `input` buffer.
 * @sideeffect Updates `input` buffer read index.
 */
static int32_t
_terminal_read(uint8_t* buf, int32_t n) {
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
static int32_t
_terminal_write(const uint8_t* buf, int32_t n) {
    int i;

    if (NULL == buf || n > strlen((const int8_t*) buf)) {
        return -1;  // Invalid buffer or `n` greater than size of buffer!
    }

    for (i = 0; i < n; ++i) {
        putc(buf[i]);
    }

    return n;
}

