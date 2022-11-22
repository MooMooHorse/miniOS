#ifndef _PIT_H
#define _PIT_H

#define PIT_IRQ     0x00
#define PIT_DATA    0x40        // Channel 0 data port (rw).
#define PIT_CMD     0x43        // Mode/command register (w).
#define PIT_FREQ    1193182
#define PIT_BIN     0x0
#define PIT_BIN     0x0         // Binary mode.
#define PIT_SQR     (0x3 << 1)  // Operating mode: Square wave generator.
#define PIT_LH      (0x3 << 4)  // Access mode: lobyte/hibyte.
#define PIT_CH0     (0x0 << 6)  // Channel 0.
#define SCHED_FREQ  100         // Scheduled frequency: 100 Hz.

#include "types.h"
#include "i8259.h"
#include "syscall.h"
#include "lib.h"
#include "scheduler.h"


void pit_init(void);
void pit_handler(void);

#endif
