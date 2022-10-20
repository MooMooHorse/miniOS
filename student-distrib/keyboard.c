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
        if(i>=0x02 && i<=0x0B){/* digits */
            if(i==0x0B) simple_char[i]='0';
            else simple_char[i]=(char)(i-1+(uint8_t)('0'));
        }else{
            switch (i){
            // case 0x02: simple_char[i]='1'; break;
            // case 0x03: simple_char[i]='2'; break;
            // case 0x04: simple_char[i]='3'; break;
            // case 0x05: simple_char[i]='4'; break;
            // case 0x06: simple_char[i]='5'; break;
            // case 0x07: simple_char[i]='6'; break;
            // case 0x08: simple_char[i]='7'; break;
            // case 0x09: simple_char[i]='8'; break;
            // case 0x0A: simple_char[i]='9'; break;
            // case 0x0B: simple_char[i]='0'; break;
            // case 0x0C: simple_char[i]='-'; break;
            // case 0x0D: simple_char[i]='='; break;
            // case 0x0E: simple_char[i]='\b'; break;
            // case 0x0F: simple_char[i]='\t'; break;
            case 0x10: simple_char[i]='q'; break;
            case 0x11: simple_char[i]='w'; break;
            case 0x12: simple_char[i]='e'; break;
            case 0x13: simple_char[i]='r'; break;
            case 0x14: simple_char[i]='t'; break;
            case 0x15: simple_char[i]='y'; break;
            case 0x16: simple_char[i]='u'; break;
            case 0x17: simple_char[i]='i'; break;
            case 0x18: simple_char[i]='o'; break;
            case 0x19: simple_char[i]='p'; break;
            // case 0x1A: simple_char[i]='['; break;
            // case 0x1B: simple_char[i]=']'; break;
            // case 0x1C: simple_char[i]='\r'; break;
            // case 0x1D: simple_char[i]='\0'; break;
            case 0x1E: simple_char[i]='a'; break;
            case 0x1F: simple_char[i]='s'; break;
            case 0x20: simple_char[i]='d'; break;
            case 0x21: simple_char[i]='f'; break;
            case 0x22: simple_char[i]='g'; break;
            case 0x23: simple_char[i]='h'; break;
            case 0x24: simple_char[i]='j'; break;
            case 0x25: simple_char[i]='k'; break;
            case 0x26: simple_char[i]='l'; break;
            // case 0x27: simple_char[i]=';'; break;
            // case 0x28: simple_char[i]=''''; break;
            // case 0x29: simple_char[i]='`'; break;
            // case 0x2A: simple_char[i]='\0'; break;
            // case 0x2B: simple_char[i]='\\'; break;
            case 0x2C: simple_char[i]='z'; break;
            case 0x2D: simple_char[i]='x'; break;
            case 0x2E: simple_char[i]='c'; break;
            case 0x2F: simple_char[i]='v'; break;
            case 0x30: simple_char[i]='b'; break;
            case 0x31: simple_char[i]='n'; break;
            case 0x32: simple_char[i]='m'; break;
            // case 0x33: simple_char[i]=','; break;
            // case 0x34: simple_char[i]='.'; break;
            // case 0x35: simple_char[i]='/'; break;
            // case 0x36: simple_char[i]='\0'; break;

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
    sti();
    return ;
}

// void test_keyboard(){
//     /* lowercase, number */
// }
