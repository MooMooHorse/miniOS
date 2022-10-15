/**
 * @file exception.h
 * @author haor2
 * @brief header file for exception related handling.
 * Make the exception.S file visible to .c files. 
 * @version 0.1
 * @date 2022-10-13
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef _X86_EXCEPTION_H
#define _X86_EXCEPTION_H

#ifndef ASM
#include "types.h"

/**
 * @brief This struct is to record the values on stack 
 * for SAVE_ALL and orig-eax 
 * before do_exception() is called
 * @note only SAVE_ALL and orig-eax are recorded in
 * this struct
 */
typedef struct old_regs {
    uint32_t oedi;
    uint32_t oesi;
    uint32_t oebp;
    uint32_t oesp;
    uint32_t oebx;
    uint32_t oedx;
    uint32_t oecx;
    uint32_t oeax;
    // uint32_t ods,oes,ofs;
    uint32_t orig_eax;
} old_regs_t;

/* jump table for exception hanlder */
extern uint32_t execption_hanlder_jump_table[21];
#endif /* ASM */
#endif

