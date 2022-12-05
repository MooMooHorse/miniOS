/*!
 * @brief This file contains declarations for PS/2 mouse.
 */
#ifndef _PSMOUSE_H
#define _PSMOUSE_H

#include "lib.h"
#include "i8259.h"

#define PSMOUSE_IRQ      12
#define PSMOUSE_DATA     0x60
#define PSMOUSE_STAT     0x64
#define PSMOUSE_READY    (1 << 1)
#define PSMOUSE_DMC      (1 << 5)       // Disable Mouse Clock.
#define PSMOUSE_LB       (1 << 0)       // Left button.
#define PSMOUSE_RB       (1 << 1)       // Right button.
#define PSMOUSE_MB       (1 << 2)       // Middle button.
#define PSMOUSE_XS       (1 << 4)       // X sign bit.
#define PSMOUSE_YS       (1 << 5)       // Y sign bit.
#define PSMOUSE_XOF      (1 << 6)       // X overflow.
#define PSMOUSE_YOF      (1 << 7)       // Y overflow.
#define PSMOUSE_WF       (1 << 1)       // Write flag.
#define PSMOUSE_RF       (1 << 0)       // Read flag.
#define PSMOUSE_STAT_EN  (1 << 5)       // Data reporting is enabled.
#define PSMOUSE_EAD      0xA8           // Enable Auxiliary command.
#define PSMOUSE_GCS      0x20           // Get Compaq Status byte.
#define PSMOUSE_DEF      0xF6           // Set Defaults.
#define PSMOUSE_EPS      0xF4           // Enable Packet Streaming.

extern void psmouse_init(void);
extern void psmouse_handler(void);

#endif
