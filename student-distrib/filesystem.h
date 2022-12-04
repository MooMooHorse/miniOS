#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H
#include "types.h"
#include "x86_desc.h"
/* flags for file struct table */
/* lower 2 bits will be used to identify file type : directory(0), rtc(1), file(2), terminal (3), sb16 (4) */
#define DESCRIPTOR_ENTRY_RTC 0
#define DESCRIPTOR_ENTRY_DIR 1
#define DESCRIPTOR_ENTRY_FILE 2
#define DESCRIPTOR_ENTRY_TERMINAL 3
#define DESCRIPTOR_ENTRY_SB16 4
#define F_OPEN (1 << 3)
#define F_CLOSE 0

#define MAX_INODE   256
#define MAX_DBLOCK  1024

/* directory entry, 64 Bytes : for read */
typedef struct
    dentry
{
    uint8_t filename[33];
    uint32_t filetype;
    uint32_t inode_num;
    uint32_t reserved[6];
} dentry_t;

/* dentry for write */
typedef struct
    wdentry
{
    uint8_t filename[32];
    uint32_t filetype;
    uint32_t inode_num;
    uint32_t reserved[6];
} wdentry_t;
typedef struct
    inode
{
    int32_t filelength;
    int32_t dblock[1023];
} inode_t;

/**
 * @brief jump table for file system read_write operation
 */
typedef struct
    filesystem_jump_table
{
    int32_t (*read_dentry_by_name)(const uint8_t *, dentry_t *);
    int32_t (*read_dentry_by_index)(uint32_t, dentry_t *);
    int32_t (*read_data)(uint32_t, uint32_t, uint8_t *, uint32_t);
    int32_t (*write_data)(uint32_t, uint32_t,const uint8_t *, uint32_t);
    int32_t (*create_file)(const uint8_t *, int32_t);
    int32_t (*remove_file)(const uint8_t *, int32_t);
    int32_t (*rename_file)(const uint8_t *, const uint8_t *, int32_t);
} fsjmp_t;

/**
 * @brief File system structure.
 * This struct provides namespace, compatibility, virtualization for filesystem.
 * f_rw - a series of read/write operations
 * open_fs - initialize file system
 * close_fs - close file system
 * file_num - number of files this system contain, including directory itself
 * r_times - number of this reads this system has performed
 * w_times - number of this writes this system has performed
 * sys_st_addr - starting address in memory for file system
 * sys_ed_addr - ending address in memory for file system
 * iblock_num - N, total number of iblocks available (can be unfilled)
 * block_size - size of one block
 * dblock_entry_size - size of each entry to data block (stored in inode)
 * dblock_entry_offset - size of metadata (in this case, length) before dblock_entry
 * dblock_offset - address of the first data block
 * boot_block_padding - padding for dentry (the address of the first dentry)
 * dentry_size - the size of one dentry
 * filename_size - the size of a filename stored in dentry
 * (Note we can miss '/0' according to Appendix A, so we need 33 bytes to store the filename)
 */
typedef struct
    filesystem
{
    fsjmp_t f_rw; /* file system read/write operations*/
    int32_t (*openr)(file_t *,
                     const uint8_t *,
                     int32_t); /* Open file/directory as read-only this installs ioctl to file struct table */
    fops_t f_ioctl;            /* file system ioctl */
    fops_t d_ioctl;
    int32_t (*open_fs)(uint32_t addr); /* this installs ioctl to file system */
    int32_t (*close_fs)(void);
    int32_t (*load_prog)(const uint8_t *, uint32_t, uint32_t);
    int32_t (*check_exec)(const uint8_t *);
    /* A series of shared variables you might want to make use of */
    uint32_t file_num;
    uint32_t r_times, w_times;
    uint32_t sys_st_addr;
    uint32_t sys_ed_addr;
    uint32_t iblock_num; /* N */
    uint32_t dblock_num; /* D */
    uint32_t block_size;
    uint32_t dblock_entry_size;
    uint32_t dblock_entry_offset;
    uint32_t dblock_offset;
    uint32_t boot_block_padding;
    uint32_t dentry_size;
    uint32_t filename_size;
    uint8_t imap[MAX_INODE];
    uint8_t dmap[MAX_DBLOCK];
    uint32_t flength[MAX_INODE];
} fs_t;

extern fs_t fs;

#endif
