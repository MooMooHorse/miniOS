/**
 * @file phandler.c
 * @author haor2
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
uint32_t do_exception(old_regs_t *oldregs) {
    if (oldregs == NULL) {
        printf("invalid pointer old_regs\n");
        return -1;
    }
    /* negate this value : reason explained in exception.S : label divide_zero_exception */
    uint32_t exception_index = (~oldregs->orig_eax);
    if (exception_index < 0 || exception_index > 20) {
        printf("wrong exception index: %d, you will double fault!\n", exception_index);
        return exception_index;
    }
    printf("exception %d : %s\n", exception_index, exception_name[exception_index]);

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
static uint8_t is_exception_reserved(uint32_t i){
    return (i==15||i>=20);
}

/**
 * @brief set exception handler table
 * IDT 0~19
 * Omit Intel reserved
 * @return ** void
 */
void install_exception_hanlder() {
    int32_t i;
    for (i = 0; i < 20; i++) {
        if (is_exception_reserved(i))
            continue;
        SET_IDT_ENTRY(idt[i], execption_hanlder_jump_table[i]);
    }
}
