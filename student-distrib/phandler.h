/**
 * @file phandler.h
 * @author haor2
 * @brief handler for exceptions (for now)
 * @version 0.1
 * @date 2022-10-13
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef _PHANLDLER_H
#define _PHANLDLER_H


#ifndef ASM
#include "types.h"
#include "exception.h"

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

uint32_t do_exception(old_regs_t* oldregs); /* exception hanlder */
void install_exception_hanlder(void);
#endif

#endif

