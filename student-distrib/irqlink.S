/* irqlink.S - interrupt handlers
 */
#define ASM 1
#include "irqlink.h"

/* assembly linkage for interrupts */
.text

.globl interrupt_handler_jump_table

.align 4

/* jump table for interrupt hanlder */
interrupt_handler_jump_table:
.long keyboard_interrupt
.long rtc_interrupt

/* Before assembly linkage, IF is cleared since we have interrupt gate 
*  eflags register is saved by proecessor 
*  fastcall is used in this function, eax, ecx, edx
*/
common_interrupt_handler:
    /* save all the registers */
    pushal
    /* old eax has ~#IRQ, note that it is always*/
    movl %esp,%eax
    /* push argument */
    /* call the handler in C : (if fastcall not available) */
    pushl %eax # push arguments (because fastcall not available)
    call do_interrupt
    addl $4,%esp # pop arguments
    /* restore all registers */
    popal

    popl %eax # this is to popl the value of pushl on exception entry.
    sti
    iret

/* 1 */
keyboard_interrupt:
    cli
    pushl $0xfffffffe # ~0xfffffffe = 1  ;
    jmp common_interrupt_handler 

/* 8 */
rtc_interrupt:
    cli
    pushl $0xfffffff7 # ~0xfffffff7 = 8  ;
    jmp common_interrupt_handler 