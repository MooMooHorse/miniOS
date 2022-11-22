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

#define MAX_TERMINAL_NUM 10


#ifndef ASM
typedef struct terminal_ops {
    fops_t ioctl; /* a series of operation on terminal shared by all terminals */
} terminal_ops_t;
extern terminal_ops_t terminal_ops;

typedef struct terminal{
    int32_t index; /* index for terminal : 0~9 */
    uint32_t pid;
    int8_t* video; /* pointer to video memory */
    input_t input;  /* input buffer for current terminal */
    int32_t screen_x,screen_y; /* cursor coordinate for current terminal */
    int8_t active; /* terminal active or not */
    int32_t (*open) (int32_t,int32_t*);
} terminal_t;

extern terminal_ops_t terminal_ops;
extern terminal_t terminal[MAX_TERMINAL_NUM];
extern int32_t terminal_index;
// Device driver interfaces.
void terminal_init(void);
extern int32_t terminal_open(file_t* file, const uint8_t* buf, int32_t nbytes);
extern int32_t terminal_close(file_t* file);
extern int32_t terminal_read(file_t* file, void* buf, int32_t nbytes);
extern int32_t terminal_write(file_t* file, const void* buf, int32_t nbytes);
extern uint32_t get_terbuf_addr(int32_t index);
extern int32_t prog_video_update(int32_t index);
extern int32_t terminal_load(int32_t index);
extern int32_t terminal_switch(int32_t old,int32_t new);
extern void prog_video_recover(char* video,int sx,int sy);
#endif /* ASM */

#endif  /* _TERMINAL_H */
