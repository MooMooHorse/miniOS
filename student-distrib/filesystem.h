#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H
#include "types.h"

/* flags for file descriptor table */
/* lower 2 bits will be used to identify file type : directory(0), rtc(1), file(2), terminal (3) */
#define DESCRIPTOR_ENTRY_RTC      0
#define DESCRIPTOR_ENTRY_DIR      1 
#define DESCRIPTOR_ENTRY_FILE     2
#define DESCRIPTOR_ENTRY_TERMINAL 3


/* directory entry, 64 Bytes*/
typedef struct 
dentry{
    uint8_t filename[33];
    uint32_t filetype;
    uint32_t inode_num;
    uint32_t reserved[6];
}dentry_t;

struct file_descriptor_item;

/**
 * @brief Each file operations are driver-specific, it doesn't know the file descriptor index.
 * It just fills related fields that are passed into it or are originally into it,
 * and perform the task to (virtualized) devices.
 * This is ioctl between kernel and module.
 */
typedef struct file_operation_table{
    /* First arg : file descriptor table item, 
    * you need to FILL IT, some items can be discarded 
    * by filling garbage . 
    * second arg : open file by filename
    * third arg  : open file by index(inode)
    * You can choose to support either or both.
    * Return Val : Inode number (in RTC, return RTC index(which RTC(virtualized) 
    * you are opening), In terminal, STDIN or STDOUT.) -1 on failure
    */
    int32_t (*open)(struct file_descriptor_item*, const uint8_t*,int32_t);

    /* First arg  : file descriptor info : inode number (In RTC, RTC index. In termminal, garbage)
    *  Second arg : buffer <- your read result (In RTC, garbage. )
    *  Third arg  : n bytes to read (In RTC, how many rounds it wants you to wait)
    *  Return Val : Number of bytes you read (wait for RTC) -1 on failure
    */
    int32_t (*read)(struct file_descriptor_item*,void*,int32_t);
    
    /* First arg  : file descriptor info : inode number (In RTC, which RTC do you want to read. In termminal, garbage)
     * Second arg : buffer <- your write source (In RTC, garbage. )
     * Third arg  : n bytes to read (In RTC, freqency you want to set)
     * Return Val : n bytes you wrote -1 on failure
     */
    int32_t (*write)(struct file_descriptor_item*,void*,int32_t);

    /* First arg  : file descriptor table item, you need to CLEAN IT, some items can be discarded 
    *  by not cleaning it.
    */
    int32_t (*close)(struct file_descriptor_item*);
} fops_t;

typedef struct file_descriptor_item{
    fops_t file_operation_jump_table;
    uint32_t inode;
    uint32_t file_position;
    uint32_t flags;
} fd_t;




/**
 * @brief jump table for file system read_write operation
 */
typedef struct 
filesystem_jump_table{
    int32_t (*read_dentry_by_name) (const uint8_t*, dentry_t*);
	int32_t (*read_dentry_by_index) (uint32_t, dentry_t*);
	int32_t (*read_data) (uint32_t, uint32_t, uint8_t*, uint32_t);
    int32_t (*write) (void);
} fsjmp_t;


/**
 * @brief File system structure.
 * This struct provdies namespace, compatibility, virtualization for filesystem.
 * f_rw - a series of read/wriet operations
 * open_fs - initialize file system
 * close_fs - close file system
 * file_num - number of files this system contain, including directory itself
 * r_times - number of this reads this system has perfomed
 * w_times - number of this writes this system has perfomed
 * sys_st_addr - starting address in memory for file system
 * sys_ed_addr - ending address in memory for file system
 * iblock_num - N, total number of iblocks available (can be unfilled)
 * block_size - size of one block
 * dblock_entry_size - size of each entry to data block (stored in inode)
 * dblock_entry_offset - size of metadata (in this case, length) before dblock_entry
 * dblock_offset - address of the first datablock
 * boot_block_padding - padding for dentry (the address of the first dentry)
 * dentry_size - the size of one dentry
 * filename_size - the size of a filename stored in dentry 
 * (Note we can miss '/0' accoding to Appendix A, so we need 33 bytes to store the filename)
 */
typedef struct 
filesystem{
    fsjmp_t f_rw; /* file system read/wriet operations*/
    int32_t (*openr)(fd_t*, const uint8_t*,int32_t); /* Open file/directory as read-only this installs ioctl to file descriptor table */
    fops_t f_ioctl; /* file system ioctl */
    fops_t d_ioctl;
    int32_t (*open_fs)(uint32_t addr); /* this installs ioctl to file system */
    int32_t (*close_fs)(void);
    /* A series of shared variables you might want to make use of */
    uint32_t file_num; 
    uint32_t r_times,w_times;
    uint32_t sys_st_addr;
    uint32_t sys_ed_addr;
    uint32_t iblock_num;
    uint32_t block_size;
    uint32_t dblock_entry_size;
    uint32_t dblock_entry_offset;
    uint32_t dblock_offset;
    uint32_t boot_block_padding;
    uint32_t dentry_size;
    uint32_t filename_size;
} fs_t;

extern fs_t readonly_fs;

#endif

