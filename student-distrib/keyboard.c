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
 * @copyright Copyright (c) 2022
 * 
 */
#include "keyboard.h"

#define ISLOWER(x)  ('a' <= (x) && (x) <= 'z')
#define ISUPPER(x)  ('A' <= (x) && (x) <= 'Z')
#define TOLOWER(x)  ((x) + 'a' - 'A')
#define TOUPPER(x)  ((x) + 'A' - 'a')

// Global circular buffer with R/W/E indices.
input_t input;

// Modifiers flags.
static uint8_t mod;

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

static const uint8_t map_alt[MAP_SIZE] = {};

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

    return c;
}

/*!
 * @brief This function handles the keyboard interrupt.
 * @param None.
 * @return None.
 * @sideeffect Modifies `input` buffer and video memory.
 */
void
keyboard_handler(void) {
    int32_t c;
    int32_t i;
    send_eoi(KEYBOARD_IRQ);
    if (0 < (c = kgetc())) {  // Ignore NUL character.
        switch (c) {
            case '\b':  // Eliminate the last character in buffer & screen.
                if (input.e != input.w) {
                    putc(c);
                    --input.e;
                }
                break;
            case '\t':
                // Enough space to place a tab (4 spaces + 1 linefeed)?
                if (INPUT_SIZE < input.e - input.r + 5) { break; }
                for (i = 0; i < 4; ++i) {  // Substitute '\t' with four spaces for now.
                    putc(' ');
                    input.buf[input.e++ % INPUT_SIZE] = ' ';
                }
                break;
            case C('C'):
                halt(0);  // Assume normal termination for now.
                break;
            case C('L'):
                clear();
                printf("391OS> ");
                break;
            default:
                // Ignore NUL characters. Stop taking input when buffer is full.
                putc(c);  // Print non-NUL character to the screen.
                if (INPUT_SIZE == input.e - input.r + 1 && '\n' != c) { break; }
                input.buf[input.e++ % INPUT_SIZE] = c;  // NOTE: Circular buffer!
                // Update buffer write status when linefeed encountered so terminal can read.
                if ('\n' == c) { input.w = input.e; }
                break;
        }
    }
}
