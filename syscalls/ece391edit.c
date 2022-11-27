#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

#define NULL 0
#define NUM_COLS    80
#define NUM_ROWS    25
#define MAX_PAGE    4
#define MAX_SIZE    (NUM_COLS*NUM_ROWS*MAX_PAGE)

uint8_t  BUF[MAX_SIZE];
uint8_t* vmem_base_addr;

void
siguser_handler (int signum){
    uint8_t c[2];
    c[0]=ece391_getc();
    c[1]='\0';
    ece391_fdputs (1, c);
}

uint8_t*
set_video_mode (void)
{
    if(ece391_vidmap(&vmem_base_addr) == -1) {
        return NULL;
    } else {
        return vmem_base_addr;
    }
}

int main ()
{
    int32_t fd, cnt;
    uint8_t buf[1024];

    if (0 != ece391_getargs (buf, 1024)) {
        ece391_fdputs (1, (uint8_t*)"could not read arguments\n");
	    return 3;
    }

    if (-1 == (fd = ece391_open (buf))) {
        ece391_fdputs (1, (uint8_t*)"file not found\n");
	    return 2;
    }

    if(set_video_mode() == NULL) {
        return -1;
    }
    if(-1==ece391_set_handler(USER1,siguser_handler)){
        return 3;
    }


    while (0 != (cnt = ece391_read (fd, buf, 1024))) {
        if (-1 == cnt) {
	        ece391_fdputs (1, (uint8_t*)"file read failed\n");
            return 3;
        }
        if (-1 == ece391_write (1, buf, cnt))
            return 3;
    }
    while(1);

    return 0;
}

