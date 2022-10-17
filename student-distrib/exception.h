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


/* jump table for exception hanlder */
extern uint32_t execption_hanlder_jump_table[21];
#endif /* ASM */
#endif

