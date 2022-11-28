/**
 * @file cursor.c
 * @author Scharfrichter Qian
 * @brief This file contains implementations for VGA hardware-level cursor.
 * @version 0.1
 * @date 2022-11-10
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "cursor.h"

/*!
 * @brief This function enables and initializes the cursor.
 * @param None.
 * @return None.
 * @sideeffect Modifies VGA registers.
 */
void
cursor_init() {
    // Write Cursor Scan Line Start field.
    outb(CUR_S, CRTC_ADDR);
    outb((inb(CRTC_DATA) & 0xC0) | TOP, CRTC_DATA);  // Preserve unused bits.
                                                
    // Write Cursor Scan Line End field.
    outb(CUR_E, CRTC_ADDR);
    outb((inb(CRTC_DATA) & 0xE0) | BOTTOM, CRTC_DATA);  // Preserve unused bits.
}

/*!
 * @brief This function disables cursor.
 * @param None.
 * @return None.
 * @sideeffect Modifies VGA registers.
 */
void
cursor_close() {
    outb(CUR_S, CRTC_ADDR);
    outb(0x20, CRTC_DATA);
}

/*!
 * @brief This function updates the location of the cursor.
 * @param x is a video memory coordinate.
 * @param y is the other video memory coordinate.
 * @return None.
 * @sideeffect Modifies VGA registers.
 */
int32_t
set_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;

    // Write low/high bits of cursor location.
    outb(CUR_LL, CRTC_ADDR);
    outb((uint8_t) (pos & 0xFF), CRTC_DATA);
    outb(CUR_LH, CRTC_ADDR);
    outb((uint8_t) ((pos >> 8) & 0xFF), CRTC_DATA);
    return 0;
}

/*!
 * @brief This function gets the current location of the cursor.
 * @param None.
 * @return Current location of the cursor.
 * @sideeffect None.
 */
int32_t
get_cursor(void) {
    int32_t pos = 0;

    // Get low/high bits of cursor location and recover the value.
    outb(CUR_LL, CRTC_ADDR);
    pos |= inb(CRTC_DATA);
    outb(CUR_LH, CRTC_ADDR);
    pos |= ((uint16_t) inb(CRTC_DATA) << 8);
    
    return pos;
}
