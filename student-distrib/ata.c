#include "ata.h"
#include "lib.h"
#include "filesystem.h"
#define SECTOR_COUNT 0x1F2
#define LBAlo        0x1F3
#define LBAmid       0x1F4
#define LBAhi        0x1F5
#define DRIVE_SELECT 0x1F6
#define COMMAND_IO   0x1F7
#define STATUS_PORT  0x1F7

#define IO_BASE      0x1F0
#define CTRL_BASE    0x3F6

#define CMD_IDENTIFY    0xEC
#define MASTER_IN       0xE0
#define CACHE_FLUSH     0xE7

#define CMD_READ_SECTOR  0x20
#define CMD_WRITE_SECTOR 0x30

#define FS_LBA_BASE      0x2138
#define FS_LBA_MAX       0x2578

#define SECTOR_SIZE      512
#define HPC              16
#define SPT              63


#define STATUS_BSY 0x80
#define STATUS_RDY 0x40

static void ATA_wait_BSY()   //Wait for bsy to be 0
{
	while(inb(0x1F7)&STATUS_BSY);
}
static void ATA_wait_DRQ()  //Wait fot drq to be 1
{
	while(!(inb(0x1F7)&STATUS_RDY));
}


void ata_soft_reset(){
    outb(0x04,0x3F6);
    outb(0x00,0x3F6);
}

/* This, from OSdev, somehow works */
int32_t 
detect_devtype (int32_t slavebit){
	ata_soft_reset();		/* waits until master drive is ready again */
	outb(0xA0 | slavebit<<4,DRIVE_SELECT);
	inb(CTRL_BASE);			/* wait 400ns for drive select to work */
	inb(CTRL_BASE);	
    inb(CTRL_BASE);	
    inb(CTRL_BASE);	
	unsigned cl=inb(IO_BASE+4);	/* get the "signature bytes" */
	unsigned ch=inb(IO_BASE+5);

	if (cl==0 && ch == 0) return 1;
	return 0;
}

/* This, interpreted from OSdev, somehow doesn't work */
// int32_t 
// identify_command(){
//     ATA_wait_BSY();
//     ATA_wait_DRQ();
//     outb(0xA0,DRIVE_SELECT);
//     outb(0,SECTOR_COUNT);
//     outb(0,LBAlo);
//     outb(0,LBAhi);
//     outb(COMMAND_IO,DRIVE_SELECT);
//     while(0==inb(STATUS_PORT));
//     printf("!\n");
//     while(1);
// }

/* This, from OSdev, works */
void read_sectors_ATA_PIO(uint32_t target_address, uint32_t LBA, uint8_t sector_count)
{
    int32_t i,j;
	ATA_wait_BSY();
	outb(MASTER_IN | ((LBA >>24) & 0xF),DRIVE_SELECT);
	outb(sector_count,SECTOR_COUNT);
	outb((uint8_t) LBA,LBAlo);
	outb((uint8_t)(LBA >> 8),LBAmid);
	outb((uint8_t)(LBA >> 16),LBAhi); 
	outb(CMD_READ_SECTOR,STATUS_PORT); //Send the read command

	uint16_t *target = (uint16_t*) target_address;

	for(j=0;j<sector_count;j++){
		ATA_wait_BSY();
		ATA_wait_DRQ();
		for(i=0;i<256;i++)
			target[i] = inw(IO_BASE);
		target+=256;
	}
}

void write_sectors_ATA_PIO(uint32_t LBA, uint8_t sector_count, uint32_t target_address)
{
	int32_t i,j;
	ATA_wait_BSY();
	outb(MASTER_IN | ((LBA >>24) & 0xF),0x1F6);
	outb(sector_count,SECTOR_COUNT);
	outb((uint8_t) LBA,LBAlo );
	outb((uint8_t)(LBA >> 8),LBAmid);
	outb((uint8_t)(LBA >> 16),LBAhi); 
	outb(CMD_WRITE_SECTOR,COMMAND_IO); //Send the write command

	uint16_t* target=(uint16_t*)target_address;
	for (j=0;j<sector_count;j++){
		ATA_wait_BSY();
		ATA_wait_DRQ();
		for(i=0;i<256;i++){
			outw(target[i],IO_BASE);
		}
	}
	outb(CACHE_FLUSH,COMMAND_IO);
	ATA_wait_BSY();
}

/* This is testing program, from 
* http://learnitonweb.com/2020/05/22/12-developing-an-operating-system-tutorial-episode-6-ata-pio-driver-osdev/
* Really concise tutorial that helps you set up ATA in 10 minutes ( although it takes me 10 hours )
*/
void test_read_write(){
    int32_t i;
    uint16_t buf[2048];
    printf("reading...\r\n");
	/* calculation based on wiki https://en.wikipedia.org/wiki/Logical_block_addressing */
    printf("st: C=%d H=%d S-1=%d offset=%d LBA=%x ed: C=%d H=%d S-1=%d offset=%d LBA=%x\n",
	fs.sys_st_addr/SECTOR_SIZE/SPT/HPC,fs.sys_st_addr/SECTOR_SIZE/SPT%HPC,fs.sys_st_addr/SECTOR_SIZE%SPT,fs.sys_st_addr%SECTOR_SIZE,
	((fs.sys_st_addr/SECTOR_SIZE/SPT/HPC)*HPC+fs.sys_st_addr/SECTOR_SIZE/SPT%HPC)*SPT+fs.sys_st_addr/SECTOR_SIZE%SPT,
	fs.sys_ed_addr/SECTOR_SIZE/SPT/HPC,fs.sys_ed_addr/SECTOR_SIZE/SPT%HPC,fs.sys_ed_addr/SECTOR_SIZE%SPT,fs.sys_ed_addr%SECTOR_SIZE,
	((fs.sys_ed_addr/SECTOR_SIZE/SPT/HPC)*HPC+fs.sys_ed_addr/SECTOR_SIZE/SPT%HPC)*SPT+fs.sys_ed_addr/SECTOR_SIZE%SPT
	);
	/* ONLY ADAPT TO OUR FILESYSTEM : !STATIC! */
	printf("size = %d\n",fs.sys_ed_addr-fs.sys_st_addr);

	/* file system start at LBA = 0x2138 end at LBA = 0x2577 size = 557,056 B */
    read_sectors_ATA_PIO((uint32_t)buf, FS_LBA_BASE, 1);
    
    for(i=0;i<256;i++){
		if(i==2) break;
        printf("%x ", buf[i] & 0xFF);
        printf("%x ", (buf[i] >> 8) & 0xFF);
    }
	printf("\n");
	uint8_t* fs_pt=(uint8_t*)fs.sys_st_addr;
	printf("%u %u %u %u\n",*fs_pt,*(fs_pt+1),*(fs_pt+2),*(fs_pt+3));

	// for(i=0;i<256;i++){
	// 	buf[i]=0xf;
	// }

	// write_sectors_ATA_PIO(0x0+512,1,buf);

	// printf("check write\n");
	// for(i=0;i<256;i++){
    //     printf("%x ", buf[i] & 0xFF);
    //     printf("%x ", (buf[i] >> 8) & 0xFF);
    // }

    while(1);
}

void read_fs(){
	uint32_t lba;
	int32_t i,j;
	uint8_t  buf[1024];
	for(lba=FS_LBA_BASE;lba<FS_LBA_MAX;lba++){
		read_sectors_ATA_PIO(buf,lba,1);
		j=0;
		if(lba==FS_LBA_BASE&&buf[0]==0){
			return ;
		}
		for(i=((uint32_t)fs.sys_st_addr)+(lba-FS_LBA_BASE)*SECTOR_SIZE;
		i<((uint32_t)fs.sys_st_addr)+(lba-FS_LBA_BASE+1)*SECTOR_SIZE;i++){
			*((uint8_t*)i)=buf[j++];
		}
	}
}

/**
 * @brief dump the file system into hard drive
 * @return ** void 
 */
void dump_fs(){
	uint32_t lba;
	for(lba=FS_LBA_BASE;lba<FS_LBA_MAX;lba++){
		write_sectors_ATA_PIO(lba,1,((uint32_t)fs.sys_st_addr)+(lba-FS_LBA_BASE)*SECTOR_SIZE);
	}
}

