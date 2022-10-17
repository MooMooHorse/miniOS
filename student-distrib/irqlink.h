/**
 * @file irqlink.h
 * @author tufekci2
 * @brief header file for interrupt related handling.
 * Make the irqlink.S file visible to .c files. 
 * @version 0.1
 * @date 2022-10-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef _X86_irqlink_H
#define _X86_irqlink_H

#ifndef ASM
#include "types.h"

/* jump table for interrupt hanlder */

extern uint32_t interrupt_handler_jump_table[2];
#endif /* ASM */

#endif

