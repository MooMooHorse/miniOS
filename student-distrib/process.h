#ifndef _PROCESS_H
#define _PROCESS_H

/* virtual memory address of user stack pointer pointing at the end of program */
#define USR_STACK_PTR (0x08000000+0x400000 - 0x04) /* 128 MB + 4 MB - 0x04 */


#ifndef ASM

#include "types.h"
#include "x86_desc.h" 
/* size of a program is 4 MB */
#define PROG_SIZE (4<<20) 

extern int32_t pcb_create(uint32_t pid);
extern int32_t pcb_open(uint32_t pid,const uint8_t* prog_name);
extern void switch_user();
extern void setup_tss();
extern fd_t* get_fd_entry(uint32_t fd);

#endif

#endif

