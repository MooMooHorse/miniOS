/**
 * @file phandler.c
 * @author haor2, tufekci2
 * @brief exception hanlder file
 * @version 0.1
 * @date 2022-10-13
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "phandler.h"
#include "exception.h"
#include "lib.h"
#include "x86_desc.h"
#include "irqlink.h"
#include "pit.h"
#include "rtc.h"
#include "keyboard.h"
#include "psmouse.h"
#include "process.h"
#include "signal.h"
#include "sb16.h"

#define SCROLL_SCREEN_ENABLE 0

/**
 * @brief exception name array
 */
static char* exception_name[20]={
"divide zero exception",
"debug exception",
"NMI interrupt",
"breakpoint exception",
"overflow exception",
"BOUND range exceed exception",
"invalid opcode exception",
"device not available exception",
"double fault exception",
"coprocessor segment overrun", /* reserved for intel */
"invalid TSS exception",
"segment not present exception",
"stack fault exception",
"general protection exception",
"page fault exception",
"",/* reserved for intel */
"x87 fpu floating point error",
"alignment check exception",
"machine check exception",
"SIMD floating point exception"
/* reserved for intel */
};

/**
 * @brief exception handler
 * @param oldregs
 * @return ** uint32_t
 * -1 : pointer passed into it is invalid
 * else : exception index
 */
uint32_t 
do_exception(old_regs_t *oldregs) {
    if (oldregs == NULL) {
        printf("invalid pointer old_regs\n");
        return -1;
    }
    /* label divide_zero_exception */
    uint32_t exception_index = (oldregs->orig_eax);
    uint32_t pid=get_pid();
    pcb_t* _pcb_ptr=(pcb_t*)(PCB_BASE-pid*PCB_SIZE);
    if (exception_index < 0 || exception_index > 20) {
        printf("wrong exception index: %d, you will double fault!\n", exception_index);
        return exception_index;
    }
    printf("exception %d : %s\n", exception_index, exception_name[exception_index]);
    if(pid>0&&pid<=PCB_MAX){
        _pcb_ptr->sig_num=exception_index?SIG_SEGFAULT:SIG_DIV_ZERO;
    }
#if (SCROLL_SCREEN_ENABLE != 0)
    printf("register values before exception are as following:\n");
    printf("ebx: %d ", oldregs->oebx);
    printf("ecx: %d ", oldregs->oecx);
    printf("edx: %d ", oldregs->oedx);
    printf("esi: %d \n", oldregs->oesi);
    printf("edi: %d ", oldregs->oedi);
    printf("ebp: %d ", oldregs->oebp);
#endif
    // printf("with eax value being discarded\n");
    // printf("ds: %d es: %d fs: %d\n",
    // oldregs->ods,oldregs->oes,oldregs->ofs);
    return exception_index;
}

/**
 * @brief return if the index of exception is reserved by intel
 *
 * @param i
 * @return ** uint8_t
 * 0 - not reserved
 * 1 - reserved
 */
static uint8_t 
is_exception_reserved(uint32_t i){
    return (i==15||i>=20);
}

/**
 * @brief set exception handler table
 * IDT 0~19
 * Omit Intel reserved
 * @return ** void
 */
void 
install_exception_hanlder() {
    int32_t i;
    for (i = 0; i < 20; i++) {
        if (is_exception_reserved(i))
            continue;
        SET_IDT_ENTRY(idt[i], exception_hanlder_jump_table[i]);
    }
}

/**
 * @brief interrupt handler
 * @param oldregs
 * @return ** uint32_t
 * -1 : pointer passed into it is invalid
 * else : interrupt index
 */
uint32_t 
do_interrupt(old_ireg_t *oldregs) {
    if (oldregs == NULL) {
        printf("invalid pointer old_regs\n");
        return -1;
    }
    /* negate this value : reason explained in irqlink.S */
    uint32_t interrupt_index = (~oldregs->orig_eax);
    if (interrupt_index < 0 || interrupt_index > 0x2F) {
        printf("wrong interrupt index: %d, you will double fault!\n", interrupt_index);
        return interrupt_index;
    }
    switch (interrupt_index) {
        case 0x00:
            pit_handler();
            break;
        case 0x01:
            keyboard_handler();
            break;
        case 0x05:
            sb16_handler();
            break;  
        case 0x08:
            rtc_handler();
            break;
        case 0x0C:
            psmouse_handler();
            break;
        default:
            printf("unknown interrupt %d\n", interrupt_index);
            break;
    }
    return interrupt_index;
}

/**
 * @brief set interrupt handler table
 * IDT 0x21, 0x28
 * @return ** void
 */
void 
install_interrupt_hanlder() {
    SET_IDT_ENTRY(idt[0x20], interrupt_handler_jump_table[0]);
    SET_IDT_ENTRY(idt[0x21], interrupt_handler_jump_table[1]);
    SET_IDT_ENTRY(idt[0x25], interrupt_handler_jump_table[2]);
    SET_IDT_ENTRY(idt[0x28], interrupt_handler_jump_table[3]);
    SET_IDT_ENTRY(idt[0x2C], interrupt_handler_jump_table[4]);
}
