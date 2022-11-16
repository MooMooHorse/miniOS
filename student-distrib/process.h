#ifndef _PROCESS_H
#define _PROCESS_H

/* virtual memory address of user stack pointer pointing at the end of program */
#define USR_STACK_PTR (0x08000000+0x400000 - 0x04) /* 128 MB + 4 MB - 0x04 */

/* stipulated in document, the maximum number of PCB entries is 8*/
#define PCB_MAX 8

/* size of one PCB block is 8 KB */
#define PCB_SIZE (8<<10)

/* base address of bottom PCB */
#define PCB_BASE (8<<20)

/* Size limit for a program image */
/* size of a program is at most 4 MB */
#define PROG_SIZE (4<<20) 

#ifndef ASM

#include "types.h"
#include "x86_desc.h" 
#include "syscall.h"
/**
 * @brief struct for pcb block
 */
typedef struct PCB{
    uint32_t pid;  /* current pid, for user program starting from 1, with kernel pid = 0 */
    uint32_t ppid; /* parent pid */
    uint32_t ueip; /* user eip */
    uint32_t uesp; /* user esp */
    uint32_t kesp; /* kernel esp */
    uint32_t kebp; /* kernel ebp */
    uint32_t cur_PCB_ptr; /* current pcb pointer */
    uint8_t present;
    uint8_t pname[33]; /* program name, aligned with filesystem, max length 32 : 33 with NUL*/
    /* Below is file struct array (open file table) */
    /* file struct entries array : with maximum items 8, defined in FILE_ARRAY_MAX */
    file_t file_entry[FILE_ARRAY_MAX];
    int8_t args[CMD_MAX_LEN]; /* arguments */
    /* after PCB, we have kernel stack for each process */
} pcb_t;


extern int32_t pcb_create(uint32_t pid);
extern int32_t pcb_open(uint32_t ppid, uint32_t pid,const uint8_t* prog_name);
extern int32_t switch_user(uint32_t pid);
extern void setup_tss(uint32_t pid);
extern file_t* get_file_entry(uint32_t fd);
extern uint32_t get_pid();
extern int32_t discard_proc(uint32_t pid,uint32_t status);
extern int32_t copy_to_command(const uint8_t* command,uint8_t* _command,int32_t nbytes);
extern int32_t set_proc_args(const uint8_t* command,int32_t start,int32_t nbytes,uint32_t pid);

#endif

#endif

