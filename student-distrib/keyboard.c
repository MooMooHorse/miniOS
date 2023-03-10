/**
 * @file keyboard.c
 * @author tufekci2
 * @brief keyboard related functions, including handler
 * @version 0.1
 * @date 2022-10-16
 *
 * @version 1.0
 * @author Scharfrichter Qian
 * @date 2022-10-21
 * @brief Refactored code to support special characters and input buffer.
 *
 * @version 2.0
 * @author haor2
 * @date 2022-11-18
 * @brief support multiple terminals
 * 
 * @version 3.0
 * @author haor2
 * @date 2022-11-28
 * @brief support signal, and user level program input handling
 * 
 * @copyright Copyright (c) 2022
 */
#include "keyboard.h"
#include "terminal.h"
#include "lib.h"
#include "mmu.h"
#include "process.h"
#include "tests.h"
#include "cursor.h"
#include "signal.h"

#define ISLOWER(x)  ('a' <= (x) && (x) <= 'z')
#define ISUPPER(x)  ('A' <= (x) && (x) <= 'Z')
#define TOLOWER(x)  ((x) + 'a' - 'A')
#define TOUPPER(x)  ((x) + 'A' - 'a')
#define ISALT(x)    (x>ALT_BASE&&x!=A(1)&&x!=A(2)&&x!=A(3))

// Global circular buffer with R/W/E indices.
// input_t input;

// Modifiers flags.
static uint8_t mod;
static uint8_t user_c;

static const uint8_t map_basics[MAP_SIZE] = {
    [2] = '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', '\0', 'a',
    's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', '\0', '\\', 'z', 'x',
    'c', 'v', 'b', 'n', 'm', ',', '.', '/', [57] = ' '
};

static const uint8_t map_shift[MAP_SIZE] = {
    [2] = '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', '\0', 'A',
    'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', '\0', '|', 'Z', 'X',
    'C', 'V', 'B', 'N', 'M', '<', '>', '?', [57] = ' '
};

#define C(x) ((x) - '@')
static const uint8_t map_ctrl[MAP_SIZE] = {
    [28] = '\n' /* \r */, [38] = C('L'), [46] = C('C')
};

/**
 * @brief 
 * 0x3B    F1 pressed  0x3C	   F2 pressed	0x3D    F3 pressed	
 * 0x3E    F4 pressed  0x3F	   F5 pressed   0x40    F6 pressed	
 * 0x41	   F7 pressed  0x42	   F8 pressed	0x43	F9 pressed
 * 0x44    F10 pressed 
 * 
 */
#define A(X) ((X)+ALT_BASE)
static const uint8_t map_alt[MAP_SIZE] = {
    [0x10]=A('q'), [0x3B]=A(1), [0x3C]=A(2), [0x3D]=A(3)
    // ,
    // [0x3E]=A(4), [0x3F]=A(5), [0x40]=A(6),
    // [0x41]=A(7), [0x42]=A(8), [0x43]=A(9),
    // [0x44]=A(10)
};

// Complete CTRL map.
/* static const uint8_t map_ctrl[MAP_SIZE] = { */
/*     [16] = C('Q'), C('W'), C('E'), C('R'), C('T'), C('Y'), C('U'), C('I'), */
/*     C('O'), C('P'), C('['), C(']'), '\n' /1* \r *1/, '\0', C('A'), C('S'), C('D'), */
/*     C('F'), C('G'), C('H'), C('J'), C('K'), C('L'), [43] = C('\\'), C('Z'), C('X'), */
/*     C('C'), C('V'), C('B'), C('N'), C('M'), [53] = C('/') */
/* }; */

// Scan Code --> Bitmask in `mod`.
static const uint8_t map_mod_or[MAP_SIZE] = {
    [0x2A] = SHIFT, [0x36] = SHIFT, [0x1D] = CTRL, [0x38] = ALT
};

// Scan Code --> Bitmask in `mod`.
static const uint8_t map_mod_xor[MAP_SIZE] = {
    [0x3A] = CAPSLOCK  // Others currently not supported.
};

// So we can use `code & (ALT | CTRL | SHIFT)` to select the corresponding map.
static const uint8_t* const map[8] = {
    map_basics, map_shift, map_ctrl, map_ctrl,  // SHIFT ignored when CTRL is pressed.
    map_alt, map_alt, map_alt, map_alt  // Redirect characters with ALT pressed to empty map for now.
};

/**
 * @brief This function initializes the keyboard.
 * NOTE: Should be called when IF is cleared.
 * @param None.
 * @return None.
 * @sideeffect Enables IRQ for the keyboard!
 */
void keyboard_init(void) {
    enable_irq(KEYBOARD_IRQ);  // Enable IRQ1.
}

/*!
 * @brief This function reads one scan code from the keyboard data port and convert
 * it to an ASCII character or update the corresponding status variable (e.g. `mod`).
 * @param None.
 * @return (Sign-extended) ASCII character. -1 if error occurred.
 * @sideeffect Consumes scan code in the keyboard data port and modifies `mod` vector.
 */
static int32_t
kgetc(void) {
    uint8_t c;
    int32_t i;
    uint8_t stat = inb(KEYBOARD_STAT);
    if (!(stat & 0x01)) { return -1; }  // ERROR: Empty keyboard buffer!
    uint8_t code = inb(KEYBOARD_DATA) & SCAN_MASK;  // Extract the least-significant byte.

    // Scan codes indicating "key release" has the HIGHEST bit set.
    if (code & RELEASE_MASK) {
        // Extract the corresponding "pressed" scan code for modifier keys (no effects on other keys).
        mod &= ~map_mod_or[code & ~RELEASE_MASK];
        return 0;  // Done processing.
    }

    // Update modifier key status. No effect if none is active.
    mod |= map_mod_or[code];
    mod ^= map_mod_xor[code];
    c = map[mod & (ALT | CTRL | SHIFT)][code];
    if (mod & CAPSLOCK) {  // CAPS LOCK reverts the cases of alphabetics.
        c = ISLOWER(c) ? TOUPPER(c) : ISUPPER(c) ? TOLOWER(c) : c;
    }

    if(code==DOUBLE_WORD||IS_DIR(code)){
        user_c=TO_DIR(code);
    }else if(ISLOWER(c)||ISUPPER(c)||IS_DIGIT(c)||IS_SPECIAL(c)||ISALT(c)){
        user_c=c;
    }else{
        /* keyboard enable doesn't have any effect on not-selected characters */
        return c;
    }
    /* for all the selected characters, we singal the currently displaying & running user programs */
    for(i=1;i<=PCB_MAX;i++){
        if(PCB(i)->terminal==terminal_index&&
        (PCB(i)->state==RUNNING||PCB(i)->state==RUNNABLE)){
            PCB(i)->sig_num=SIG_USER1;
            break;
        }
    }
    if(!PCB(i)->keyboard_enable) c=0;
    return c;
}


/*!
 * @brief This function handles the keyboard interrupt.
 * Handling keyboard always use the current terminal (active).
 * @param None.
 * @return None.
 * @sideeffect Modifies `input` buffer and video memory.
 */
void
keyboard_handler(void) {
    int32_t c;
    int32_t i;
    
    /* if scheduler is running background process, keyboard is still outputting to displayed terminal */
    uint32_t pid=get_pid();
    pcb_t* _pcb_ptr=(pcb_t*)(PCB_BASE-pid*PCB_SIZE);


    send_eoi(KEYBOARD_IRQ);
    if (0 < (c = kgetc())) {  // Ignore NUL character.
        
        if(get_graphics()) return ; //skip when in graphic mode  

        if(c > ALT_BASE){ /* switch terminal */
            if (c - ALT_BASE > MAX_TERMINAL_NUM){
                #ifdef RUN_TESTS
                printf("bad ascii\n");
                #endif
            } else {
                terminal_switch(terminal_index, (c - ALT_BASE) % MAX_TERMINAL_NUM); /* terminal 10 -> 0 */
            }
            return;
        }
        if(IS_DIR(c)){ /* nothing shows on screen, so no need to reset screen_x,screen_y */
            if (INPUT_SIZE < terminal[terminal_index].input.e 
            - terminal[terminal_index].input.r + 1) { return ; }
            terminal[terminal_index].input.buf[
                terminal[terminal_index].input.e++ % INPUT_SIZE
            ] = c;
            return ;
        }
        switch (c) {
            case '\b':  // Eliminate the last character in buffer & screen.
                if (terminal[terminal_index].input.e != terminal[terminal_index].input.w) {
                    kputc(c);
                    --terminal[terminal_index].input.e;
                }
                break;
            case '\t':
                /* current version leaves handling tab to user level programs */
                // Enough space to place a tab (4 spaces + 1 linefeed)?
                // if (INPUT_SIZE < terminal[terminal_index].input.e 
                // - terminal[terminal_index].input.r + 5) { break; }
                // for (i = 0; i < 4; ++i) {  // Substitute '\t' with four spaces for now.
                //     kputc(' ');
                //     terminal[terminal_index].input.buf[
                //         terminal[terminal_index].input.e++ % INPUT_SIZE
                //     ] = ' ';
                // }
                // enough space for a '\t'  ?
                if (INPUT_SIZE < terminal[terminal_index].input.e 
                - terminal[terminal_index].input.r + 1) { break; }
                terminal[terminal_index].input.buf[
                    terminal[terminal_index].input.e++ % INPUT_SIZE
                ] = c;
                /* no printing in screen at current version */
                break;
            case C('C'):
                for(i=1;i<=PCB_MAX;i++){
                    pcb_t*  _pcb_ptr=(pcb_t*)(PCB_BASE-i*PCB_SIZE); 
                    if(_pcb_ptr->terminal==terminal_index&&
                        (_pcb_ptr->state==RUNNABLE||_pcb_ptr->state==RUNNING)
                    ){
                        set_async_signal(SIG_INTERRUPT,i);
                        break;
                    }        
                }
                break;
            case C('L'):
                kclear();
                kputs("391OS> ");
                break;
            default:
                // Ignore NUL characters. Stop taking input when buffer is full.
                if (INPUT_SIZE == 
                terminal[terminal_index].input.e 
                - terminal[terminal_index].input.r + 1
                && '\n' != c) {
                    break;
                }

                kputc(c);  // Print non-NUL character to the screen.
                terminal[terminal_index].input.buf[
                    terminal[terminal_index].input.e++ % INPUT_SIZE
                ] = c;  // NOTE: Circular buffer!
                // Update buffer write status when linefeed encountered so terminal can read.
                if ('\n' == c) { 
                    terminal[terminal_index].input.w = terminal[terminal_index].input.e; 
                }
                break;
        }
    }

    if(_pcb_ptr->terminal==terminal_index){
        set_vid((char*)VIDEO,terminal[terminal_index].screen_x,terminal[terminal_index].screen_y);
    }
}

uint8_t get_c(){
    uint8_t ret=user_c;
    user_c=0;
    return ret;
}
