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
static char simple_char[SCAN_CODE_SIZE];
// static compound_char[];

// void flush_the_buffer(){
//     // printf(" "); printing the buffer
//     buffer_pt=0;
// }

/**
 * @brief Initialize the keyboard
 * Should be ran when CLI is called 
 */
void keyboard_init(void){
    int i;

    // initialize the scanCodeArray
    for (i=0; i<SCAN_CODE_SIZE; i++){
        if(i>=0x02&&i<=0x0B){/* digits */
            if(i==0x0B) simple_char[i]='0';
            else simple_char[i]=(char)(i-1+(uint8_t)('0'));
        }else{
            switch (i){
            case 0x10:
                simple_char[i]='q';
                break;
            case 0x11:
                simple_char[i]='w';
                break;
            case 0x12:
                simple_char[i]='e';
            break;
            case 0x13:
                simple_char[i]='r';
            break;
            /* undefined right now */
            default:
                simple_char[i]=0;
                break;
            }
        }
    }
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
    if((simple_char[scan_code]>='a'&&simple_char[scan_code]<='z')||
    (simple_char[scan_code]>='0'&&simple_char[scan_code]<='9'))
        putc(simple_char[scan_code]); // output the ascii 
    send_eoi(KEYBOARD_IRQ);
    sti();
    return ;
}

// void test_keyboard(){
//     /* lowercase, number */
// }
