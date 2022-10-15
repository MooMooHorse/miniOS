/* syslink.S - define entry function for system calls
* This file allows manipulation before going to another layer of indirection in assembly level.
* System calls will be encapsulated in another file.
*/
#define ASM 1
#include "syscall.h"

.globl syscall_entry_ptr
syscall_entry_ptr:
.long syscall_entry


syscall_entry:
    pushal
    pushfl
    call sys_call_dispatcher
    popfl
    popal
    iret

