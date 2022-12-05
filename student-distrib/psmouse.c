#include "psmouse.h"

#define PSMOUSE_TIMEOUT  100000

#define PSMOUSE_WAIT_IN                                      \
    do {                                                     \
        uint32_t i = PSMOUSE_TIMEOUT;                        \
        while (--i && (inb(PSMOUSE_STAT) & PSMOUSE_RF));     \
    } while (0)

#define PSMOUSE_WAIT_OUT                                     \
    do {                                                     \
        uint32_t i = PSMOUSE_TIMEOUT;                        \
        while (--i && (inb(PSMOUSE_STAT) & PSMOUSE_WF));     \
    } while (0)

#define PSMOUSE_WRITE(x)                    \
    do {                                    \
        PSMOUSE_WAIT_OUT;                   \
        outb(0xD4, PSMOUSE_STAT);           \
        PSMOUSE_WAIT_OUT;                   \
        outb((x), PSMOUSE_DATA);            \
    } while (0)

#define PSMOUSE_READ                        \
    ({                                      \
        PSMOUSE_WAIT_IN;                    \
        inb(PSMOUSE_DATA);                  \
    })

void
psmouse_init(void) {
    uint8_t stat;
    PSMOUSE_WAIT_OUT;
    outb(PSMOUSE_EAD, PSMOUSE_STAT);
    PSMOUSE_WAIT_OUT;
    outb(PSMOUSE_GCS, PSMOUSE_STAT);
    PSMOUSE_WAIT_IN;
    stat = (inb(PSMOUSE_DATA) | PSMOUSE_READY) & ~PSMOUSE_DMC;
    PSMOUSE_WAIT_OUT;
    outb(PSMOUSE_DATA, PSMOUSE_STAT);
    PSMOUSE_WAIT_OUT;
    outb(stat, PSMOUSE_DATA);
    PSMOUSE_WRITE(PSMOUSE_DEF);
    (void) PSMOUSE_READ;  // Waiting for ACK.
    PSMOUSE_WRITE(PSMOUSE_EPS);
    (void) PSMOUSE_READ;  // Waiting for ACK.

    enable_irq(PSMOUSE_IRQ);
}

void
psmouse_handler(void) {
    printf("FUCK\n");
}
