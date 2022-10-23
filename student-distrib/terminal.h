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


// Device driver interfaces.
void terminal_init(void);
int32_t terminal_open(const uint8_t* filename);
int32_t terminal_close(int32_t fd);
int32_t terminal_read(int32_t fd, void* buf, int32_t n_bytes);
int32_t terminal_write(int32_t fd, const void* buf, int32_t n_bytes);


#endif  /* _TERMINAL_H */
