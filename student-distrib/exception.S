/* exception.S - exception handlers
 * vim:ts=4 noexpandtab
 */


/* assembly linkage for exceptions */
.globl divide_zero_exception         
.globl debug_exception
.globl NMI_interrupt
.globl breakpoint_exception
.globl overflow_exception
.globl BOUND_range_exceed_exception
.globl invalid_opcode_exception
.globl device_not_available_exception
.globl double_fault_exception
.globl invalid_TSS_exception
.globl segment_not_present_exception
.globl stack_fault_exception
.globl general_protection_exception
.globl page_fault_exception
.globl x87_fpu_floating_point_error
.globl alignment_check_exception
.globl machine_check_exception
.globl SIMD_floating_point_exception

.align 4


/* Before assembly linkage, IF is cleared since we have interrupt gate 
*  eflags register is saved by proecessor 
*  fastcall is used in this function, eax, ecx, edx
*/
common_exception_handler:
    /* old eax has ~#IRQ, note that it is always*/
    movl (%esp),%eax
    /* save all the registers */
    pushw %fs
    pushw %es 
    pushw %ds
    pushl %eax
    pushl %ebp
    pushl %edi
    pushl %esi 
    pushl %edx 
    pushl %ecx 
    pushl %ebx 

    /* call the handler in C : fastcall */
    call do_exception
    /* restorre all registers */
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ebp
    popl %eax
    /* Warning !!! This pop is problematic because of alignment */
    popw %ds
    popw %es
    popw %fs
    /* jump to wrapup_common_exception handler */
    jmp wrapup_common_exception:

wrapup_common_exception:


iret # this will never be called because of the nature of exceptions 

/* 0 */
divide_zero_exception:
    pushl $0xffffffff # ~0xffffffff = 0  ; the first push will be saved as eax, so it will be popped when you restore registers
    jmp common_exception_handler # No need to restore after this, because it will be popped when restoring registers 

/* 1 */
debug_exception:
    pushl $0xfffffffe # ~0xfffffffe = 1  ;
    jmp common_exception_handler 
/* 2 */
NMI_interrupt:
    pushl $0xfffffffd # ~0xfffffffd = 2  ;
    jmp common_exception_handler 
/* 3 */
breakpoint_exception:
    pushl $0xfffffffc # ~0xfffffffc = 3  ;
    jmp common_exception_handler 
/* 4 */
overflow_exception:
    pushl $0xfffffffb # ~0xfffffffb = 4  ;
    jmp common_exception_handler 
/* 5 */
BOUND_range_exceed_exception:
    pushl $0xfffffffa # ~0xfffffffa = 5  ;
    jmp common_exception_handler 
/* 6 */
invalid_opcode_exception:  
    pushl $0xfffffff9 # ~0xfffffff9 = 6  ;
    jmp common_exception_handler 
/* 7 */
device_not_available_exception:
    pushl $0xfffffff8 # ~0xfffffff8 = 7  ;
    jmp common_exception_handler 
/* 8 */
double_fault_exception:
    pushl $0xfffffff7 # ~0xfffffff7 = 8  ;
    jmp common_exception_handler 
/* 9 */
/* reserved for Intel, should not be filled with this */
# coprocessor_segment_overrun:

/* 10 */
invalid_TSS_exception:
    pushl $0xfffffff5 # ~0xfffffff5 = 10  ;
    jmp common_exception_handler 

/* 11 */
segment_not_present_exception: 
    pushl $0xfffffff4 # ~0xfffffff4 = 11  ;
    jmp common_exception_handler 

/* 12 */
stack_fault_exception:
    pushl $0xfffffff3 # ~0xfffffff3 = 12  ;
    jmp common_exception_handler 

/* 13 */
general_protection_exception:
    pushl $0xfffffff2 # ~0xfffffff2 = 13  ;
    jmp common_exception_handler 

/* 14 */
page_fault_exception:
    pushl $0xfffffff1 # ~0xfffffffe = 14  ;
    jmp common_exception_handler 
/* 15 */
/* reserved for intel */

/* 16 */
x87_fpu_floating_point_error:
    pushl $0xffffffef # ~0xfffffffe = 16  ;
    jmp common_exception_handler 
/* 17 */
alignment_check_exception:
    pushl $0xffffffee # ~0xfffffffe = 17  ;
    jmp common_exception_handler 
/* 18 */
machine_check_exception:   
    pushl $0xffffffed # ~0xfffffffe = 18  ;
    jmp common_exception_handler 
/* 19 */ 
SIMD_floating_point_exception: 
    pushl $0xffffffec # ~0xfffffffe = 19  ;
    jmp common_exception_handler 

/* 20 */
/* reserved for intel */
