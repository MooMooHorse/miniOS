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
#define SYS_SIGRETURN  10

#define CMD_MAX_LEN 30
#define ARG_MAX_NUM 10

#ifndef ASM
#include "types.h"


/**
 * @brief syscall number table
 * copied from ../syscalls/sysnum.h
 */

extern void install_syscall();
extern int32_t execute(const uint8_t* command);
extern int32_t halt(uint8_t status);
extern int32_t close(uint32_t fd);
#endif

#endif

