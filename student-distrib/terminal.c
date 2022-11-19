/*!
 * @file terminal.c
 * @author Scharfrichter Qian
 * @brief This file contains the implementation of the terminal operations.
 * @version 1.0
 * @date 2022-10-21
 * 
 * @version 2.0
 * @author haor2
 * @brief support multiple terminals
 * @date 2022-11-18
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "terminal.h"
#include "mmu.h"
#include "tests.h"
#include "process.h"

static int32_t _terminal_read(uint8_t* buf, int32_t n);
static int32_t _terminal_write(const uint8_t* buf, int32_t n);
static int32_t _open(int32_t,int32_t*);

int32_t terminal_index; /* global variable : track current displayed terminal in OS */

terminal_ops_t terminal_ops = {
    .ioctl.open = terminal_open,
    .ioctl.close = terminal_close,
    .ioctl.read = terminal_read,
    .ioctl.write = terminal_write
};

terminal_t terminal[MAX_TERMINAL_NUM]={
    {.open=_open,.active=0,.index=0},/* terminal 0 */
    {.open=_open,.active=0,.index=1},/* terminal 1 */
    {.open=_open,.active=0,.index=2},/* terminal 2 */
    {.open=_open,.active=0,.index=3},/* terminal 3 */
    {.open=_open,.active=0,.index=4},/* terminal 4 */
    {.open=_open,.active=0,.index=5},/* terminal 5 */
    {.open=_open,.active=0,.index=6},/* terminal 6 */
    {.open=_open,.active=0,.index=7},/* terminal 7 */
    {.open=_open,.active=0,.index=8},/* terminal 8 */
    {.open=_open,.active=0,.index=9} /* terminal 9 */
};


/**
 * @brief Update OS video setting to corresponding terminal setting
 * Usage : 
 * In terminal switch
 * when switching to new terminal, prog_video_update(new)
 * 
 * In context switch
 * look at pcb field to find corresponding terminal index, and use this function,
 * so the output(e.g. printf) under background program will go to background terminal 
 * buffer instead of being displayed directly
 * 
 * @param index 
 * @return ** int32_t 
 */
int32_t prog_video_update(int32_t index){
    if(index<0||index>9){
        return -1;
    }
    set_vid((char*)terminal[index].video,terminal[index].screen_x,terminal[index].screen_y);
    return 0;
}

/**
 * @brief recover the terminal into set state 
 * 
 * @param video - video memory for current program
 * @param sx    - cursor pos for current program
 * @param sy    - cursor pos for current program
 * @return ** void 
 */
void prog_video_recover(char* video,int sx,int sy){
    terminal[terminal_index].screen_x=get_screen_x();
    terminal[terminal_index].screen_y=get_screen_y();
    set_vid(video,sx,sy);
}

/**
 * @brief load the terminal for their video, screen_x, screen_y
 * Shouldn't be used in keyboard interrupt handler and context switch
 * @param index - terminal index to load
 * @return ** int32_t -1 on illegal index
 * 0 on success
 * SIDE-EFFECT : copy the whole video memory : slow
 */
int32_t terminal_load(int32_t index){
    int32_t i;
    if(index<0||index>9){
        return -1;
    }
    char* vid=(char*)VIDEO;
    /* copy the video memory into terminal buffer */
    /* TO DO : Optimize it so you only have to copy chars you wrote */
    /* it's NOT looping from old coordinate to new coordinate, that's buggy */
    for(i=0;i<VIDEO_SIZE;i++) terminal[index].video[i]=vid[i]; 
    terminal[index].screen_x=get_screen_x();
    terminal[index].screen_y=get_screen_y();
    return 0;
}

/**
 * @brief switch the terminal
 * 
 * @param old - old terminal index
 * @param new - new terminal index
 * @return ** int32_t 
 */
int32_t terminal_switch(int32_t old,int32_t new){
    
    terminal_load(old);
    terminal_index=new;
    if(!terminal[new].active) terminal[new].open(new,(int32_t*)get_terbuf_addr(new));
    prog_video_update(new);
    /* initalize screen if terminal is just created(opened) */
    if(terminal[new].screen_x==terminal[new].screen_y&&terminal[new].screen_x==0){
        clear();
        printf("391OS> ");
    }
    return 0;
}


/**
 * @brief open terminal
 * 
 * @param index - terminal index : 0~9
 * @param video - terminal video buffer pointer
 * @return ** int32_t -1 on illegal parameters
 * 0 normal return 
 */
int32_t _open(int32_t index,int32_t* video){
    if(index<0||index>9||(uint32_t)video<VIDEO+VIDEO_SIZE||(uint32_t)video>VIDEO+10*VIDEO_SIZE){
    #ifdef RUN_TESTS
        printf("invalid terminal number or buffer address\n");
    #endif
        return -1;
    }
    terminal[index].input.e=terminal[index].input.w=terminal[index].input.r=0;
    terminal[index].screen_x=terminal[index].screen_y=0;
    terminal[index].video=(int8_t*)video;
    uvmmap_tbuf((uint32_t)video); /* set up terminal buffer */
    terminal[index].active=1;
    return 0;
}

/* abandoned for cp5 */
// /*!
//  * @brief This function initializes the terminal to the expected initial state.
//  * @param None.
//  * @return None.
//  * @sideeffect Initializes `input` buffer indices.
//  */
// void
// terminal_init(void) {
//     input.r = input.w = input.e = 0;
// }

/*!
 * @brief Abandoned for cp5
 */
int32_t
terminal_open(__attribute__((unused)) file_t* file, __attribute__((unused)) const uint8_t* buf, __attribute((unused)) int32_t nbytes) {
    // terminal_init();
    return 0;
}

/*!
 * @brief Device driver interface. Used to close the terminal.
 * @param Determined by OS device driver interface.
 * @return 0 on success, non-zero otherwise.
 * @sideeffect Discard remaining input characters in the `input` buffer.
 */
int32_t
terminal_close(file_t* file) {
    file->fops.close = NULL;
    file->fops.read = NULL;
    file->fops.write = NULL;
    file->fops.open = NULL;
    file->flags = F_CLOSE;
    file->pos = 0;
    file->inode = -1;
    
    return 0;
}

/*!
 * @brief Device driver interface. Used to read `nbytes` bytes from the terminal.
 * @param file is file struct (unused).
 * @param buf is pointer to input buffer.
 * @param nbytes is number of characters the caller intended to read.
 * @return number of characters actually read.
 * @sideeffect See `_terminal_read` below.
 */
int32_t
terminal_read(__attribute__((unused)) file_t* file, void* buf, int32_t nbytes) {
    return _terminal_read((uint8_t*) buf, nbytes);
}

/*!
 * @brief Device driver interface. Used to write `nbytes` bytes to the terminal.
 * @param file is file struct (unused).
 * @param buf is pointer to input buffer.
 * @param nbytes is number of character the caller intended to write.
 * @return number of characters actually wrote.
 * @sideeffect See `_terminal_write` below.
 */
int32_t
terminal_write(__attribute__((unused)) file_t* file, const void* buf, int32_t nbytes) {
    return _terminal_write((const uint8_t*) buf, nbytes);
}

/*!
 * @brief This function reads `n` character from the `input` buffer of the keyboard driver and
 * writes them to the input buffer `buf`.
 * @param buf is input buffer to be written with characters.
 * @param n is number of characters the caller intended to read.
 * @return actual number of characters read from `input` buffer.
 * @sideeffect Updates `input` buffer read index.
 */
static int32_t
_terminal_read(uint8_t* buf, int32_t n) {
    int32_t target = n;
    uint8_t c = '\0';
    uint32_t pid=get_pid();
    pcb_t* _pcb_ptr=(pcb_t*)(PCB_BASE-pid*PCB_SIZE);
    int32_t prog_terminal=_pcb_ptr->terminal; /* terminal index for current running program : background/foreground */

    if (NULL == buf || 0 > n) { return -1; }  // Invalid input parameter!

    while (0 < n && '\n' != c) {  // Stop when '\n' reached or `n` characters read.
        while (terminal[prog_terminal].input.w == terminal[prog_terminal].input.r);  // Waiting for new user input in input buffer.
        c = terminal[prog_terminal].input.buf[
            terminal[prog_terminal].input.r++ % INPUT_SIZE
        ];  // NOTE: Circular buffer.
        *buf++ = c;
        --n;  // NOTE: Linefeed ('\n') is written to the buffer AND counted.
    }

    return target - n;
}

/*!
 * @brief This function writes `n` characters in the given buffer to the screen.
 * @param buf is the input buffer containing characters to be displayed.
 * @param n is number of characters to write to the screen.
 * @return number of characters successfully written to the screen.
 * @sideeffect Modifies video memory content.
 */
static int32_t
_terminal_write(const uint8_t* buf, int32_t n) {
    int i;

    if (NULL == buf) {
        return -1;  // Invalid buffer.
    }

    for (i = 0; i < n; ++i) {
        putc(buf[i]);
    }

    return n;
}

/**
 * @brief Get the starting address of corresponding terminal buffer
 * 
 * @param index - terminal index (0~9)
 * @return ** int32_t 
 */
uint32_t get_terbuf_addr(int32_t index){
    // no sanity check here 
    return VIDEO+(terminal_index+1)*VIDEO_SIZE;
}

