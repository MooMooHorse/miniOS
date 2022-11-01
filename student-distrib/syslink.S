/* syslink.S - define entry function for system calls
* This file allows manipulation before going to another layer of indirection in assembly level.
* System calls will be encapsulated in another file.
*/
#define ASM 1
#include "syscall.h"

.globl syscall_entry_ptr
.text



syscall_entry_ptr:
.long syscall_entry

/* local storage for return value of system call */
_orig_eax:
.long 0


syscall_entry:
    # TO Do : check eax in correct range (1~10)

    pushal
    pushfl
#   sti        # might have problems on context switch : better sti() in do_system_call function 
    pushl %edx # 3rd argument
    pushl %ecx # 2nd argument
    pushl %ebx # 1st argument
    call  *syscall_table(,%eax,4)
    movl  %eax,_orig_eax
    addl $12, %esp # pop 4 arguments  
    popfl
    popal
    movl _orig_eax,%eax # make return value in eax
    iret



