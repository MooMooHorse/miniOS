/**
 * @file vga.c
 * @author 
 * Steve Lumetta 
 * haor2
 * @brief mp2 VGA setting adapted to our own OS
 * @version 0.1
 * @date 2022-12-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "vga.h"
#include "lib.h"
#include "mmu.h"
#include "filesystem.h"


#define IMAGE_X_DIM     320   /* pixels; must be divisible by 4             */
#define IMAGE_Y_DIM     200   /* pixels                                     */
#define IMAGE_X_WIDTH   (IMAGE_X_DIM / 4)          /* addresses (bytes)     */
#define SCROLL_X_DIM	IMAGE_X_DIM                /* full image width      */
#define SCROLL_Y_DIM    IMAGE_Y_DIM                /* full image width      */
#define SCROLL_X_WIDTH  (IMAGE_X_DIM / 4)          /* addresses (bytes)     */

#define SCROLL_SIZE     (SCROLL_X_WIDTH * SCROLL_Y_DIM)
#define SCREEN_SIZE	(SCROLL_SIZE * 4 + 1)
#define BUILD_BUF_SIZE  (SCREEN_SIZE + 20000) 
#define BUILD_BASE_INIT ((BUILD_BUF_SIZE - SCREEN_SIZE) / 2)

/* Mode X and general VGA parameters */
#define VID_MEM_SIZE       131072
#define MODE_X_MEM_SIZE     65536
#define NUM_SEQUENCER_REGS      5
#define NUM_CRTC_REGS          25
#define NUM_GRAPHICS_REGS       9
#define NUM_ATTR_REGS          22

#if !defined(NDEBUG)
#define MEM_FENCE_WIDTH 256
#else
#define MEM_FENCE_WIDTH 0
#endif
#define MEM_FENCE_MAGIC 0xF3

static unsigned char build[BUILD_BUF_SIZE + 2 * MEM_FENCE_WIDTH];
static int img3_off;		    /* offset of upper left pixel   */
static unsigned char* img3;	    /* pointer to upper left pixel  */
static int show_x, show_y;          /* logical view coordinates     */

/* displayed video memory variables */
static unsigned char* mem_image;    /* pointer to start of video memory */
static unsigned short target_img;   /* offset of displayed screen image */

extern unsigned char font_data[256][16];

uint8_t write_buf[IMAGE_X_DIM]; /* used to write horizontally/vertically */

/* photo related */
#define OBJ_CLR_TRANSP 0x40	/* transparent pixel color in object image */


/* VGA register settings for mode X */
static unsigned short mode_X_seq[NUM_SEQUENCER_REGS] = {
    0x0100, 0x2101, 0x0F02, 0x0003, 0x0604
};
static unsigned short mode_X_CRTC[NUM_CRTC_REGS] = {
    0x5F00, 0x4F01, 0x5002, 0x8203, 0x5404, 0x8005, 0xBF06, 0x1F07/* line compare reg. bit 8*/,
    0x0008, 0x4109/* line compare reg. high bit bit 9*/, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
    0x9C10, 0x8E11, 0x8F12, 0x2813, 0x0014, 0x9615, 0xB916, 0xE317,
    0xFF18/* line compare reg. bit 0~7*/
};

static unsigned char mode_X_attr[NUM_ATTR_REGS * 2] = {
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 
    0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 
    0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 
    0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F,
    0x10, 0x41, 0x11, 0x00, 0x12, 0x0F, 0x13, 0x00,
    0x14, 0x00, 0x15, 0x00
};
static unsigned short mode_X_graphics[NUM_GRAPHICS_REGS] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x4005, 0x0506, 0x0F07,
    0xFF08
};

/* VGA register settings for text mode 3 (color text) */
static unsigned short text_seq[NUM_SEQUENCER_REGS] = {
    0x0100, 0x2001, 0x0302, 0x0003, 0x0204
};
static unsigned short text_CRTC[NUM_CRTC_REGS] = {
    0x5F00, 0x4F01, 0x5002, 0x8203, 0x5504, 0x8105, 0xBF06, 0x1F07,
    0x0008, 0x4F09, 0x0D0A, 0x0E0B, 0x000C, 0x000D, 0x000E, 0x000F,
    0x9C10, 0x8E11, 0x8F12, 0x2813, 0x1F14, 0x9615, 0xB916, 0xA317,
    0xFF18
};
static unsigned char text_attr[NUM_ATTR_REGS * 2] = {
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 
    0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 
    0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 
    0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F,
    0x10, 0x0C, 0x11, 0x00, 0x12, 0x0F, 0x13, 0x08,
    0x14, 0x00, 0x15, 0x00
};
static unsigned short text_graphics[NUM_GRAPHICS_REGS] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x1005, 0x0E06, 0x0007,
    0xFF08
};




/* 
 * macro used to target a specific video plane or planes when writing
 * to video memory in mode X; bits 8-11 in the mask_hi_bits enable writes
 * to planes 0-3, respectively
 */
#define SET_WRITE_MASK(mask_hi_bits)                                    \
do {                                                                    \
    asm volatile ("                                                     \
	movw $0x03C4,%%dx    	/* set write mask                    */;\
	movb $0x02,%b0                                                 ;\
	outw %w0,(%%dx)                                                 \
    " : : "a" ((mask_hi_bits)) : "edx", "memory");                      \
} while (0)

/* macro used to write a byte to a port */
#define OUTB(port,val)                                                  \
do {                                                                    \
    asm volatile ("                                                     \
        outb %b1,(%w0)                                                  \
    " : /* no outputs */                                                \
      : "d" ((port)), "a" ((val))                                       \
      : "memory", "cc");                                                \
} while (0)

/* macro used to write two bytes to two consecutive ports */
#define OUTW(port,val)                                                  \
do {                                                                    \
    asm volatile ("                                                     \
        outw %w1,(%w0)                                                  \
    " : /* no outputs */                                                \
      : "d" ((port)), "a" ((val))                                       \
      : "memory", "cc");                                                \
} while (0)

/* 
 * macro used to write an array of two-byte values to two consecutive ports 
 */
#define REP_OUTSW(port,source,count)                                    \
do {                                                                    \
    asm volatile ("                                                     \
     1: movw 0(%1),%%ax                                                ;\
	outw %%ax,(%w2)                                                ;\
	addl $2,%1                                                     ;\
	decl %0                                                        ;\
	jne 1b                                                          \
    " : /* no outputs */                                                \
      : "c" ((count)), "S" ((source)), "d" ((port))                     \
      : "eax", "memory", "cc");                                         \
} while (0)

/* 
 * macro used to write an array of one-byte values to two consecutive ports 
 */
#define REP_OUTSB(port,source,count)                                    \
do {                                                                    \
    asm volatile ("                                                     \
     1: movb 0(%1),%%al                                                ;\
	outb %%al,(%w2)                                                ;\
	incl %1                                                        ;\
	decl %0                                                        ;\
	jne 1b                                                          \
    " : /* no outputs */                                                \
      : "c" ((count)), "S" ((source)), "d" ((port))                     \
      : "eax", "memory", "cc");                                         \
} while (0)




/* OCTREE START */
typedef struct photo_header_t photo_header_t;
typedef struct photo_t    photo_t;
typedef struct OctreeNode octnode_t;

struct photo_header_t {
    uint16_t width;	/* image width in pixels  */
    uint16_t height;	/* image height in pixels */
};

struct photo_t {
    photo_header_t hdr;			                              /* defines height and width */
    uint8_t        palette[192][3];                               /* optimized palette colors */
    uint16_t       img[IMAGE_Y_DIM][IMAGE_X_DIM];                 /* pixel data  */
};


struct OctreeNode{
	int col; /* color prefix of that octree node : in level 4 it's 4:4:4 RGB, in level 2 it's 2:2:2 RGB*/
	int num; /* number of pixels in this node */ 
	unsigned long R_tot; /* max{R_tot} <= max{G_tot} <= 4 bytes */
	unsigned long G_tot; /* max{G_tot} = MAX_PHOTO_HEIGHT*MAX_PHOTO_WIDTH*(2^7) = 0x08 00 00 00 <= 4 bytes */
	unsigned long B_tot; /* max{B_tot} <= max{G_tot} <= 4 bytes */
	unsigned long ave; /* average color of that node */
};


#define LEVEL_2_SIZE (64) 
#define LEVEL_4_SIZE (LEVEL_2_SIZE*64) /* each level 2 node has 64 sons */

static octnode_t lev_2[LEVEL_2_SIZE]; /* level 2 nodes in octree */
static octnode_t lev_4[LEVEL_4_SIZE]; /* level 4 nodes in octree */
static int lev_4_rank[LEVEL_4_SIZE]; /* index after sort, shows the rank in terms of number of pixels one node has */
static int top_128_num;

static int mark_lev_4[LEVEL_4_SIZE]; /* mark which nodes of level 4 are chosen */
static uint8_t phot_store[IMAGE_Y_DIM][IMAGE_X_DIM];

/* sort 128 color-prefix in level four */
static void sort_lev_4(void);  
/* mark and get average of 128 color-prefix in level four*/
static void markcol_lev_4(void);
static void	ave_lev_4(void);
/* use pixel information to update octnode_t */
static void update_lev_4(int y,int x,int pixel);
static void update_lev_2(int y,int x,int pixel);
/* get average for colors in level 2 */
static void ave_lev_2();
/* clear octree related arrays*/
static void octree_clear(void);
/* transform 16 bit pixel data to level 4 [R : G : B] [4 : 4 : 4] format */
static int pix16_to_lev4(int pixel);
/* transform 16 bit pixel data to level 2 [R : G : B] [2 : 2 : 2] format */
static int pix16_to_lev2(int pixel);

/* get R,G,B values for palette from 18 bit rgb */
static unsigned long rgb18_getR(unsigned long rgb_18){
	return ((rgb_18>>12)&0x3f);
}
static unsigned long rgb18_getG(unsigned long rgb_18){
	return ((rgb_18>>6)&0x3f);
}
static unsigned long rgb18_getB(unsigned long rgb_18){
	return (rgb_18&0x3f);
}




/**
 * @brief transform 16 bit pixel data to level 4 [R : G : B] [4 : 4 : 4] format
 * >>1 in first term is to shift the first 4 significant bit(SB), <<0 to make it in B pos
 * <<2 in second term is to >>2 so the 4 SB will be 0 based then <<4 to make it in G pos
 * <<7 in third term is to >>1 and then <<8 to make it in R pos
 * &1e is to extract first 4 SB in 5 bits
 * &3c is to extract first 4 SB in 6 bits
 * >>5, >>11 in 2nd, 3rd terms are to make G,R segments in 16 bits aligned to 0 base
 * @param pixel - 16 bits pixel
 * @return ** int - 4:4:4 RGB
 */
static int pix16_to_lev4(int pixel){
	return (((pixel&0x1e)>>1)|(((pixel>>5)&(0x3c))<<2)|(((pixel>>11)&0x1e)<<7));
}

/**
 * @brief transform 16 bit pixel data to level 2 [R : G : B] [2 : 2 : 2] format
 * 0x18 - first 2 Significant bit from 5 bits
 * 0x30 - first 2 Significant bit from 6 bits
 * shifting is the same as level 4, to be precise, pix16_to_lev4() function
 * @param pixel - 16 bit pixel information
 * @return ** int - 2:2:2 RGB
 */
static int pix16_to_lev2(int pixel){
	return (((pixel&0x18)>>3)|(((pixel>>5)&(0x30))>>2)|(((pixel>>11)&0x18)<<1));
}

/**
 * @brief get 5 bits R value from 16 bits pixel
 * @param pixel 
 * @return ** int 
 */
static int pix16_getR(int pixel){
	return ((pixel>>11)&0x1f);
}
/**
 * @brief get 6 bits G value from 16 bits pixel
 * @param pixel 
 * @return ** int 
 */
static int pix16_getG(int pixel){
	return ((pixel>>5)&0x3f);
}
/**
 * @brief get 5 bits B value from 16 bits pixel
 * @param pixel 
 * @return ** int 
 */
static int pix16_getB(int pixel){
	return (pixel&0x1f);
}

/**
 * @brief clear every variable about octree for eah picture
 * 
 * @return ** void 
 */
static void octree_clear(){
	int i;
	for(i=0;i<LEVEL_4_SIZE;i++){
		lev_4[i].num=lev_4[i].R_tot=lev_4[i].G_tot=lev_4[i].B_tot=0;
		lev_4[i].col=i;
		mark_lev_4[i]=0; /* mark each level 4 color as not in level 4 initially */
	}
	for(i=0;i<LEVEL_2_SIZE;i++){
		lev_2[i].num=lev_2[i].R_tot=lev_2[i].G_tot=lev_2[i].B_tot=0;
		lev_2[i].col=i;
	}
	top_128_num=0;

}

/**
 * @brief use pixel information to update octnode_t.
 * 
 * @param y - row index
 * @param x - col index
 * @param pixel - pixel information 16 bits (R[5]:G[6]:B[5])
 * @return ** void 
 */
static void update_lev_4(int y,int x,int pixel){
	int lev_4_col=pix16_to_lev4(pixel);
	if(lev_4_col>=LEVEL_4_SIZE){
		// perror("4:4:4 color out of bound");
	}
	lev_4[lev_4_col].num++;
	lev_4[lev_4_col].R_tot+=pix16_getR(pixel);
	lev_4[lev_4_col].G_tot+=pix16_getG(pixel);
	lev_4[lev_4_col].B_tot+=pix16_getB(pixel);
}

static void update_lev_2(int y,int x,int pixel){
	int lev_2_col=pix16_to_lev2(pixel);
	if(lev_2_col>=LEVEL_2_SIZE){
		// perror("2:2:2 color out of bound");
	}
	lev_2[lev_2_col].num++;
	lev_2[lev_2_col].R_tot+=pix16_getR(pixel);
	lev_2[lev_2_col].G_tot+=pix16_getG(pixel);
	lev_2[lev_2_col].B_tot+=pix16_getB(pixel);
}

/**
 * @brief compare x and y, sort the array in descending order w.r.t. field num
 * 
 * @param x 
 * @param y 
 * @return ** int 
 */
int cmp_lev_4(void* x,void* y){
	return ((octnode_t*)x)->num<((octnode_t*)y)->num;
}

/* bucket sort used data structure */
typedef struct lnode_t lnode_t;
struct lnode_t{
    lnode_t* next;
    octnode_t val;
};
/* each bucket node is a sinly linked-list */
/* lnode_stk stores the sinly linked list nodes */
lnode_t lnode_stk[LEVEL_4_SIZE];
lnode_t* buckets[LEVEL_4_SIZE];
int32_t lnode_r;


void bucket_sort(){
    int32_t i,j;
    lnode_r=0;
    for(i=0;i<LEVEL_4_SIZE;i++)
        buckets[i]=NULL;
    for(i=0;i<LEVEL_4_SIZE;i++){
        lnode_stk[lnode_r].val=lev_4[i];
        lnode_stk[lnode_r].next=buckets[lev_4[i].num];
        buckets[lev_4[i].num]=&lnode_stk[lnode_r++];
    }
    j=0;
    for(i=LEVEL_4_SIZE-1;i>=0;i--){
        lnode_t* pt=buckets[i];
        while(pt){
            lev_4[j++]=pt->val;
            pt=pt->next;
        }
    }

}

/**
 * @brief sort 128 color-prefix in level four
 */
static void sort_lev_4(){
	int i;
	bucket_sort();
	for(i=0;i<LEVEL_4_SIZE;i++){/* assign a rank for each level 4 color */
		lev_4_rank[lev_4[i].col]=i;
		if(lev_4[i].num&&i<128) top_128_num=i+1; /* how many valid colors are in top 128 popular colors */
	}
}
/**
 * @brief mark most popular 128 color-prefix in level 4
 * @return ** void 
 */
static void markcol_lev_4(){
	int i;
	for(i=0;i<top_128_num;i++) mark_lev_4[lev_4[i].col]=1;
}

/**
 * @brief map 5:6:5 color in level 4 to 6:6:6 RGB color in palette
 * 
 * @return ** void 
 */
static void ave_lev_4(){
	int i;
	for(i=0;i<top_128_num;i++) 
		lev_4[i].ave=(((lev_4[i].R_tot/lev_4[i].num)<<13)|
		((lev_4[i].G_tot/lev_4[i].num)<<6)|
		((lev_4[i].B_tot/lev_4[i].num)<<1));
}

/**
 * @brief transform 5:6:5 color in level 2 to 6:6:6 color in palette
 * 
 * @return ** void 
 */
static void ave_lev_2(){
	int i;
	for(i=0;i<64;i++){
		if(lev_2[i].num){
			lev_2[i].ave=(((lev_2[i].R_tot/lev_2[i].num)<<13)|
			((lev_2[i].G_tot/lev_2[i].num)<<6)|
			((lev_2[i].B_tot/lev_2[i].num)<<1));
		}else{/* if this type of color isn't in picture, any color mapping is pixel is ok */
			lev_2[i].ave=0;
		}
	}
}






/* OCTREE END */



/*
 * VGA_blank
 *   DESCRIPTION: Blank or unblank the VGA display.
 *   INPUTS: blank_bit -- set to 1 to blank, 0 to unblank
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */   
static void
VGA_blank (int blank_bit)
{
    /* 
     * Move blanking bit into position for VGA sequencer register 
     * (index 1). 
     */
    blank_bit = ((blank_bit & 1) << 5);

    asm volatile (
	"movb $0x01,%%al         /* Set sequencer index to 1. */       ;"
	"movw $0x03C4,%%dx                                             ;"
	"outb %%al,(%%dx)                                              ;"
	"incw %%dx                                                     ;"
	"inb (%%dx),%%al         /* Read old value.           */       ;"
	"andb $0xDF,%%al         /* Calculate new value.      */       ;"
	"orl %0,%%eax                                                  ;"
	"outb %%al,(%%dx)        /* Write new value.          */       ;"
	"movw $0x03DA,%%dx       /* Enable display (0x20->P[0x3C0]) */ ;"
	"inb (%%dx),%%al         /* Set attr reg state to index. */    ;"
	"movw $0x03C0,%%dx       /* Write index 0x20 to enable. */     ;"
	"movb $0x20,%%al                                               ;"
	"outb %%al,(%%dx)                                               "
      : : "g" (blank_bit) : "eax", "edx", "memory");
}

/*
 * set_seq_regs_and_reset
 *   DESCRIPTION: Set VGA sequencer registers and miscellaneous output
 *                register; array of registers should force a reset of
 *                the VGA sequencer, which is restored to normal operation
 *                after a brief delay.
 *   INPUTS: table -- table of sequencer register values to use
 *           val -- value to which miscellaneous output register should be set
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */   
static void
set_seq_regs_and_reset (unsigned short table[NUM_SEQUENCER_REGS],
			unsigned char val)
{
    /* 
     * Dump table of values to sequencer registers.  Includes forced reset
     * as well as video blanking.
     */
    REP_OUTSW (0x03C4, table, NUM_SEQUENCER_REGS);

    /* Delay a bit... */
    {volatile int ii; for (ii = 0; ii < 10000; ii++);}

    /* Set VGA miscellaneous output register. */
    OUTB (0x03C2, val);

    /* Turn sequencer on (array values above should always force reset). */
    OUTW (0x03C4,0x0300);
}


/*
 * set_CRTC_registers
 *   DESCRIPTION: Set VGA cathode ray tube controller (CRTC) registers.
 *   INPUTS: table -- table of CRTC register values to use
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */   
static void
set_CRTC_registers (unsigned short table[NUM_CRTC_REGS])
{
    /* clear protection bit to enable write access to first few registers */
    OUTW (0x03D4, 0x0011); 
    REP_OUTSW (0x03D4, table, NUM_CRTC_REGS);
}


/*
 * set_attr_registers
 *   DESCRIPTION: Set VGA attribute registers.  Attribute registers use
 *                a single port and are thus written as a sequence of bytes
 *                rather than a sequence of words.
 *   INPUTS: table -- table of attribute register values to use
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */   
static void 
set_attr_registers (unsigned char table[NUM_ATTR_REGS * 2])
{
    /* Reset attribute register to write index next rather than data. */
    asm volatile (
	"inb (%%dx),%%al"
      : : "d" (0x03DA) : "eax", "memory");
    REP_OUTSB (0x03C0, table, NUM_ATTR_REGS * 2);
}

/*
 * set_graphics_registers
 *   DESCRIPTION: Set VGA graphics registers.
 *   INPUTS: table -- table of graphics register values to use
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */   
static void
set_graphics_registers (unsigned short table[NUM_GRAPHICS_REGS])
{
    REP_OUTSW (0x03CE, table, NUM_GRAPHICS_REGS);
}


/*
 * fill_palette_mode_x
 *   DESCRIPTION: Fill VGA palette with necessary colors for the adventure 
 *                game.  Only the first 64 (of 256) colors are written.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes the first 64 palette colors
 */   
static void
fill_palette_mode_x ()
{
    /* 6-bit RGB (red, green, blue) values for first 64 colors */
    /* these are coded for 2 bits red, 2 bits green, 2 bits blue */
    static unsigned char palette_RGB[64][3] = {
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x15},
	{0x00, 0x00, 0x2A}, {0x00, 0x00, 0x3F},
	{0x00, 0x15, 0x00}, {0x00, 0x15, 0x15},
	{0x00, 0x15, 0x2A}, {0x00, 0x15, 0x3F},
	{0x00, 0x2A, 0x00}, {0x00, 0x2A, 0x15},
	{0x00, 0x2A, 0x2A}, {0x00, 0x2A, 0x3F},
	{0x00, 0x3F, 0x00}, {0x00, 0x3F, 0x15},
	{0x00, 0x3F, 0x2A}, {0x00, 0x3F, 0x3F},
	{0x15, 0x00, 0x00}, {0x15, 0x00, 0x15},
	{0x15, 0x00, 0x2A}, {0x15, 0x00, 0x3F},
	{0x15, 0x15, 0x00}, {0x15, 0x15, 0x15},
	{0x15, 0x15, 0x2A}, {0x15, 0x15, 0x3F},
	{0x15, 0x2A, 0x00}, {0x15, 0x2A, 0x15},
	{0x15, 0x2A, 0x2A}, {0x15, 0x2A, 0x3F},
	{0x15, 0x3F, 0x00}, {0x15, 0x3F, 0x15},
	{0x15, 0x3F, 0x2A}, {0x15, 0x3F, 0x3F},
	{0x2A, 0x00, 0x00}, {0x2A, 0x00, 0x15},
	{0x2A, 0x00, 0x2A}, {0x2A, 0x00, 0x3F},
	{0x2A, 0x15, 0x00}, {0x2A, 0x15, 0x15},
	{0x2A, 0x15, 0x2A}, {0x2A, 0x15, 0x3F},
	{0x2A, 0x2A, 0x00}, {0x2A, 0x2A, 0x15},
	{0x2A, 0x2A, 0x2A}, {0x2A, 0x2A, 0x3F},
	{0x2A, 0x3F, 0x00}, {0x2A, 0x3F, 0x15},
	{0x2A, 0x3F, 0x2A}, {0x2A, 0x3F, 0x3F},
	{0x3F, 0x00, 0x00}, {0x3F, 0x00, 0x15},
	{0x3F, 0x00, 0x2A}, {0x3F, 0x00, 0x3F},
	{0x3F, 0x15, 0x00}, {0x3F, 0x15, 0x15},
	{0x3F, 0x15, 0x2A}, {0x3F, 0x15, 0x3F},
	{0x3F, 0x2A, 0x00}, {0x3F, 0x2A, 0x15},
	{0x3F, 0x2A, 0x2A}, {0x3F, 0x2A, 0x3F},
	{0x3F, 0x3F, 0x00}, {0x3F, 0x3F, 0x15},
	{0x3F, 0x3F, 0x2A}, {0x3F, 0x3F, 0x3F}
    };

    /* Start writing at color 0. */
    OUTB (0x03C8, 0x00);

    /* Write all 64 colors from array. */
    REP_OUTSB (0x03C9, palette_RGB, 64 * 3);
}

/*
 * fill_palette_text
 *   DESCRIPTION: Fill VGA palette with default VGA colors.
 *                Only the first 32 (of 256) colors are written.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes the first 32 palette colors
 */   
static void
fill_palette_text ()
{
    /* 6-bit RGB (red, green, blue) values VGA colors and grey scale */
    static unsigned char palette_RGB[32][3] = {
	{0x00, 0x00, 0x00}, {0x00, 0x00, 0x2A},   /* palette 0x00 - 0x0F    */
	{0x00, 0x2A, 0x00}, {0x00, 0x2A, 0x2A},   /* basic VGA colors       */
	{0x2A, 0x00, 0x00}, {0x2A, 0x00, 0x2A},
	{0x2A, 0x15, 0x00}, {0x2A, 0x2A, 0x2A},
	{0x15, 0x15, 0x15}, {0x15, 0x15, 0x3F},
	{0x15, 0x3F, 0x15}, {0x15, 0x3F, 0x3F},
	{0x3F, 0x15, 0x15}, {0x3F, 0x15, 0x3F},
	{0x3F, 0x3F, 0x15}, {0x3F, 0x3F, 0x3F},
	{0x00, 0x00, 0x00}, {0x05, 0x05, 0x05},   /* palette 0x10 - 0x1F    */
	{0x08, 0x08, 0x08}, {0x0B, 0x0B, 0x0B},   /* VGA grey scale         */
	{0x0E, 0x0E, 0x0E}, {0x11, 0x11, 0x11},
	{0x14, 0x14, 0x14}, {0x18, 0x18, 0x18},
	{0x1C, 0x1C, 0x1C}, {0x20, 0x20, 0x20},
	{0x24, 0x24, 0x24}, {0x28, 0x28, 0x28},
	{0x2D, 0x2D, 0x2D}, {0x32, 0x32, 0x32},
	{0x38, 0x38, 0x38}, {0x3F, 0x3F, 0x3F}
    };

    /* Start writing at color 0. */
    OUTB (0x03C8, 0x00);

    /* Write all 32 colors from array. */
    REP_OUTSB (0x03C9, palette_RGB, 32 * 3);
}


/*
 * write_font_data
 *   DESCRIPTION: Copy font data into VGA memory, changing and restoring
 *                VGA register values in order to do so. 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: leaves VGA registers in final text mode state
 */   
static void
write_font_data ()
{
    int i;                /* loop index over characters                   */
    int j;                /* loop index over font bytes within characters */
    unsigned char* fonts; /* pointer into video memory                    */

    /* Prepare VGA to write font data into video memory. */
    OUTW (0x3C4, 0x0402);
    OUTW (0x3C4, 0x0704);
    OUTW (0x3CE, 0x0005);
    OUTW (0x3CE, 0x0406);
    OUTW (0x3CE, 0x0204);

    /* Copy font data from array into video memory. */
    for (i = 0, fonts = mem_image; i < 256; i++) {
	for (j = 0; j < 16; j++)
	    fonts[j] = font_data[i][j];
	    fonts += 32; /* skip 16 bytes between characters */
    }

    /* Prepare VGA for text mode. */
    OUTW (0x3C4, 0x0302);
    OUTW (0x3C4, 0x0304);
    OUTW (0x3CE, 0x1005);
    OUTW (0x3CE, 0x0E06);
    OUTW (0x3CE, 0x0004);
}

/*
 * set_text_mode_3
 *   DESCRIPTION: Put VGA into text mode 3 (color text).
 *   INPUTS: clear_scr -- if non-zero, clear screens; otherwise, do not
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: may clear screens; writes font data to video memory
 */   
static void
set_text_mode_3 (int clear_scr)
{
    unsigned long* txt_scr; /* pointer to text screens in video memory */
    int i;                  /* loop over text screen words             */

    VGA_blank (1);                               /* blank the screen        */
    /* 
     * The value here had been changed to 0x63, but seems to work
     * fine in QEMU (and VirtualPC, where I got it) with the 0x04
     * bit set (VGA_MIS_DCLK_28322_720).  
     */
    set_seq_regs_and_reset (text_seq, 0x67);     /* sequencer registers     */
    set_CRTC_registers (text_CRTC);              /* CRT control registers   */
    set_attr_registers (text_attr);              /* attribute registers     */
    set_graphics_registers (text_graphics);      /* graphics registers      */
    fill_palette_text ();			 /* palette colors          */
    if (clear_scr) {				 /* clear screens if needed */
	txt_scr = (unsigned long*)(mem_image + 0x18000); 
	for (i = 0; i < 8192; i++)
	    *txt_scr++ = 0x07200720;
    }
    write_font_data ();                          /* copy fonts to video mem */
    VGA_blank (0);			         /* unblank the screen      */
}




/*
 * copy_image
 *   DESCRIPTION: Copy one plane of a screen from the build buffer to the 
 *                video memory.
 *   INPUTS: img -- a pointer to a single screen plane in the build buffer
 *           scr_addr -- the destination offset in video memory
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: copies a plane from the build buffer to video memory
 */   
static void
copy_image (unsigned char* img, unsigned short scr_addr)
{
    /* 
     * memcpy is actually probably good enough here, and is usually
     * implemented using ISA-specific features like those below,
     * but the code here provides an example of x86 string moves
     */
    asm volatile (
        "cld                                                 ;"
       	"movl $16000,%%ecx                                   ;"
       	"rep movsb    # copy ECX bytes from M[ESI] to M[EDI]  "
      : /* no outputs */
      : "S" (img), "D" (mem_image + scr_addr) 
      : "eax", "ecx", "memory"
    );
}

/*
 * show_screen
 *   DESCRIPTION: Show the logical view window on the video display.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: copies from the build buffer to video memory;
 *                 shifts the VGA display source to point to the new image
 */   
void
show_screen ()
{
    unsigned char* addr;  /* source address for copy             */
    int p_off;            /* plane offset of first display plane */
    int i;		  /* loop index over video planes        */

    /* 
     * Calculate offset of build buffer plane to be mapped into plane 0 
     * of display.
     */
    p_off = (3 - (show_x & 3));

    /* Switch to the other target screen in video memory. */
    target_img ^= 0x4000;

    /* Calculate the source address. */
    addr = img3 + (show_x >> 2) + show_y * SCROLL_X_WIDTH;

    /* Draw to each plane in the video memory. */
    for (i = 0; i < 4; i++) {
	SET_WRITE_MASK (1 << (i + 8));
	copy_image (addr + ((p_off - i + 4) & 3) * SCROLL_SIZE + (p_off < i), 
	            target_img);
    }

    /* 
     * Change the VGA registers to point the top left of the screen
     * to the video memory that we just filled.
     */
    OUTW (0x03D4, (target_img & 0xFF00) | 0x0C);
    OUTW (0x03D4, ((target_img & 0x00FF) << 8) | 0x0D);
}

/*
 * draw_vert_line
 *   DESCRIPTION: Draw a vertical map line into the build buffer.  The 
 *                line should be offset from the left side of the logical
 *                view window screen by the given number of pixels.  
 *   INPUTS: x -- the 0-based pixel column number of the line to be drawn
 *                within the logical view window (equivalent to the number
 *                of pixels from the leftmost pixel to the line to be
 *                drawn)
 *           buf -- buffer for graphical image of line 
 *   OUTPUTS: none
 *   RETURN VALUE: Returns 0 on success.  If x is outside of the valid 
 *                 SCROLL range, the function returns -1.  
 *   SIDE EFFECTS: draws into the build buffer
 */   
int
draw_vert_line (int x,unsigned char buf[SCROLL_Y_DIM]){
    unsigned char* addr;             /* address of first pixel in build    */
   				     /*     buffer (without plane offset)  */
    int p_off;                       /* offset of plane of first pixel     */
    int i;			     /* loop index over pixels             */

    /* Check whether requested line falls in the logical view window. */
    if (x < 0 || x >= SCROLL_X_DIM) return -1;

    /* Adjust y to the logical row value. */
    x += show_x;



    /* Calculate starting address in build buffer. */
    addr = img3 + (x >> 2) + show_y * SCROLL_X_WIDTH;

    /* Calculate plane offset of first pixel. */
    p_off = (3 - (x & 3));

    /* Copy image data into appropriate planes in build buffer. */
    for (i = 0; i < SCROLL_Y_DIM; i++) {
        addr[p_off * SCROLL_SIZE] = buf[i];
        addr+=SCROLL_X_WIDTH;
    }

    /* Return success. */
    return 0;
}


/*
 * draw_horiz_line
 *   DESCRIPTION: Draw a horizontal map line into the build buffer.  The 
 *                line should be offset from the top of the logical view 
 *                window screen by the given number of pixels.  
 *   INPUTS: y -- the 0-based pixel row number of the line to be drawn
 *                within the logical view window (equivalent to the number
 *                of pixels from the top pixel to the line to be drawn)
 *          buf -- buffer for graphical image of line 
 *   OUTPUTS: none
 *   RETURN VALUE: Returns 0 on success.  If y is outside of the valid 
 *                 SCROLL range, the function returns -1.  
 *   SIDE EFFECTS: draws into the build buffer
 */   
int
draw_horiz_line (int y, unsigned char buf[SCROLL_X_DIM]){
    unsigned char* addr;             /* address of first pixel in build    */
   				     /*     buffer (without plane offset)  */
    int p_off;                       /* offset of plane of first pixel     */
    int i;			     /* loop index over pixels             */

    /* Check whether requested line falls in the logical view window. */
    if (y < 0 || y >= SCROLL_Y_DIM)
	return -1;

    /* Adjust y to the logical row value. */
    y += show_y;

    /* Calculate starting address in build buffer. */
    addr = img3 + (show_x >> 2) + y * SCROLL_X_WIDTH;

    /* Calculate plane offset of first pixel. */
    p_off = (3 - (show_x & 3));

    /* Copy image data into appropriate planes in build buffer. */
    for (i = 0; i < SCROLL_X_DIM; i++) {
        addr[p_off * SCROLL_SIZE] = buf[i];
        if (--p_off < 0) {
            p_off = 3;
            addr++;
        }
    }

    /* Return success. */
    return 0;
}

/*
 * clear_screens
 *   DESCRIPTION: Fills the video memory with zeroes. 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: fills all 256kB of VGA video memory with zeroes
 */   
void 
clear_screens ()
{
    /* Write to all four planes at once. */ 
    SET_WRITE_MASK (0x0F00);

    /* Set 64kB to zero (times four planes = 256kB). */
    memset (mem_image, 0, MODE_X_MEM_SIZE);
}

// Abandon for current version : no support for shifting image : v0.1
// /*
//  * set_view_window
//  *   DESCRIPTION: Set the logical view window, moving its location within
//  *                the build buffer if necessary to keep all on-screen data
//  *                in the build buffer.  If the location within the build
//  *                buffer moves, this function copies all data from the old
//  *                window that are within the new screen to the appropriate
//  *                new location, so only data not previously on the screen 
//  *                must be drawn before calling show_screen.
//  *   INPUTS: (scr_x,scr_y) -- new upper left pixel of logical view window
//  *   OUTPUTS: none
//  *   RETURN VALUE: none
//  *   SIDE EFFECTS: may shift position of logical view window within build 
//  *                 buffer
//  */   
// void
// set_view_window (int scr_x, int scr_y)
// {
//     int old_x, old_y;     /* old position of logical view window           */
//     int start_x, start_y; /* starting position for copying from old to new */ 
//     int end_x, end_y;     /* ending position for copying from old to new   */ 
//     int start_off;        /* offset of copy start relative to old build    */
//      		          /*    buffer start position                      */
//     int length;           /* amount of data to be copied                   */
//     int i;	          /* copy loop index                               */
//     unsigned char* start_addr;  /* starting memory address of copy     */
//     unsigned char* target_addr; /* destination memory address for copy */

//     /* Record the old position. */
//     old_x = show_x;
//     old_y = show_y;

//     /* Keep track of the new view window. */
//     show_x = scr_x;
//     show_y = scr_y;

//     /*
//      * If the new view window fits within the boundaries of the build 
//      * buffer, we need move nothing around.
//     */
//     if (img3_off + (scr_x >> 2) + scr_y * SCROLL_X_WIDTH >= 0 &&
//         img3_off + 3 * SCROLL_SIZE +
// 	    ((scr_x + SCROLL_X_DIM - 1) >> 2) + 
// 	    (scr_y + SCROLL_Y_DIM - 1) * SCROLL_X_WIDTH < BUILD_BUF_SIZE)
// 	return;

//     /*
//      * If the new screen does not overlap at all with the old screen, none
//      * of the old data need to be saved, and we can simply reposition the
//      * valid window of the build buffer in the middle of that buffer.
//      */
//     if (scr_x <= old_x - SCROLL_X_DIM || scr_x >= old_x + SCROLL_X_DIM ||
// 	scr_y <= old_y - SCROLL_Y_DIM || scr_y >= old_y + SCROLL_Y_DIM) {
// 	img3_off = BUILD_BASE_INIT - (scr_x >> 2) - scr_y * SCROLL_X_WIDTH;
// 	img3 = build + img3_off + MEM_FENCE_WIDTH;
// 	return;
//     }

//     /*
//      * Any still-visible portion of the old screen should be retained.
//      * Rather than clipping exactly, we copy all contiguous data between
//      * a clipped starting point to a clipped ending point (which may
//      * include non-visible data).
//      *
//      * The starting point is the maximum (x,y) coordinates between the
//      * new and old screens.  The ending point is the minimum (x,y)
//      * coordinates between the old and new screens (offset by the screen
//      * size).
//      */
//     if (scr_x > old_x) {
//         start_x = scr_x;
// 	end_x = old_x;
//     } else {
//         start_x = old_x;
// 	end_x = scr_x;
//     }
//     end_x += SCROLL_X_DIM - 1;
//     if (scr_y > old_y) {
//         start_y = scr_y;
// 	end_y = old_y;
//     } else {
//         start_y = old_y;
// 	end_y = scr_y;
//     }
//     end_y += SCROLL_Y_DIM - 1;

//     /* 
//      * We now calculate the starting and ending addresses for the copy
//      * as well as the new offsets for use with the build buffer.  The
//      * length to be copied is basically the ending offset minus the starting
//      * offset plus one (plus the three screens in between planes 3 and 0).
//      */
//     start_off = (start_x >> 2) + start_y * SCROLL_X_WIDTH;
//     start_addr = img3 + start_off;
//     length = (end_x >> 2) + end_y * SCROLL_X_WIDTH + 1 - start_off + 
// 	     3 * SCROLL_SIZE;
//     img3_off = BUILD_BASE_INIT - (show_x >> 2) - show_y * SCROLL_X_WIDTH;
//     img3 = build + img3_off + MEM_FENCE_WIDTH;
//     target_addr = img3 + start_off;

//     /* 
//      * Copy the relevant portion of the screen from the old location to the
//      * new one.  The areas may overlap, so copy direction is important. 
//      * (You should be able to explain why!)
//      */
//     if (start_addr < target_addr)
// 	for (i = length; i-- > 0; )
// 	    target_addr[i] = start_addr[i];
//     else
// 	for (i = 0; i < length; i++)
// 	    target_addr[i] = start_addr[i];
// }




/*
 * set_mode_X
 *   DESCRIPTION: Puts the VGA into mode X.
 *   INPUTS: horiz_fill_fn -- this function is used as a callback (by
 *   			      draw_horiz_line) to obtain a graphical 
 *   			      image of a particular logical line for 
 *   			      drawing to the build buffer
 *           vert_fill_fn -- this function is used as a callback (by
 *   			     draw_vert_line) to obtain a graphical 
 *   			     image of a particular logical line for 
 *   			     drawing to the build buffer
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: 
 *                  * initializes the logical view window; maps video memory
 *                 and obtains permission for VGA ports; clears video memory
 *                 
 *                  * Mapped the video memory will always be the same, so use this
 *                    function in displayed terminal only (no background process 
 *                    should call this function)
 */   
int
set_mode_X (){
    int i; /* loop index for filling memory fence with magic numbers */

    /* 
     * Record callback functions for obtaining horizontal and vertical 
     * line images.
     */
    // if (horiz_fill_fn == NULL || vert_fill_fn == NULL)
    //     return -1;
    // horiz_line_fn = horiz_fill_fn;
    // vert_line_fn = vert_fill_fn;

    /* Initialize the logical view window to position (0,0). */
    show_x = show_y = 0;
    img3_off = BUILD_BASE_INIT;
    img3 = build + img3_off + MEM_FENCE_WIDTH;

    /* Set up the memory fence on the build buffer. */
    for (i = 0; i < MEM_FENCE_WIDTH; i++) {
        build[i] = MEM_FENCE_MAGIC;
        build[BUILD_BUF_SIZE + MEM_FENCE_WIDTH + i] = MEM_FENCE_MAGIC;
    }

    /* One display page goes at the start of video memory. */
    target_img = 0x0; 

    mem_image=(uint8_t*)VGA_START;
    
    // /* Map video memory and obtain permission for VGA port access. */
    // if (open_memory_and_ports () == -1)
    //     return -1;

    /* 
     * The code below was produced by recording a call to set mode 0013h
     * with display memory clearing and a windowed frame buffer, then
     * modifying the code to set mode X instead.  The code was then
     * generalized into functions...
     *
     * modifications from mode 13h to mode X include...
     *   Sequencer Memory Mode Register: 0x0E to 0x06 (0x3C4/0x04)
     *   Underline Location Register   : 0x40 to 0x00 (0x3D4/0x14)
     *   CRTC Mode Control Register    : 0xA3 to 0xE3 (0x3D4/0x17)
     */

    VGA_blank (1);                               /* blank the screen      */
    set_seq_regs_and_reset (mode_X_seq, 0x63);   /* sequencer registers   */
    set_CRTC_registers (mode_X_CRTC);            /* CRT control registers */
    set_attr_registers (mode_X_attr);            /* attribute registers   */
    set_graphics_registers (mode_X_graphics);    /* graphics registers    */
    fill_palette_mode_x ();			 /* palette colors        */
    clear_screens ();				 /* zero video memory     */
    VGA_blank (0);			         /* unblank the screen    */

    /* Return success. */
    return 0;
}

void palette_out(unsigned char palette[192][3]){
    /* modify the palette setting */

	 /* Start writing at color 64. */
    OUTB (0x03C8, 64);

    /* Write all 192 colors from array. */
    REP_OUTSB (0x03C9, palette , 192 * 3);
}

/**
 * @brief write to VGA
 * @param file - dumped
 * @param buf  - uint16* array, consisting of RGB in format of 5:6:5
 * @param bytes - number of bytes in this array
 * @return ** int32_t - 0 on success : -1 on failure
 */
int32_t vga_write(file_t* file, const void* buf, int32_t bytes){
    photo_t* p=(photo_t*)buf;
    int32_t len=bytes>>1;
    int32_t i,x,y;
    uint16_t pixel;
    if(len!=p->hdr.height*p->hdr.width){
        return -1;
    }
    octree_clear();
    for (y = p->hdr.height; y-- > 0; ) {

		/* Loop over columns from left to right. */
		for (x = 0; p->hdr.width > x; x++) {
            pixel=p->img[y][x];
			/* 
			* 16-bit pixel is coded as 5:6:5 RGB (5 bits red, 6 bits green,
			* and 6 bits blue).  We change to 2:2:2, which we've set for the
			* game objects.  You need to use the other 192 palette colors
			* to specialize the appearance of each photo.
			*
			* In this code, you need to calculate the p->palette values,
			* which encode 6-bit RGB as arrays of three uint8_t's.  When
			* the game puts up a photo, you should then change the palette 
			* to match the colors needed for that photo.
			*/
			// phot_store[y][x] = (((pixel >> 14) << 4) |
			// 				(((pixel >> 9) & 0x3) << 2) |
			// 				((pixel >> 3) & 0x3)); /* code for mapping 2:2:2 RGB */
            // phot_store[y][x]=pixel;
			update_lev_4(y,x,pixel);
		}
    }

    /* sort 128 color-prefix in level four */
	sort_lev_4(); 
	/* after this function, the array is indexed by rank */
	/* either use a loop or use lev_4_rank array to access lev_4 array items */
	/* mark and get average of 128 color-prefix in level four*/
	markcol_lev_4();
	ave_lev_4();
	


	/* loop over each pixel again and selecting those who are not marked in level 4 */
	for (y = IMAGE_Y_DIM; y-- > 0; ){
		for (x = 0; IMAGE_X_DIM > x; x++) {
			if(mark_lev_4[pix16_to_lev4(p->img[y][x])]) continue;
			update_lev_2(y,x,p->img[y][x]);
		}
	}

	/* get average for colors in level 2 */
	ave_lev_2();
	
	/* optimization suggestion, compare that to the basic colors */

	/* map level 4 to palette */
	for(i=0;i<128;i++){
		p->palette[i+64][0]=rgb18_getR(lev_4[i].ave);
		p->palette[i+64][1]=rgb18_getG(lev_4[i].ave);
		p->palette[i+64][2]=rgb18_getB(lev_4[i].ave);
	}
	/* map level 2 to palette */
	for(i=0;i<64;i++){
		p->palette[i][0]=rgb18_getR(lev_2[i].ave);
		p->palette[i][1]=rgb18_getG(lev_2[i].ave);
		p->palette[i][2]=rgb18_getB(lev_2[i].ave);
	}
	/* assign each pixel with palette index */
	for (y = p->hdr.height; y-- > 0; ){
		for (x = 0; p->hdr.width > x; x++) {
			int lev_4_ind=lev_4_rank[pix16_to_lev4(p->img[y][x])];
			int lev_2_ind=pix16_to_lev2(p->img[y][x]);
			phot_store[y][x]=lev_4_ind<128?lev_4_ind+128:lev_2_ind+64;
		}
	}
    
    palette_out(p->palette);

    for (y = p->hdr.height; y-- > 0; ){
		draw_horiz_line(y,(uint8_t*)phot_store[y]);
    }

    show_screen();
    return 0;
}

/**
 * @brief dumped
 */
int32_t vga_read(file_t* file,void* buf, int32_t nbytes){
    return -1;
}

/**
 * @brief set text mode back, close the vga 
 * @param file - file to close 
 * @return ** int32_t 
 */
int32_t vga_close(file_t* file){
    file->flags=F_CLOSE;
    file->pos=0;
    set_text_mode_3(0);
    set_graphics(0);
    clear();
    return 0;
}

/**
 * @brief open VGA set graphics mode X
 * @param file  - file descriptor to set
 * @param fname - dump
 * @param dump  - dump
 * @return ** int32_t - 0 on success other on failure
 */
int32_t vga_open(file_t* file, const uint8_t* fname, int32_t dump){
    file->flags=F_OPEN;
    file->fops.write=vga_write;
    file->fops.read=vga_read;
    file->fops.close=vga_close;
    file->pos=0;
    file->inode=-1;
    set_graphics(1);
    return set_mode_X();
}

/**
 * @brief testing function for the whole VGA 
 * 
 * @return ** void 
 */
void VGA_test(){
    set_mode_X();
    set_text_mode_3(0);
    clear();
}



