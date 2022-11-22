#define ASM

#include "signal.h"

.globl sig_enter

.align 4

_uesp:
    .long 0x0
_kesp:
    .long 0x0

# execute sigreturn(#10)  
_prog_start:          
	movl	$10,%eax  
	int	$0x80

_prog_end: 
jmp _prog_end # while(1), this should never be accessed 

# Before this call, the current kernel stack for 
# a process is only left with parameters for IRET 
# namely, hardware context pushed by processor
sig_enter:
    pushal # complete hardware context pushed by program
    movl 44(%esp),%ebx # 32 : registers saved - 12 : esp
    movl %ebx,_uesp  # user stack
    movl %esp,_kesp
    # now esp points to hardware context
    pushl $_prog_end
    pushl $_prog_start
    pushl (_uesp)
    pushl (_kesp)
    call do_signal

_sig_end: # this should never be accessed 
jmp _sig_end


