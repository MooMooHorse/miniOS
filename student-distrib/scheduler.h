#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include "process.h"
#include "terminal.h"
#include "mmu.h"

void scheduler(void);
void swtch(context_t** curr, struct context* next);

#endif
