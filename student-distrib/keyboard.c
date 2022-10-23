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
// static uint32_t buffer_pt;
static char buf[BUFFER_SIZE];

// modifier key flags
int shift_flag = 0;
int caps_flag = 0;
int ctrl_flag = 0;
int alt_flag = 0;
int keyCount = 0;
// static compound_char[];

static const char simple_char[SCAN_CODE_SIZE] = {
    [2] = '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', '\0', 'a',
    's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', '\0', '\\', 'z', 'x',
    'c', 'v', 'b', 'n', 'm', ',', '.', '/', '\0', '\0', '\0', ' '}; // 0x39 

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

        // Shift Release (L)
        case (0xAA):
            shift_flag = 0;
            break;

        // Shift Press (R)
        case (0x36):
            shift_flag = 1;
            break;

        // Shift Release (R)
        case (0xB6):
            shift_flag = 0;
            break;

        // Ctrl Press (L/R)
        case (0x1D):
            ctrl_flag = 1;
            break;

        // Ctrl Release (L/R)
        case (0x9D):
            ctrl_flag = 0;
            break;

        // Alt Press (L/R)
        case (0x38):
            alt_flag = 1;
            break;
        
        // Alt Release (L/R)
        case (0xB8):
            alt_flag = 0;
            break;

        // Caps Lock
        case (0x3A):
            caps_flag = !caps_flag;
            break;
        
        // case (0xE0):
        //     extended_flag = 1;
        //     break;
    }

    if (scan_code < SCAN_CODE_SIZE) {
        char c = simple_char[scan_code];
        if (c != '\0') {
            if (shift_flag ^ caps_flag) {
                c = c - 32;
            }
            if (ctrl_flag) {
                if (c == 'l') { // clear terminal (ctrl+l)
                    clear_screen();
                    send_eoi(KEYBOARD_IRQ);
                    return;
                }
            }

            if (keyCount < 80) {
                buf[keyCount++] = c;
            }
            else {
                handle_vertical_scroll();
                keyCount = 1;
            }

            putc(c);
        }
    }

    send_eoi(KEYBOARD_IRQ);
}


// to be added/integrated into terminal.c/h
#define VIDEO_MEM_ADDR 0xB8000
#define NUM_COLS 80
#define NUM_ROWS 25

static char* VIDEO_MEM = (char *)VIDEO_MEM_ADDR;
static char buf[BUFFER_SIZE];

void clear_buffer(void) {
    int32_t i;
    for (i = 0; i < BUFFER_SIZE; i++) {
        buf[i] = '\0';
    }
    keyCount = 0;
}

void clear_screen (void) {
    int32_t mem_index;

    for (mem_index = 0; mem_index < NUM_ROWS * NUM_COLS; mem_index++) {
        *(uint8_t *)(VIDEO_MEM + (mem_index << 1)) = ' ';
        *(uint8_t *)(VIDEO_MEM + (mem_index << 1) + 1) = 0x7;
    }

    clear_buffer();
    // // Hey Brant! I added this to reset cursor position when we clear the screen on terminal
    // screen_x = 0;
    // screen_y = 0;
    return;
}

void handle_vertical_scroll(void) {
    int32_t i;

    // shift rows up
    for (i = 0; i < (NUM_ROWS - 1) * NUM_COLS; i++) {
        // set current line to the next line
        *(uint8_t *)(VIDEO_MEM + (i << 1)) = *(uint8_t *)(VIDEO_MEM + ((i + NUM_COLS) << 1));
        *(uint8_t *)(VIDEO_MEM + (i << 1) + 1) = 0x7;
    }

    // clear last line
    for(i = (NUM_ROWS - 1) * NUM_COLS; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(VIDEO_MEM + (i << 1)) = ' ';
        *(uint8_t *)(VIDEO_MEM + (i << 1) + 1) = 0x7;
    }
    
    // Hey Brant! You probably need to adjust the cursor/prompt here after the vertical scroll
    return;
}