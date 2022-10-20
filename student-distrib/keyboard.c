/**
 * @file keyboard.c
 * @author tufekci2
 * @brief keyboard related functions, including handler
 * @version 0.1
 * @date 2022-10-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "keyboard.h"
static char buf[BUFFER_SIZE];
// static uint32_t buffer_pt;
static char simple_char[SCAN_CODE_SIZE];

// modifier key flags
int shift_flag = 0;
int caps_flag = 0;
int ctrl_flag = 0;
// static compound_char[];

static const char simple_char[SCAN_CODE_SIZE] = {
    [2] = '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\r', '\0', 'a',
    's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', '\0', '\\', 'z', 'x',
    'c', 'v', 'b', 'n', 'm', ',', '.', '/', '\0'  // [0x36]
};

// modifier key flags
int shift_flag = 0;
int caps_flag = 0;
int ctrl_flag = 0;

/**
 * @brief Initialize the keyboard
 * Should be ran when CLI is called 
 */
void keyboard_init(void){
    // enable IRQ1
    enable_irq(KEYBOARD_IRQ);
}

/**
 * @brief keyboard interrupt handler
 *
 */
void keyboard_handler(void){
    uint8_t scan_code = inb(KEYBOARD_PORT); //first byte NOT IN ASCII
    // if(val==0xe0){
    //     // inb();//second byte;
    //     // special character flag
    //     // also check for array of flags
    //     // currently does nothing due to cp1 not requiring it
    // }
    // if(singlechar(val)){
    //     printf("%c");
    //     buf[buffer_pt++]=val;
    // }
    //
    /* ctrl + L, */
    // if(){
    //     send_eoi(KEYBOARD_IRQ);
    //     sti();
    //     return ;
    // }
    switch (scan_code) {
        // "i will do scan codes" -- oz
        // Shift Press (L)
        case (0x2A):
            shift_flag = 1;
            break;

        // Shift Press (R)
        case (0x36):
            shift_flag = 1;
            break;

        // Caps Lock
        case (0x3A):
            caps_flag = !caps_flag;
            break;

        // Ctrl (L) Press
        case (0x9D):
            ctrl_flag = 1;
            break;

        // Ctrl (R) Press


    }

    if ((simple_char[scan_code]>='a' && simple_char[scan_code]<='z') ||
        (simple_char[scan_code]>='0' && simple_char[scan_code]<='9'))
        
        putc(simple_char[scan_code]); // output the ascii 
    send_eoi(KEYBOARD_IRQ);
}
