/**
 * @file signal.h
 * @author haor2
 * @brief signal header file
 * @version 0.1
 * @date 2022-11-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef _SIGNAL_H
#define _SIGNAL_H

#include "types.h"

#define SIG_ENABLE 1 /* enable signal or not */

#define NUM_SIGNALS     4    /* number of signals */
#define SIG_DIV_ZERO    0    /* divide by zero */
#define SIG_SEGFAULT    1    /* any other exceptions than divide by zero */
#define SIG_ALARM       2    /* 10 seconds 1 time */
#define SIG_USER1       3    /* user-level defined */


#ifndef ASM

/* copied from ece391syscall.h */
enum signums {
	DIV_ZERO = 0,
	SEGFAULT,
	INTERRUPT,
	ALARM,
	USER1
};
typedef struct sig_handler{
	void (*handler)(void);
} sig_handler_t;

extern int32_t install_sighandler(uint32_t signum, void* handler_address);


#endif /* ASM */
#endif 

