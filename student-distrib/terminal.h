/*!
 * @file terminal.h
 * @author Scharfrichter Qian
 * @brief This file contains the implementation of the terminal operations.
 * @version 1.0
 * @date 2022-10-21
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "keyboard.h"
#include "filesystem.h"

typedef struct terminal {
    fops_t ioctl;
} terminal_t;

extern terminal_t terminal;

// Device driver interfaces.
void terminal_init(void);
int32_t terminal_open(file_t* file, const uint8_t* buf, int32_t nbytes);
int32_t terminal_close(file_t* file);
int32_t terminal_read(file_t* file, void* buf, int32_t nbytes);
int32_t terminal_write(file_t* file, const void* buf, int32_t nbytes);


#endif  /* _TERMINAL_H */
