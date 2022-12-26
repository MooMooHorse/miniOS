/**
 * @file ece391shell.c
 * @author haor2
 * @brief  shell that supports shortcut 
 * @version 0.1
 * @date 2022-11-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

#define BUFSIZE 1024
#define HIST_LEN 100
#define KBUF_LEN 128

#define NULL 0
#define NUM_COLS    80
#define NUM_ROWS    25
#define MAX_PAGE    4
#define MAX_SIZE    (NUM_COLS*NUM_ROWS*MAX_PAGE)

#define IS_ARROW(c)       (c==0x4B||c==0x48||c==0x50||c==0x4D)

#define LEFT_ARROW  0x4B
#define UP_ARROW    0x48
#define DOWN_ARROW  0x50
#define RIGHT_ARROW 0x4D

#define VGA_WIDTH   80

uint8_t hist_buf[HIST_LEN][KBUF_LEN];
uint32_t hist;


int main ()
{
    int32_t cnt, rval;
    uint8_t buf[BUFSIZE];
    ece391_fdputs (1, (uint8_t*)"Starting 391 Shell\n");

    while (1) {
        ece391_fdputs (1, (uint8_t*)"391OS> ");
	while(-1 != (cnt = ece391_read (0, buf, BUFSIZE-1))) {
        if (cnt > 0 && '\n' == buf[cnt - 1]) break;
        if(cnt>0&&'\t'==buf[cnt-1]){ /* fill in shortcut */
            
        }
        if(cnt>0&&IS_ARROW(buf[cnt-1])){ /* change command shortcut */

        }
        
    }
    if(cnt==-1){
	    ece391_fdputs (1, (uint8_t*)"read from keyboard failed\n");
	    return 3;
    }
    /* buf[cnt] can only be '\n' now */
    cnt--;
    buf[cnt] = '\0';
	if (0 == ece391_strcmp (buf, (uint8_t*)"exit"))
	    return 0;
	if ('\0' == buf[0])
	    continue;

	rval = ece391_execute (buf);
	if (-1 == rval)
	    ece391_fdputs (1, (uint8_t*)"no such command\n");
	else if (256 == rval)
	    ece391_fdputs (1, (uint8_t*)"program terminated by exception\n");
	else if (0 != rval)
	    ece391_fdputs (1, (uint8_t*)"program terminated abnormally\n");
    }
}

