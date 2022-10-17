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
// static char buf[BUFFER_SIZE];
// static uint32_t buffer_pt;
static uint8_t simple_char[scan_code_2_size];
// static compound_char[];

// void flush_the_buffer(){
//     // printf(" "); printing the buffer
//     buffer_pt=0;
// }

/**
 * @brief Initialize the keyboard
 *
 */
void keyboard_init(void){
    int i;
    // disable all interrupts
    cli();

    // initialize the scanCodeArray
    for (i=0; i<scan_code_2_size; i++){
        simple_char[i] = 0;
    }
    // initialize simple_char
    /* map from scancode to ascii*/
    simple_char[0x15] =(uint8_t) 'q';
    simple_char[0x16] =(uint8_t) '1';
    simple_char[0x1a] =(uint8_t) 'z';
    simple_char[0x1b] =(uint8_t) 's';
    simple_char[0x1c] =(uint8_t) 'a';
    simple_char[0x1d] =(uint8_t) 'w';
    simple_char[0x1e] =(uint8_t) '2';
    simple_char[0x21] =(uint8_t) 'c';
    simple_char[0x22] =(uint8_t) 'x';
    simple_char[0x23] =(uint8_t) 'd';
    simple_char[0x24] =(uint8_t) 'e';
    simple_char[0x25] =(uint8_t) '4';
    simple_char[0x26] =(uint8_t) '3';
    simple_char[0x2a] =(uint8_t) 'v';
    simple_char[0x2b] =(uint8_t) 'f';
    simple_char[0x2c] =(uint8_t) 't';
    simple_char[0x2d] =(uint8_t) 'r';
    simple_char[0x2e] =(uint8_t) '5';
    simple_char[0x31] =(uint8_t) 'n';
    simple_char[0x32] =(uint8_t) 'b';
    simple_char[0x33] =(uint8_t) 'h';
    simple_char[0x34] =(uint8_t) 'g';
    simple_char[0x35] =(uint8_t) 'y';
    simple_char[0x36] =(uint8_t) '6';
    simple_char[0x3a] =(uint8_t) 'm';
    simple_char[0x3b] =(uint8_t) 'j';
    simple_char[0x3c] =(uint8_t) 'u';
    simple_char[0x3d] =(uint8_t) '7';
    simple_char[0x3e] =(uint8_t) '8';
    simple_char[0x42] =(uint8_t) 'k';
    simple_char[0x43] =(uint8_t) 'i';
    simple_char[0x44] =(uint8_t) 'o';
    simple_char[0x45] =(uint8_t) '0';
    simple_char[0x46] =(uint8_t) '9';
    simple_char[0x4B] =(uint8_t) 'l';
    simple_char[0x4D] =(uint8_t) 'p';

    // enable IRQ1
    enable_irq(KEYBOARD_IRQ);

    // enable interrupts
    sti();
}

/**
 * @brief keyboard interrupt handler
 *
 */
void keyboard_handler(void){
    cli();
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
    // printf("%c", simple_char[scan_code]); // output the ascii 
    // send_eoi(KEYBOARD_IRQ);
    sti();
    return ;
}

// void test_keyboard(){
//     /* lowercase, number */
// }
