/**
 * @file cursor.h
 * @author Scharfrichter Qian
 * @brief This file contains declarations for VGA hardware-level cursor.
 * @version 0.1
 * @date 2022-11-10
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef _CURSOR_H
#define _CURSOR_H

#include "lib.h"

// Cursor shape configuration.
#define TOP                 0
#define BOTTOM              15      // A block-style cursor!

// Constants.
#define VGA_WIDTH           80
#define CRTC_ADDR           0x3D4
#define CRTC_DATA           0x3D5
#define CUR_S               0x0A    // Cursor Start Register.
#define CUR_E               0x0B    // Cursor End Register.
#define CUR_LH              0x0E    // Cursor Location High Register.
#define CUR_LL              0x0F    // Cursor Location Low Register.
                                    
// Driver interfaces.
void cursor_init(void);
void cursor_close(void);
void cursor_update(int x, int y);
uint16_t cursor_get_pos(void);

#endif  /* _CURSOR_H */
