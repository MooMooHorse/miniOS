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

uint32_t do_exception(old_regs_t* oldregs); /* exception hanlder */
void install_exception_hanlder(void);
#endif

#endif

