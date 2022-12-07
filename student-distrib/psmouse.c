/*!
 * @brief This file contains implementations for PS/2 mouse.
 */
#include "psmouse.h"
#include "process.h"
#include "terminal.h"
#include "signal.h"

#define PSMOUSE_TIMEOUT     100000
#define SIGN_EXTENSION(x)   ((x) | 0xFFFFFF00)
#define SCREEN_WIDTH        80
#define SCREEN_HEIGHT       25

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

#define PSMOUSE_PACKET_INVALID(x)  \
    (((x) & (PSMOUSE_XOF | PSMOUSE_YOF)) || !((x) & 0x8))

static int32_t screen_x;
static int32_t screen_y;
static int32_t if_click;

/*!
 * @brief This function initializes the PS/2 mouse.
 * @param None.
 * @return None.
 * @sideeffect Consumes data in ports 0x60 and 0x64.
 */
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

/*!
 * @brief PS/2 mouse interrupt handler.
 * @param None.
 * @return None.
 * @sideeffect Modifies video memory.
 */
void
psmouse_handler(void) {
    uint8_t stat;
    int32_t delta_x;
    int32_t delta_y;
    int32_t i;
    send_eoi(PSMOUSE_IRQ);

    // Get the first three packet bytes.
    stat = PSMOUSE_READ;
    delta_x = PSMOUSE_READ;
    delta_y = PSMOUSE_READ;

    if (PSMOUSE_PACKET_INVALID(stat)) {
        return;  // Discard the packet since it is invalid.
    }

    delta_x = (stat & PSMOUSE_XS) ? SIGN_EXTENSION(delta_x) : delta_x;
    delta_y = (stat & PSMOUSE_YS) ? SIGN_EXTENSION(delta_y) : delta_y;

    if(!get_graphics()) 
        unlight_pixel(screen_x, screen_y);
    screen_x += delta_x;
    screen_y -= delta_y;

    // Reset to edges if out of bounds.
    screen_x = (1 > screen_x) ? 1 : (SCREEN_WIDTH < screen_x) ? SCREEN_WIDTH : screen_x;
    screen_y = (0 > screen_y) ? 0 : (SCREEN_HEIGHT - 1 < screen_y) ? SCREEN_HEIGHT - 1 : screen_y;
    if(!get_graphics()) 
        light_pixel(screen_x, screen_y);

    // Handle left clicks...
    if (stat & PSMOUSE_LB) {
        if_click=1;
    }

    for(i=1;i<=PCB_MAX;i++){
        if(PCB(i)->terminal==terminal_index&&
        (PCB(i)->state==RUNNING||PCB(i)->state==RUNNABLE)){
            PCB(i)->sig_num=SIG_USER1;
            break;
        }
    }

}


/**
 * @brief bad call
 */
int32_t mouse_write(file_t* file, const void* buf, int32_t bytes){
    return -1;
}

/**
 * @brief read screen location of mouse and if mouse has clicked
 * @param file - dump
 * @param buf - buffer to read the location and if click
 * @param nbytes - must 12
 * @return ** int32_t -1 on failure 0 on success 
 */
int32_t mouse_read(file_t* file,void* buf, int32_t nbytes){
    if(nbytes!=12) return -1;
    ((int32_t*)buf)[0]=screen_x;
    ((int32_t*)buf)[1]=screen_y;
    ((int32_t*)buf)[2]=if_click;
    if_click=0;
    return 0;
}
/**
 * @brief close mouse
 * @param file - file descriptor
 * @return ** int32_t always 0
 */
int32_t mouse_close(file_t* file){
    file->flags=F_CLOSE;
    file->pos=0;
    return 0;
}

/**
 * @brief open mouse
 * @param file - file descriptor
 * @param fname - file name to open : "psmouse"
 * @param dump - dumped
 * @return ** int32_t always 0
 */
int32_t mouse_open(file_t* file, const uint8_t* fname, int32_t dump){
    file->flags=F_OPEN;
    file->fops.write=mouse_write;
    file->fops.read=mouse_read;
    file->fops.close=mouse_close;
    file->pos=0;
    file->inode=-1;
    screen_x=screen_y=0;
    return 0;
}
