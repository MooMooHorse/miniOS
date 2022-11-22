/* exception.S - exception handlers
 */
#define ASM 1
#include "exception.h"
#include "syscall.h"

/* assembly linkage for exceptions */
.text

.globl exception_hanlder_jump_table

.align 4

/* jump table for exception hanlder, in total 21, because of reserved for intel */
exception_hanlder_jump_table:
.long divide_zero_exception
.long debug_exception
.long NMI_interrupt
.long breakpoint_exception
.long overflow_exception
.long BOUND_range_exceed_exception
.long invalid_opcode_exception
.long device_not_available_exception
.long double_fault_exception
.long coprocessor_segment_overrun
.long invalid_TSS_exception
.long segment_not_present_exception
.long stack_fault_exception
.long general_protection_exception
.long page_fault_exception
.long reserved_exception
.long x87_fpu_floating_point_error
.long alignment_check_exception
.long machine_check_exception
.long SIMD_floating_point_exception
.long reserved_exception


/* Before assembly linkage, IF is cleared since we have interrupt gate 
*  eflags register is saved by proecessor 
*  fastcall is used in this function, eax, ecx, edx
*/
common_exception_handler:
    /* save all the registers */
    pushfl  # There is no IRET, we squash the current program afterwards
    pushal
    cli
    /* old eax has ~#IRQ, note that it is always*/
    movl %esp,%eax
    /* push argument */
    /* call the handler in C : (if fastcall not available) */
    pushl %eax # push arguments (because fastcall not available)
    call do_exception
    addl $4,%esp # pop arguments
    /* restore all registers */
    popal
    popfl
    /* jump to wrapup_common_exception handler */
    jmp wrapup_common_exception

wrapup_common_exception:
    popl %eax 

    jmp sig_enter
    pushl $255 # parameter to halt (status=255)
    call halt 
# You should never enter area below 
exception_error_loop:
    jmp exception_error_loop # this will never be called because of the nature of exceptions for cp1

/* 0 */
divide_zero_exception:
    
    pushl $0 # ~0xffffffff = 0  ; the first push will be saved as eax, so it will be popped when you restore registers
    jmp common_exception_handler # No need to restore after this, because it will be popped when restoring registers 

/* 1 */
debug_exception:
    
    pushl $1 # ~0xfffffffe = 1  ;
    jmp common_exception_handler 
/* 2 */
NMI_interrupt:
    
    pushl $2 # ~0xfffffffd = 2  ;
    jmp common_exception_handler 
/* 3 */
breakpoint_exception:
    
    pushl $3 # ~0xfffffffc = 3  ;
    jmp common_exception_handler 
/* 4 */
overflow_exception:
    
    pushl $4 # ~0xfffffffb = 4  ;
    jmp common_exception_handler 
/* 5 */
BOUND_range_exceed_exception:
    
    pushl $5 # ~0xfffffffa = 5  ;
    jmp common_exception_handler 
/* 6 */
invalid_opcode_exception:  
    
    pushl $6 # ~0xfffffff9 = 6  ;
    jmp common_exception_handler 
/* 7 */
device_not_available_exception:
    
    pushl $7 # ~0xfffffff8 = 7  ;
    jmp common_exception_handler 
/* 8 */
double_fault_exception:
    
    pushl $8 # ~0xfffffff7 = 8  ;
    jmp common_exception_handler 
/* 9 */
/* reserved for Intel*/
coprocessor_segment_overrun:
    
    pushl $9 # ~0xfffffff6 = 9   l
/* 10 */
invalid_TSS_exception:
    
    pushl $10 # ~0xfffffff5 = 10  ;
    jmp common_exception_handler 

/* 11 */
segment_not_present_exception: 
    
    pushl $11 # ~0xfffffff4 = 11  ;
    jmp common_exception_handler 

/* 12 */
stack_fault_exception:
    
    pushl $12 # ~0xfffffff3 = 12  ;
    jmp common_exception_handler 

/* 13 */
general_protection_exception:
    
    pushl $13 # ~0xfffffff2 = 13  ;
    jmp common_exception_handler 

/* 14 */
page_fault_exception:
    /* page fault is pushed by processor */
    ; pushl $14 # ~0xfffffffe = 14  ;
    jmp common_exception_handler 
/* 15 */
/* reserved for intel */

/* 16 */
x87_fpu_floating_point_error:
    
    pushl $16 # ~0xfffffffe = 16  ;
    jmp common_exception_handler 
/* 17 */
alignment_check_exception:
    
    pushl $17 # ~0xfffffffe = 17  ;
    jmp common_exception_handler 
/* 18 */
machine_check_exception:   
    
    pushl $18 # ~0xfffffffe = 18  ;
    jmp common_exception_handler 
/* 19 */ 
SIMD_floating_point_exception: 
    
    pushl $19 # ~0xfffffffe = 19  ;
    jmp common_exception_handler 

/* 20 */
/* reserved for intel */
reserved_exception:
