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


void terminal_init(void);

int32_t terminal_read(uint8_t* buf, int32_t n);

int32_t terminal_write(const uint8_t* buf, int32_t n);


#endif  /* _TERMINAL_H */
