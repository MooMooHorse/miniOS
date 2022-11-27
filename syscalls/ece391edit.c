#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"


#define NUM_COLS    80
#define NUM_ROWS    25
#define MAX_PAGE    8

uint8_t *vmem_base_addr;
uint8_t  BUF[NUM_COLS*NUM_ROWS*MAX_PAGE];
int32_t st,ed; /* start and end pointer to BUF for displayed text */


uint8_t*
set_video_mode (void)
{
    if(ece391_vidmap(&vmem_base_addr) == -1) {
        return (uint8_t*)0;
    } else {
        return vmem_base_addr;
    }
}

void
user1_handler (int signum){

}


int main ()
{
    int32_t fd, cnt, i, j;
    uint8_t buf[1024];
    if (0 != ece391_getargs (buf, 1024)) {
        ece391_fdputs (1, (uint8_t*)"could not read arguments\n");
	return 3;
    }

    if (-1 == (fd = ece391_open (buf))) {
        ece391_fdputs (1, (uint8_t*)"file not found\n");
	return 2;
    }

    /* init BUF */
    for(i=0;i<NUM_COLS*NUM_ROWS*MAX_PAGE;i++){
        BUF[i]='\0';
    }

    i=0;

    while (0 != (cnt = ece391_read (fd, buf, 1024))) {
        j=0;
        if (-1 == cnt) {
            ece391_fdputs (1, (uint8_t*)"file read failed\n");
            return 3;
        }
        while(cnt--) BUF[i++]=buf[j++];
        if(i==NUM_COLS*NUM_ROWS*MAX_PAGE){ /* upper limit of text buffer */
            break;
        }
    }

    if((uint8_t*)0==set_video_mode()){
        ece391_fdputs (1, (uint8_t*)"bad video map\n");
        return 3;
    }

    st=0;
    ed=NUM_COLS*NUM_ROWS-1;
    j=0;
    for(i=st;i<=ed;i++) vmem_base_addr[i<<1]=BUF[i];
    

    while(1);

    return 0;
}

