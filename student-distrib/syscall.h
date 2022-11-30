#ifndef _SYSCALL_H
#define _SYSCALL_H

#define SYS_HALT    1
#define SYS_EXECUTE 2
#define SYS_READ    3
#define SYS_WRITE   4
#define SYS_OPEN    5
#define SYS_CLOSE   6
#define SYS_GETARGS 7
#define SYS_VIDMAP  8
#define SYS_SET_HANDLER  9
#define SYS_SIGRETURN   10
#define SYS_SET_CURSOR  11
#define SYS_GET_CURSOR  12
#define SYS_FILE_CREATE 13
#define SYS_FILE_REMOVE 14
#define SYS_FILE_RENAME 15
#define SYS_GETC        16
#define SYS_SB16_IOCTL  17

#define CMD_MAX_LEN 128
#define ARG_MAX_NUM 10

#define SYSCALL_NUM 17

#ifndef ASM
#include "types.h"


/**
 * @brief syscall number table
 * copied from ../syscalls/sysnum.h
 */

extern void install_syscall();
extern int32_t execute(const uint8_t* command);
extern int32_t _execute(const uint8_t* command,uint32_t pid,uint32_t ppid);
extern int32_t halt(uint8_t status);
extern int32_t close(int32_t fd);
#endif

#endif

