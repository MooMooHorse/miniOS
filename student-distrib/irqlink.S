/**
 * @file irqlink.s
 * @author tufekci2
 * @brief asm file for interrupt related handling. 
 * @version 0.1
 * @date 2022-10-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#define ASM 1
#include "irqlink.h"

/* assembly linkage for interrupts */
.text

.globl interrupt_handler_jump_table

.align 4

/* jump table for interrupt hanlder */
interrupt_handler_jump_table:
.long pit_interrupt
.long keyboard_interrupt
.long rtc_interrupt

/* Before assembly linkage, IF is cleared since we have interrupt gate 
*  eflags register is saved by proecessor 
*  fastcall is used in this function, eax, ecx, edx
*/
common_interrupt_handler:
    /* save all the registers */
    pushal
    pushfl
    cli
    /* old eax has ~#IRQ, note that it is always*/
    movl %esp,%eax
    /* push argument */
    /* call the handler in C : (if fastcall not available) */
    pushl %eax # push arguments (because fastcall not available)
    call do_interrupt
    addl $4,%esp # pop arguments
    /* restore all registers */
    popfl
    popal
    addl $4,%esp # pop $0xfffff...

    jmp sig_enter
    iret

/* 0 */
pit_interrupt:
    pushl $0xffffffff # ~0xffffffff = 0  ;
    jmp common_interrupt_handler

/* 1 */
keyboard_interrupt:
    pushl $0xfffffffe # ~0xfffffffe = 1  ;
    jmp common_interrupt_handler 

/* 8 */
rtc_interrupt:
    pushl $0xfffffff7 # ~0xfffffff7 = 8  ;
    jmp common_interrupt_handler 
