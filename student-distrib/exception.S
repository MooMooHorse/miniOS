/* exception.S - exception handlers
 * vim:ts=4 noexpandtab
 */
#define ASM 1
#include "exception.h"

/* assembly linkage for exceptions */
.text

.globl execption_hanlder_jump_table

.align 4

/* jump table for exception hanlder, in total 21, because of reserved for intel */
execption_hanlder_jump_table:
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
    pushal
    /* old eax has ~#IRQ, note that it is always*/
    movl %esp,%eax
    /* push argument */
    /* call the handler in C : (if fastcall not available) */
    pushl %eax # push arguments (because fastcall not available)
    call do_exception
    addl $4,%esp # pop arguments
    /* restorre all registers */
    popal
    /* jump to wrapup_common_exception handler */
    jmp wrapup_common_exception

wrapup_common_exception:
    jmp wrapup_common_exception # while(1);
    popl %eax # this is to popl the value of pushl on exception entry. For cp1, this will never be called
    sti # this will never be called because of the nature of exceptions for cp1
    iret # this will never be called because of the nature of exceptions for cp1

/* 0 */
divide_zero_exception:
    cli
    pushl $0xffffffff # ~0xffffffff = 0  ; the first push will be saved as eax, so it will be popped when you restore registers
    jmp common_exception_handler # No need to restore after this, because it will be popped when restoring registers 

/* 1 */
debug_exception:
    cli
    pushl $0xfffffffe # ~0xfffffffe = 1  ;
    jmp common_exception_handler 
/* 2 */
NMI_interrupt:
    cli
    pushl $0xfffffffd # ~0xfffffffd = 2  ;
    jmp common_exception_handler 
/* 3 */
breakpoint_exception:
    cli
    pushl $0xfffffffc # ~0xfffffffc = 3  ;
    jmp common_exception_handler 
/* 4 */
overflow_exception:
    cli
    pushl $0xfffffffb # ~0xfffffffb = 4  ;
    jmp common_exception_handler 
/* 5 */
BOUND_range_exceed_exception:
    cli
    pushl $0xfffffffa # ~0xfffffffa = 5  ;
    jmp common_exception_handler 
/* 6 */
invalid_opcode_exception:  
    cli
    pushl $0xfffffff9 # ~0xfffffff9 = 6  ;
    jmp common_exception_handler 
/* 7 */
device_not_available_exception:
    cli
    pushl $0xfffffff8 # ~0xfffffff8 = 7  ;
    jmp common_exception_handler 
/* 8 */
double_fault_exception:
    cli
    pushl $0xfffffff7 # ~0xfffffff7 = 8  ;
    jmp common_exception_handler 
/* 9 */
/* reserved for Intel*/
coprocessor_segment_overrun:
    cli
    pushl $0xfffffff6 # ~0xfffffff6 = 9   l
/* 10 */
invalid_TSS_exception:
    cli
    pushl $0xfffffff5 # ~0xfffffff5 = 10  ;
    jmp common_exception_handler 

/* 11 */
segment_not_present_exception: 
    cli
    pushl $0xfffffff4 # ~0xfffffff4 = 11  ;
    jmp common_exception_handler 

/* 12 */
stack_fault_exception:
    cli
    pushl $0xfffffff3 # ~0xfffffff3 = 12  ;
    jmp common_exception_handler 

/* 13 */
general_protection_exception:
    cli
    pushl $0xfffffff2 # ~0xfffffff2 = 13  ;
    jmp common_exception_handler 

/* 14 */
page_fault_exception:
    cli
    pushl $0xfffffff1 # ~0xfffffffe = 14  ;
    jmp common_exception_handler 
/* 15 */
/* reserved for intel */

/* 16 */
x87_fpu_floating_point_error:
    cli
    pushl $0xffffffef # ~0xfffffffe = 16  ;
    jmp common_exception_handler 
/* 17 */
alignment_check_exception:
    cli
    pushl $0xffffffee # ~0xfffffffe = 17  ;
    jmp common_exception_handler 
/* 18 */
machine_check_exception:   
    cli
    pushl $0xffffffed # ~0xfffffffe = 18  ;
    jmp common_exception_handler 
/* 19 */ 
SIMD_floating_point_exception: 
    cli
    pushl $0xffffffec # ~0xfffffffe = 19  ;
    jmp common_exception_handler 

/* 20 */
/* reserved for intel */
reserved_exception:
