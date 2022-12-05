/**
 * @file ece391edit.c
 * @author haor2
 * @brief Editor that supports input output and arrow keys
 * @version 0.1
 * @date 2022-11-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

#define NULL 0
#define NUM_COLS    80
#define NUM_ROWS    25
#define MAX_PAGE    4
#define MAX_SIZE    (NUM_COLS*NUM_ROWS*MAX_PAGE)

#define ALT_BASE    128

#define TO_DIR(c)         (c+ALT_BASE)

#define LEFT_ARROW  TO_DIR(0x4B)
#define UP_ARROW    TO_DIR(0x48)
#define DOWN_ARROW  TO_DIR(0x50)
#define RIGHT_ARROW TO_DIR(0x4D)

#define IS_ARROW(c)       (c==LEFT_ARROW||c==UP_ARROW||c==DOWN_ARROW||c==RIGHT_ARROW)

#define VGA_WIDTH   80

uint8_t  BUF[MAX_SIZE];
int32_t  line_end[NUM_ROWS];
int32_t  buf_map[NUM_COLS][NUM_ROWS];
uint8_t* vmem_base_addr;
int32_t fd,flength,rtc_fd,rtc_val;
int32_t cx,cy;
int32_t x,y;
int32_t ocx,ocy;
int32_t BUF_s;

uint8_t ovga[NUM_ROWS*NUM_COLS];
uint8_t fname[1024];
uint8_t buf[1024];

void recover_video(){
    int32_t i,j,cnt=0;
    for(i=0;i<NUM_ROWS;i++)
        for(j=0;j<NUM_COLS;j++){
            vmem_base_addr[(cnt++)<<1]=ovga[i*NUM_COLS+j];
        }
}


void 
store_old_video(){
    int32_t i,j,cnt=0;
    for(i=0;i<NUM_ROWS;i++)
        for(j=0;j<NUM_COLS;j++){
            ovga[i*NUM_COLS+j]=vmem_base_addr[(cnt++)<<1];
        }
}

void vertical_scroll(int32_t x,int32_t y) { // not considering right now
    int32_t i;

    --y;  // Reset `y` to the 23rd line.

    // Shift rows up.
    for (i = 0; i < (NUM_ROWS - 1) * NUM_COLS; i++) {
        // Set current line to the next line, no need to change the color here.
        *(uint8_t *)(vmem_base_addr + (i << 1)) = *(uint8_t *)(vmem_base_addr + ((i + NUM_COLS) << 1));
    }

    // Clear the last line.
    for (i = (NUM_ROWS - 1) * NUM_COLS; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(vmem_base_addr + (i << 1)) = ' ';
    }
}

/**
 * @brief Caveat : This putc must not have '\b' as input
 * 
 * @param c 
 * @return ** int32_t 
 */
int32_t putc(uint8_t c){
    if ('\n' == c) {  // NOTE: Carriage return already converted to linefeed.
        line_end[y]=x;
        x = 0;
        if (NUM_ROWS == ++y) { // not considering right now
            // vertical_scroll(x,y);
            return -1;
        }
    } else{
        *(uint8_t *)(vmem_base_addr + ((NUM_COLS * y + x) << 1)) = c;
        if (NUM_COLS == ++x) {
            line_end[y]=x-1;
            x = 0;
            if (NUM_ROWS == ++y) { // not considering right now
                // vertical_scroll(x,y);
                return -1;
            }
        }
    }
    return 0;
}

void clear(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++)
        *(uint8_t *)(vmem_base_addr + (i << 1)) = ' ';
    x = y = 0;
    ece391_set_cursor(0, 0);
    cx=cy=0;
}


void show_text(){
    int32_t i=BUF_s;
    int32_t j=BUF_s;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) *(uint8_t *)(vmem_base_addr + (i << 1)) = ' ';
    for(x=0;x<NUM_COLS;x++) for(y=0;y<NUM_ROWS;y++) buf_map[x][y]=-1;
    for(i=0;i<flength;i++)
        if(i+1<flength&&BUF[i+1]!='\b'&&BUF[i]!='\b') BUF[j++]=BUF[i];
    BUF[j++]=BUF[flength-1];
    i=0;
    flength=j;
    x=0,y=0;
    buf_map[x][y]=i;
    while(i<flength &&-1!=putc(BUF[i++])){
        buf_map[x][y]=i;
    }

}

void BUF_shift_right(){
    int32_t i=buf_map[x][y],j;
    for(j=flength;j>i;j--){
        BUF[j]=BUF[j-1];
    }
    flength++;
}
// abandon for current version
// void BUF_shift_left(){
//     int32_t i=buf_map[x][y],j;
//     for(j=i-1;j<flength-1;j++){
//         BUF[j]=BUF[j+1];
//     }
//     flength--;
// }

void
siguser_handler (int signum){
    uint8_t c[2];
    c[0]=ece391_getc();
    c[1]='\0';
    if(IS_ARROW(c[0])){
        switch (c[0])
        {
        case UP_ARROW:
            if(cy&&buf_map[cx][cy-1]!=-1) cy--;
            break;
        case DOWN_ARROW:
            if(cy<NUM_ROWS-1&&buf_map[cx][cy+1]!=-1) cy++;
            break;  
        case LEFT_ARROW:
            if(cx&&buf_map[cx-1][cy]!=-1) cx--;
            break;
        case RIGHT_ARROW:
            if(cx<NUM_COLS-1&&buf_map[cx+1][cy]!=-1) cx++;
            break;
        default:
            break;
        }
        ece391_set_cursor(cx,cy);
    }else{
        x=cx,y=cy;
        
        BUF_shift_right();
        BUF[buf_map[x][y]]=c[0];
        if(c[0]=='\n'){
            cy++;
            cx=0;
        }else{
            cx++;
            cx%=NUM_COLS;
        }
        ece391_set_cursor(cx,cy);
    }
    // ece391_fdputs (1, c);

    show_text();
}

void
sigint_handler (int signum){
    ece391_set_handler(USER1,NULL);
    ece391_set_handler(INTERRUPT,NULL);
    ece391_set_cursor(ocx,ocy); /* recover cursor */
    recover_video();
    ece391_close(fd);
    fd=ece391_open(fname);
    if(-1==ece391_write(fd,BUF,flength)){
        ece391_fdputs (1, (uint8_t*)"write to file failed\n");
    }
    ece391_close(fd);
    ece391_halt(0);
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
    int32_t cnt,i,j;

    if (0 != ece391_getargs (fname, 1024)) {
        ece391_fdputs (1, (uint8_t*)"could not read arguments\n");
	    return 3;
    }

    if (-1 == (fd = ece391_open (fname))) {
        ece391_fdputs (1, (uint8_t*)"file not found\n");
	    return 2;
    }
    cx=ece391_get_cursor();
    cy=cx/VGA_WIDTH;
    cx=cx%VGA_WIDTH;
    ocx=cx,ocy=cy;

    
    if(set_video_mode() == NULL) {
        return -1;
    }

    store_old_video();


    if(-1==ece391_set_handler(USER1,siguser_handler)||-1==ece391_set_handler(INTERRUPT,sigint_handler)){
        return 3;
    }

    /* read one page, doesn't support scrowling */
    BUF_s=0;
    i=0;

    j=0;
    cnt = ece391_read (fd, buf, NUM_COLS*NUM_ROWS);
    if (-1 == cnt) {
        ece391_fdputs (1, (uint8_t*)"file read failed\n");
        return 3;
    }else{
        while(j<cnt){
            BUF[i++]=buf[j++];
        }
        flength+=cnt;
    }

    rtc_fd = ece391_open((uint8_t*)"rtc");
    rtc_val = 128;
    rtc_val = ece391_write(rtc_fd, &rtc_val, 4);
    clear();
    show_text();
    while(1){
        ece391_read(rtc_fd, &buf, 4); // buf = garbage 
    }   

    return 0;
}

