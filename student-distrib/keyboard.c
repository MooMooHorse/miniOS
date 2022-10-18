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

static char simple_char[SCAN_CODE_SIZE];

/**
 * @brief Initialize the keyboard
 * Should be ran when CLI is called 
 */
void keyboard_init(void){
    int i;

    // initialize the scanCodeArray
    for (i=0; i<SCAN_CODE_SIZE; i++)
        simple_char[i]=0;

    simple_char[0x02]='1'; 
    simple_char[0x03]='2'; 
    simple_char[0x04]='3';
    simple_char[0x05]='4';
    simple_char[0x06]='5';
    simple_char[0x07]='6';
    simple_char[0x08]='7';
    simple_char[0x09]='8';
    simple_char[0x0A]='9';
    simple_char[0x0B]='0';
    simple_char[0x0C]='-';
    simple_char[0x0D]='=';
    simple_char[0x0E]='\b';
    simple_char[0x0F]='\t';
    simple_char[0x10]='q';
    simple_char[0x11]='w';
    simple_char[0x12]='e';
    simple_char[0x13]='r';
    simple_char[0x14]='t';
    simple_char[0x15]='y';
    simple_char[0x16]='u';
    simple_char[0x17]='i';
    simple_char[0x18]='o';
    simple_char[0x19]='p';
    simple_char[0x1A]='[';
    simple_char[0x1B]=']';
    simple_char[0x1C]='\r';
    simple_char[0x1D]='\0';
    simple_char[0x1E]='a';
    simple_char[0x1F]='s';
    simple_char[0x20]='d';
    simple_char[0x21]='f';
    simple_char[0x22]='g';
    simple_char[0x23]='h';
    simple_char[0x24]='j';
    simple_char[0x25]='k';
    simple_char[0x26]='l';
    simple_char[0x27]=';';
    simple_char[0x28]='\'';
    simple_char[0x29]='`';
    simple_char[0x2A]='\0';
    simple_char[0x2B]='\\';
    simple_char[0x2C]='z';
    simple_char[0x2D]='x';
    simple_char[0x2E]='c';
    simple_char[0x2F]='v';
    simple_char[0x30]='b';
    simple_char[0x31]='n';
    simple_char[0x32]='m';
    simple_char[0x33]=',';
    simple_char[0x34]='.';
    simple_char[0x35]='/';
    simple_char[0x36]='\0';

    // initialize simple_char
    /* map from scancode to ascii*/

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
    if((simple_char[scan_code]>='a'&&simple_char[scan_code]<='z')||
    (simple_char[scan_code]>='0'&&simple_char[scan_code]<='9'))
        putc(simple_char[scan_code]); // output the ascii 
    send_eoi(KEYBOARD_IRQ);
}
