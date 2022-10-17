#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"
#include "i8259.h"
#include "lib.h"

#define KEYBOARD_IRQ 0x01
#define KEYBOARD_PORT 0x60
// #define SCAN_CODE 0xe0
// #define BUFFER_SIZE 256
#define SCAN_CODE_SIZE 0xff

void keyboard_init(void);
void keyboard_handler(void);

#endif

