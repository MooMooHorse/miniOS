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
    pushal
    pushfl
    sti
    pushl %edx # 3rd argument
    pushl %ecx # 2nd argument
    pushl %ebx # 1st argument
    # TO DO : try to see why syscall_table(0,%eax,4) will give compilation error 
    leal  syscall_table,%ebx
    movl  %eax,%ecx
    addl  %ecx,%ecx
    addl  %ecx,%ecx
    addl  %ecx,%ebx
    call  *(%ebx)
    movl  %eax,_orig_eax
    addl $12, %esp # pop 4 arguments  
    popfl
    popal
    movl _orig_eax,%eax # make return value in eax
    iret



