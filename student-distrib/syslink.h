#ifndef _SYSLINK_H
#define _SYSLINK_H

#ifndef ASM
#include "lib.h"

extern uint32_t syscall_entry_ptr; /* pointer to syscall entry */
extern int32_t sys_execute_wrap(); /* to do */
extern int32_t sys_halt_wrap(); /* to do */

#endif

#endif

