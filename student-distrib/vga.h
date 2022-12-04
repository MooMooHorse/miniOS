#ifndef _VGA_H

#define _VGA_H

#include "types.h"
#include "x86_desc.h"

extern void VGA_test();
extern int32_t vga_open(file_t* file, const uint8_t* fname, int32_t dump);


#endif

