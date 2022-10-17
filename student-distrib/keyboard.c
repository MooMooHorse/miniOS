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
static simple_char[scan_code_2_size];
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
    // disable all interrupts
    cli();

    // initialize the scanCodeArray
    for (int i=0; i<scan_code_2_size; i++){
        simple_char[i] = 0;
    }
    // initialize simple_char
    /* map from scancode to ascii*/
    simple_char[0x15] = "q";
    simple_char[0x16] = "1";
    simple_char[0x1a] = "z";
    simple_char[0x1b] = "s";
    simple_char[0x1c] = "a";
    simple_char[0x1d] = "w";
    simple_char[0x1e] = "2";
    simple_char[0x21] = "c";
    simple_char[0x22] = "x";
    simple_char[0x23] = "d";
    simple_char[0x24] = "e";
    simple_char[0x25] = "4";
    simple_char[0x26] = "3";
    simple_char[0x2a] = "v";
    simple_char[0x2b] = "f";
    simple_char[0x2c] = "t";
    simple_char[0x2d] = "r";
    simple_char[0x2e] = "5";
    simple_char[0x31] = "n";
    simple_char[0x32] = "b";
    simple_char[0x33] = "h";
    simple_char[0x34] = "g";
    simple_char[0x35] = "y";
    simple_char[0x36] = "6";
    simple_char[0x3a] = "m";
    simple_char[0x3b] = "j";
    simple_char[0x3c] = "u";
    simple_char[0x3d] = "7";
    simple_char[0x3e] = "8";
    simple_char[0x42] = "k";
    simple_char[0x43] = "i";
    simple_char[0x44] = "o";
    simple_char[0x45] = "0";
    simple_char[0x46] = "9";
    simple_char[0x4B] = "l";
    simple_char[0x4D] = "p";

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
    char scan_code = inb(KEYBOARD_PORT); //first byte NOT IN ASCII
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
    printf("%c", simple_char[scan_code]); // output the ascii 
    send_eoi(KEYBOARD_IRQ);
    sti();
    return ;
}

// void test_keyboard(){
//     /* lowercase, number */
// }