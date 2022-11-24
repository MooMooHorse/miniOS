/**
 * @file filesystem.c
 * @author haor2
 * @brief File system supporting file open,read write, close, load program
 * @version 0.1
 * @date 2022-10-26
 * 
 * 
 * @version 1.0
 * @author haor2
 * @brief enable writable filesystem
 * @date 2022-11-19
 * @copyright Copyright (c) 2022
 * 
 */


#include "filesystem.h"
#include "multiboot.h"
#include "lib.h"
#include "tests.h"
static int32_t open_fs(uint32_t addr);
static int32_t close_fs();

fs_t fs = {
    .open_fs = open_fs,
    .close_fs = close_fs
};

/* internal file read/write function set */
static int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
static int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
static int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
static int32_t create_file(const uint8_t* fname,int32_t nbytes);
static int32_t write_data(uint32_t inode, uint32_t offset,const uint8_t* buf, uint32_t length);
static int32_t remove_file(const uint8_t* fname,int32_t nbytes);
static int32_t rename_file(const uint8_t* src,const uint8_t* dest,int32_t nbytes);
// static int32_t fake_write(void);
/* file system io function set */
static int32_t openr(file_t* ret, const uint8_t* fname, int32_t findex);
static int32_t file_open(file_t* ret, const uint8_t* fname, int32_t findex);
static int32_t directory_open(file_t* ret, const uint8_t* fname, int32_t findex);
static int32_t file_read(file_t* file, void* buf, int32_t nbytes);
static int32_t directory_read(file_t* file, void* buf, int32_t nbytes);
static int32_t file_write(file_t* file, const void* buf, int32_t nbytes);
static int32_t directory_write(file_t* file, const void* buf, int32_t nbytes);
static int32_t file_close(file_t* file);
static int32_t directory_close(file_t* file);
/* program loader function set */
static int32_t load_prog(const uint8_t* prog_name, uint32_t addr, uint32_t nbytes);
static int32_t check_exec(const uint8_t* prog_name);
/* scan filesytem function set */
static int32_t mark_dblock(uint32_t inode);
static int32_t mark_after_fname(uint32_t dentry_addr,dentry_t* dentry);
static int32_t mark_inode_and_dblock();

/* file create remove operation */
static int32_t create_file(const uint8_t* fname,int32_t nbytes);




/* shortcut for calculating address */

inline uint32_t INODE_ADDR(uint32_t inode){
    return fs.sys_st_addr + (inode + 1) * fs.block_size;
}

inline uint32_t DENTRY_ADDR(uint32_t index){
    return fs.sys_st_addr + fs.boot_block_padding + index * fs.dentry_size;
}
/**
 * @brief read 4 Bytes from memory
 * @param addr memory address
 * @return ** uint32_t 4 Bytes contained in memory
 */
static inline uint32_t read_4B(uint32_t addr) {
    return *((uint32_t*) addr);
}

/**
 * @brief read 1 Byte from memory
 * @param addr memory address
 * @return ** uint32_t 1 Byte contained in memory
 */
static inline uint8_t read_1B(uint32_t addr) {
    return *((uint8_t*) addr);
}

/**
 * @brief Check whether inode is legal and addr within the filesystem
 * Internal used.
 * @param inode - number of inode
 * @param addr - address within the filesystem to check
 * @return ** int32_t 0 - success
 * else - failed
 */
static inline int32_t fs_sanity_check(uint32_t inode, uint32_t addr) {
    return (inode < 0 || inode > fs.iblock_num || addr < fs.sys_st_addr
        || addr > fs.sys_ed_addr);
}

/**
 * @brief Initialization for Fileystem
 * @param addr The address of Filesystem in multiboot, should be cast into uint32_t
 * @return ** int32_t
 */
static int32_t
open_fs(uint32_t addr) {
    module_t* _addr = (module_t*) addr;
    int32_t   i;
    uint32_t filled_rate=0;
    // load all functions into struct

    /* extended functionality : program loader */
    fs.load_prog = load_prog;

    /* install lower-level r/w control to file system */
    fs.f_rw.write_data = write_data;
    fs.f_rw.read_data = read_data;
    fs.f_rw.read_dentry_by_index = read_dentry_by_index;
    fs.f_rw.read_dentry_by_name = read_dentry_by_name;
    fs.f_rw.create_file=create_file;
    fs.f_rw.remove_file=remove_file;
    fs.f_rw.rename_file=rename_file;

    fs.openr = openr; /* open file/directory as read-oonly*/
    fs.check_exec = check_exec; /* check if file is exec */ 

    /* install ioctl for file to file system */
    fs.f_ioctl.open = file_open;
    fs.f_ioctl.close = file_close;
    fs.f_ioctl.read = file_read;
    fs.f_ioctl.write = file_write;
    /* install ioctl for directory to file system */
    fs.d_ioctl.open = directory_open;
    fs.d_ioctl.close = directory_close;
    fs.d_ioctl.read = directory_read;
    fs.d_ioctl.write = directory_write;

    // initialize all shared variables
    fs.sys_st_addr = _addr->mod_start;
    fs.sys_ed_addr = _addr->mod_end;
    fs.file_num = read_4B(_addr->mod_start); /* first 4 Bytes, number of dentries */
    fs.iblock_num = read_4B(_addr->mod_start + 4);
    fs.dblock_num = read_4B(_addr->mod_start + 8 );
    printf("Opening file system which contains %d files&dir, %d iblocks(N)\n",
           fs.file_num,
           fs.iblock_num);
    printf("Filesystem has %d datablocks in total\n",fs.dblock_num);
    fs.r_times = fs.w_times = 0;
    /* Install a set of file system parameters(properties) */
    fs.block_size = 4096;
    fs.dblock_entry_size = 4;
    fs.dblock_entry_offset = 4;  /* the number of bytes in inode before data block # : In this case, length */
    fs.dblock_offset = fs.block_size * (fs.iblock_num + 1); /* +1 : number of boot block */
    fs.boot_block_padding = 64;
    fs.dentry_size = 64;
    fs.filename_size = 32;

    /* start initializing iblock, datablock map */
    for(i=0;i<N_INODE;i++) fs.imap[i]=0;
    for(i=0;i<D_DBLOCKS;i++) fs.dmap[i]=0;
    if(-1==mark_inode_and_dblock()){
        printf("file system boot failed\n");
        return -1;
    }
    #ifdef RUN_TESTS
    for(i=0;i<N_INODE;i++){
        if(fs.imap[i]){
            printf("inode %d is filled with %d length of file ",i,fs.flength[i]);
        }
    }

    for(i=0;i<D_DBLOCKS;i++){
        if(fs.dmap[i]){
            printf("dblock %d is used \n",i);
            filled_rate+=100;
        }
    }
    printf("file system dblock is filled with rate %d%%\n",filled_rate/fs.dblock_num);
    #endif
    return 0;
}

/**
 * @brief open file/directory
 * For arguments : only ret, fname will be used
 * @param ret - return file struct
 * @param fname - file name to open
 * @param findex - discarded
 * @return ** int32_t 0 on success
 * -1 on failure
 */
static int32_t openr(file_t* ret, const uint8_t* fname, int32_t findex) {
    dentry_t dentry;
    if(-1==fs.f_rw.read_dentry_by_name(fname, &dentry)){
        return -1;
    }
    if (dentry.filetype == DESCRIPTOR_ENTRY_DIR)
        return directory_open(ret, fname, findex);
    return file_open(ret, fname, findex);
}

/**
 * @brief Open a file in the file system
 * For arguments : only ret, fname will be used
 * @param ret - return file struct
 * @param fname - file name to open
 * @param findex - discarded
 * @return ** int32_t 0 on sccess, -1 on failure
 */
static int32_t
file_open(file_t* ret, const uint8_t* fname, int32_t findex) {
    dentry_t dentry;
    if (ret == NULL || fname == NULL)
        return -1;
    if (fs.f_rw.read_dentry_by_name(fname, &dentry) == -1) {
        return -1;
    }
    ret->fops.open = fs.f_ioctl.open;
    ret->fops.close = fs.f_ioctl.close;
    ret->fops.read = fs.f_ioctl.read;
    ret->fops.write = fs.f_ioctl.write;

    ret->pos = 0;
    ret->flags = DESCRIPTOR_ENTRY_FILE | F_OPEN;
    ret->inode = dentry.inode_num;
    return 0;
}
/**
 * @brief Open a directory in the file system
 * For arguments : only ret, fname will be used
 * @param ret - return file struct
 * @param fname - file name to open
 * @param findex - discarded
 * @return ** int32_t 0 on sccess, -1 on failure
 */
static int32_t
directory_open(file_t* ret, const uint8_t* fname, int32_t findex) {
    dentry_t dentry;
    if (ret == NULL || fname == NULL)
        return -1;
    if (fs.f_rw.read_dentry_by_name(fname, &dentry) == -1) {
        return -1;
    }
    fs.f_rw.read_dentry_by_name(fname, &dentry);
    ret->fops.open = fs.d_ioctl.open;
    ret->fops.close = fs.d_ioctl.close;
    ret->fops.read = fs.d_ioctl.read;
    ret->fops.write = fs.d_ioctl.write;
    ret->pos = 0;
    ret->flags = DESCRIPTOR_ENTRY_DIR | F_OPEN;
    ret->inode = dentry.inode_num;
    return 0;
}

/**
 * @brief Read n bytes from the file into buf, file offset (position) is given in file struct
 * @param file - file struct
 * @param buf - read result will go to buf
 * @param nbytes - n bytes to read
 * @return ** int32_t
 */
static int32_t
file_read(file_t* file, void* buf, int32_t nbytes) {
    int32_t ret;
    if (file == NULL || buf == NULL)
        return -1;
    if (fs_sanity_check(file->inode, fs.sys_st_addr))
        return -1;
    if (nbytes <= 0)
        return 0;
    ret = fs.f_rw.read_data(file->inode, file->pos, (uint8_t*) buf, nbytes);
    if (ret == -1)
        return 0;
    file->pos += ret; /* update file offset */
    return ret;
}

/**
 * @brief Read file(directory) name
 * According to hint 3.2 read_dir should read one byte at a time
 * @param file - file struct
 * @param buf - read result will go to buf
 * @param nbytes - n bytes to read
 * @return ** int32_t number of bytes read for file name -1 on failure
 */
static int32_t
directory_read(file_t* file, void* buf, int32_t nbytes) {
    int32_t ret, i;
    if (file == NULL || buf == NULL)
        return -1;
    if (fs_sanity_check(file->inode, fs.sys_st_addr))
        return -1;
    if (nbytes <= 0)
        return 0;
    dentry_t dentry;
    ret = fs.f_rw.read_dentry_by_index(file->pos, &dentry);
    if (ret == -1)
        return 0;
    file->pos++; /* each time advance one file */
    if (nbytes > strlen((int8_t*) dentry.filename))
        nbytes = strlen((int8_t*) dentry.filename);
    for (i = 0; i < nbytes; i++)
        ((uint8_t*) buf)[i] = dentry.filename[i];
    ((uint8_t*) buf)[nbytes] = '\0'; /* terminate with NUL */
    return nbytes;
}

/**
 * @brief write to a file with buf of length nbytes
 * @param file - file to write
 * @param buf - buffer to write
 * @param nbytes - length of buffer
 * @return ** int32_t - number of bytes you wrote
 * -1 on failure for write
 */
static int32_t
file_write(file_t* file, const void* buf, int32_t nbytes) {
    int32_t ret;
    if (file == NULL || buf == NULL)
        return -1;
    if (fs_sanity_check(file->inode, fs.sys_st_addr))
        return -1;
    if (nbytes <= 0)
        return 0;
    ret = fs.f_rw.write_data(file->inode, file->pos, (uint8_t*) buf, nbytes);
    if (ret == -1)
        return -1;
    file->pos += ret; /* update file offset */
    return ret;
}
/**
 * @brief Read only system doesn't support write
 * @param file - discarded
 * @param buf - discarded
 * @param nbytes - discarded
 * @return ** int32_t
 */
static int32_t
directory_write(file_t* file, const void* buf, int32_t nbytes) {
    return -1;
}

/**
 * @brief close file
 * @param file
 * @return ** int32_t
 */
static int32_t
file_close(file_t* file) {
    if (file == NULL)
        return -1;
    if (fs_sanity_check(file->inode, fs.sys_st_addr))
        return -1;
    file->fops.close = NULL;
    file->fops.read = NULL;
    file->fops.write = NULL;
    file->fops.open = NULL;
    file->inode = -1;
    file->pos = 0;
    file->flags = F_CLOSE;
    return 0;
}
/**
 * @brief close file
 * @param file
 * @return ** int32_t
 */
static int32_t
directory_close(file_t* file) {
    if (file == NULL)
        return -1;
    if (fs_sanity_check(file->inode, fs.sys_st_addr))
        return -1;
    file->fops.close = NULL;
    file->fops.read = NULL;
    file->fops.write = NULL;
    file->fops.open = NULL;
    file->inode = -1;
    file->pos = 0;
    file->flags = F_CLOSE;
    return 0;
}

/**
 * @brief Tear down File System, But this hasn't happened because we
 * don't have power down
 * @return ** int32_t
 */
static int32_t
close_fs() {
    // Unload all functions to struct

    // erase all shared variables
    return 0;
}

/**
 * @brief For read-only file system, no use, directly return 0
 *
 * @return ** int32_t - 0 for read only file system
 */
// static int32_t fake_write() {

    
//     return -1;
// }

/**
 * @brief Read from buffer with maximum length length.
 * This function operates on each data block.
 * @param offset - pointer to offset
 * @param dnum - data block number, specifying which data block this function reads
 * @param buf - read result will go to buf
 * @param length - maximum length can be read from this data block
 * @param buf_ptr - pointer pointing at bottom of buf
 * @return ** int32_t - number of bytes read
 * <=0 when failed
 */
static int32_t
read_from_block(uint32_t* offset, uint32_t dnum, uint8_t* buf, uint32_t length, uint32_t* buf_ptr) {
    /* data block starting addres : starting address + (number of blocks before this dblock)*block_size */
    /* number of blocks before dblock : dblock_index + 1 (bootblock) + number of iblocks (N) */
    if (offset == NULL || buf == NULL || buf_ptr == NULL)
        return -1;
    uint32_t data_block_addr = fs.sys_st_addr + (1 + fs.iblock_num + dnum) * fs.block_size;
    
    length = length > (fs.block_size-(*offset)) ? 
    (fs.block_size-(*offset)) : length; /* length = min (length, block_size) */

    data_block_addr += (*offset);
    *offset = 0;
    // if (fs_sanity_check(0, data_block_addr))
    //     return -1;
    uint32_t i;

    for (i = 0; i < length; i++) {
        if (fs_sanity_check(0, data_block_addr + i))
            return -1;
        buf[(*buf_ptr)++] = read_1B(data_block_addr + i);
    }
    return length;
}

/**
 * @brief The three routines provided by the file system module return -1 on failure,
 * indicating a non-existent file or invalid index in the case of the first two calls,
 * or an invalid inode number in the case of the last routine.
 */

/**
 * @brief Read at most length bytes data from file staring from the offset-th byte in the file
 * with inode number inode, and the result goes into buf.
 * @param inode - number of inode
 * @param offset - offset in file from which the read starts
 * @param buf - where the read result goes
 * @param length - read at most length bytes
 * @return ** int32_t bytes read
 * >=0 on success
 * <0 read failed
 */
static int32_t
read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {
    uint32_t data_block_num, buf_ptr = 0, ret;
    uint32_t inode_addr, file_length, data_block_offset, data_block_entry_addr;
    int32_t read_length,max_read_length;
    if (buf == NULL|| 0 == fs.imap[inode])
        return -1;
    if (fs_sanity_check(inode, fs.sys_st_addr))
        return -1; /* inode out of bound */
    if (length <= 0)
        return 0; /* no need to read */

    /* calculate inode address */
    inode_addr = (uint32_t) fs.sys_st_addr
        + (inode + 1) * fs.block_size; /* inode+1 : boot block and number of inode */
    if (fs_sanity_check(0, inode_addr) || fs_sanity_check(0, inode_addr + 3))
        return -1;

    /* extract file length */
    file_length = read_4B(inode_addr);
    if (offset >= file_length) {
        return 0; /* invalid index for reading */
    }
    max_read_length = file_length - offset;

    /* calculate in-block offset*/
    data_block_offset = offset / fs.block_size;
    offset %= fs.block_size; /* discard offset that fully occupies previous blocks*/

    /* calculate first data block address */
    data_block_entry_addr = inode_addr +
        fs.dblock_entry_offset +
        data_block_offset * fs.dblock_entry_size;
    if (fs_sanity_check(0, data_block_entry_addr))
        return -1;

    /* !This always assume length in inode is accurate! */
    length = (max_read_length) > length ? length : (max_read_length); /* Actual reading length */
    ret = length;                                         /* set return value to this length */
    while (length > 0) {
        data_block_num = read_4B(data_block_entry_addr);
        read_length = read_from_block(&offset,
                                      data_block_num,
                                      buf,
                                      length,
                                      &buf_ptr); /* read from data block, update buf using buf_ptr */
        if (read_length <= 0)
            return -1;
        length -= read_length;                                  /* the length left to read */
        data_block_entry_addr += fs.dblock_entry_size; /* move to next data block */
    }
    return ret;
}
/**
 * @brief After reading filename into dentry, this function fills the rest of dentry fields
 * Internally used
 * @param dentry_addr
 * @param dentry
 * @return ** int32_t
 */
static int32_t
read_after_fname(uint32_t dentry_addr, dentry_t* dentry) {
    uint32_t i;
    dentry_addr += fs.filename_size;
    if (fs_sanity_check(0, dentry_addr) || fs_sanity_check(0, dentry_addr + 3))
        return -1;
    dentry->filetype = read_4B(dentry_addr);

    /* bad filetype */
    if(dentry->filetype>3) return -1;

    dentry_addr += 4;
    if (fs_sanity_check(0, dentry_addr) || fs_sanity_check(0, dentry_addr + 3))
        return -1;
    dentry->inode_num = read_4B(dentry_addr);

    /* bad inode # */
    if(dentry->inode_num>=N_INODE||0==fs.imap[dentry->inode_num]) return -1;

    dentry_addr += 4;
    for (i = 0; i < 6; i++) {
        if (fs_sanity_check(0, dentry_addr) || fs_sanity_check(0, dentry_addr + 3))
            return -1;
        dentry->reserved[i] = read_4B(dentry_addr);
        dentry_addr += 4;
    }
    return 0;
}

/**
 * @brief read dentry according to index
 *
 * @param index
 * @param dentry
 * @return ** int32_t
 */
static int32_t
read_dentry_by_index(uint32_t index, dentry_t* dentry) {
    if (dentry == NULL || index >= N_INODE ) {
        return -1;
    }
    uint32_t dentry_addr = fs.sys_st_addr + fs.boot_block_padding + index * fs.dentry_size;
    if (fs_sanity_check(0, dentry_addr))
        return -1;
    uint32_t i, check_EOS = 0;
    /* duplicate filename */
    for (i = 0; i < fs.filename_size; i++) {
        if (fs_sanity_check(0, dentry_addr + i))
            return -1;
        dentry->filename[i] = read_1B(dentry_addr + i);
        if (dentry->filename[i] == '\0'){
            if(i==0) return -1; /* no file name */
            check_EOS = 1;
        }
    }
    if (!check_EOS)
        dentry->filename[fs.filename_size] = '\0'; /* no zero padding, add at the end */
    return read_after_fname(dentry_addr, dentry);
}

/**
 * @brief read dentry accoding to name
 *
 * @param fname
 * @param dentry
 * @return ** int32_t -1 on failure or fname unfound
 * other on success
 */
static int32_t
read_dentry_by_name(const uint8_t* fname, dentry_t* dentry) {
    /* file name must be non-empty : equivalent to check if each dentry has non-empty file-name */
    if (dentry == NULL || fname == NULL || strlen((int8_t*) fname) == 0 ) {
        return -1;
    }
    if (strlen((int8_t*) fname) > fs.filename_size) {
        return -1;
    }
    uint32_t dentry_addr = fs.sys_st_addr + fs.boot_block_padding;
    if (fs_sanity_check(0, dentry_addr))
        return -1;
    uint32_t i, dentry_index;
    uint32_t check_EOS;
    /* search in whole file system */
    for (dentry_index = 0; dentry_index < fs.iblock_num; dentry_index++) {
        if (fs_sanity_check(0, dentry_addr))
            return -1;
        check_EOS = 0;
        /* search through 32 bytes */
        for (i = 0; i < fs.filename_size; i++) {
            if (fs_sanity_check(0, dentry_addr + i))
                return -1;
            dentry->filename[i] = read_1B(dentry_addr + i);
            if (dentry->filename[i] == '\0')
                check_EOS = 1;
        }
        /* if without EOS add one NUL character */
        if (!check_EOS)
            dentry->filename[fs.filename_size] = '\0'; /* no zero padding, add at the end */
        if (strncmp((int8_t*) fname, (int8_t*) dentry->filename, strlen((int8_t*) fname) + 1) == 0) {
            /* strlen((int8_t*)fname)+1 : both string ends at the same time */
            return read_after_fname(dentry_addr, dentry);
        }
        dentry_addr += fs.dentry_size;
    }
    return -1;
}

/**
 * @brief Load the image. Read the WHOLE executable.
 * Illegal condition are:
 *      Exectuable size exceeds 4MB
 *      Illegal operands for memory read
 *      Read Failure
 * 
 * Internal implementation note: 
 *      Copy 4KB at a time, because this is how inside copy works, 
 *      so no overhead will be generated in this way.
 * @param prog_name - program name to read
 * @param addr  - virtual memory address you want to copy to
 * @param nbytes - number of bytes you want to copy to
 * @return ** int32_t - number of bytes you load
 * -1 on failure
 */
static int32_t
load_prog(const uint8_t* prog_name, uint32_t addr, uint32_t nbytes) {
    uint32_t read_block_size = (4 << 10); /* each time, try 4KB blocks, no time overhead */
    dentry_t dentry;
    if (read_dentry_by_name(prog_name, &dentry) == -1) {
        return -1;
    }
    int32_t ret = 0, offset = 0, bytes_read;
    /* read the whole executable */
    while (1) {
        bytes_read = fs.f_rw.read_data(dentry.inode_num, offset, (uint8_t*) addr, read_block_size);
        if (bytes_read == -1) return -1;
        if (bytes_read == 0) return bytes_read;
        /* read next data block */
        addr += bytes_read;
        ret += bytes_read;
        offset += bytes_read;
        if (ret > nbytes) {
            printf("executable too large\n");
            return -1;
        }
    }
    return ret;
}

/**
 * @brief check whether program is executable or not
 * 
 * @param prog_name - program name
 * @return ** int32_t - 
 * 1 : is executable
 * else : not executable  
 * -1 : file doesn't exist 
 * 0 : file not executable
 */
static int32_t
check_exec(const uint8_t* prog_name){
    dentry_t dentry;
    if(-1==fs.f_rw.read_dentry_by_name(prog_name,&dentry)){
        return -1;
    }
    uint8_t buf[5];
    if(-1==fs.f_rw.read_data(dentry.inode_num,0,buf,4)){
        return 0;
    }else{
        return (buf[0]==0x7f)&&(buf[1]==0x45)&&(buf[2]==0x4c)&&(buf[3]==0x46);
    }
}


/* scan filesystem function set */

/**
 * @brief after we find inode, we check the dblocks, and mark them as used
 * @param inode - inode number
 * @return ** void 
 */
static int32_t
mark_dblock(uint32_t inode){
    uint32_t i;
    uint32_t inode_addr = INODE_ADDR(inode);
    uint32_t data_block_entry_addr = inode_addr +
        fs.dblock_entry_offset;
    if(fs_sanity_check(0,inode_addr)||fs_sanity_check(0,data_block_entry_addr)){
        printf("toxic inode/dblock_entry address at inode %d\n",inode);
        return -1;
    }
    fs.flength[inode] = read_4B(inode_addr);
    uint32_t dblock_num = (fs.flength[inode]-1)/(int32_t)fs.block_size+1;
    uint32_t dblock_ind;
    if(dblock_num>=D_DBLOCKS){
        printf("bad file length at inode %d\n",inode);
        return -1;
    }
    for(i=0;i<dblock_num;i++){
        dblock_ind=read_4B(data_block_entry_addr);
        if(dblock_ind>=D_DBLOCKS){
            printf("toxic dblock index at inode %d\n",inode);
            continue;
        }
        fs.dmap[dblock_ind]=1; /* mark datablock */
        data_block_entry_addr+=fs.dblock_entry_size;
    }
    return 0;
}   

/**
 * @brief after extracting file name, we go to following items in dentry
 * and find inode 
 * @param dentry_addr - dentry address for current dentry 
 * @param dentry - dentry to fill
 * @return ** int32_t 
 */
static int32_t
mark_after_fname(uint32_t dentry_addr,dentry_t* dentry){
    dentry_addr += fs.filename_size;
    if (fs_sanity_check(0, dentry_addr) || fs_sanity_check(0, dentry_addr + 3))
        return -1;
    dentry->filetype = read_4B(dentry_addr);

    /* bad filetype */
    if(dentry->filetype>3) return -1;

    dentry_addr += 4;
    if (fs_sanity_check(0, dentry_addr) || fs_sanity_check(0, dentry_addr + 3))
        return -1;
    dentry->inode_num = read_4B(dentry_addr);
    /* bad inode # */
    if(dentry->inode_num>=N_INODE) return -1;

    mark_dblock(dentry->inode_num);

    return 0;
}

/**
 * @brief mark all inodes and dblocks in the filesystem 
 * @return ** int32_t 
 */
static int32_t
mark_inode_and_dblock(){
    uint32_t index;
    for(index=0;index<fs.file_num;index++){
        dentry_t dentry;
        uint32_t dentry_addr = fs.sys_st_addr + fs.boot_block_padding + index * fs.dentry_size;
        if (fs_sanity_check(0, dentry_addr))
            return -1;
        uint32_t i, check_EOS = 0;
        for (i = 0; i < fs.filename_size; i++) {
            if (fs_sanity_check(0, dentry_addr + i))
                return -1;
            dentry.filename[i] = read_1B(dentry_addr + i);
            if (dentry.filename[i] == '\0'){
                if(i==0) break; /* no file name */
                check_EOS = 1;
            }
        }
        if(i==0) continue; /* no file name */
        if (!check_EOS)
            dentry.filename[fs.filename_size] = '\0'; /* no zero padding, add at the end */
        if(0==mark_after_fname(dentry_addr, &dentry)){
            fs.imap[dentry.inode_num]=1;
        }
    }
    return 0;
}



/**
 * @brief find a file with dentry returned 
 * 
 * @param fname - file name to find 
 * @return ** dentry_t* 
 * NULL if not exist 
 */
static wdentry_t*
find_file(const uint8_t* fname){
    /* file name must be non-empty : equivalent to check if each dentry has non-empty file-name */
    if (fname == NULL || strlen((int8_t*) fname) == 0 ) {
        return NULL;
    }
    dentry_t dentry;
    if (strlen((int8_t*) fname) > fs.filename_size) {
        return NULL;
    }
    uint32_t dentry_addr = fs.sys_st_addr + fs.boot_block_padding;
    if (fs_sanity_check(0, dentry_addr))
        return NULL;
    uint32_t i, dentry_index;
    uint32_t check_EOS;
    /* search in whole file system */
    for (dentry_index = 0; dentry_index < fs.iblock_num; dentry_index++) {
        if (fs_sanity_check(0, dentry_addr))
            return NULL;
        check_EOS = 0;
        /* search through 32 bytes */
        for (i = 0; i < fs.filename_size; i++) {
            if (fs_sanity_check(0, dentry_addr + i))
                return NULL;
            dentry.filename[i] = read_1B(dentry_addr + i);
            if (dentry.filename[i] == '\0')
                check_EOS = 1;
        }
        /* if without EOS add one NUL character */
        if (!check_EOS)
            dentry.filename[fs.filename_size] = '\0'; /* no zero padding, add at the end */
        if (strncmp((int8_t*) fname, (int8_t*) dentry.filename, strlen((int8_t*) fname) + 1) == 0) {
            /* strlen((int8_t*)fname)+1 : both string ends at the same time */
            return (wdentry_t*)DENTRY_ADDR(dentry_index);
        }
        dentry_addr += fs.dentry_size;
    }
    return NULL;
}
/**
 * @brief allocate new inode
 * 
 * @return ** int32_t inode index 
 * -1 on not available 
 */
static int32_t
assign_inode(){
    int32_t i;
    for(i=0;i<fs.iblock_num;i++) if(fs.imap[i]==0) return i;
    return -1;
}
/**
 * @brief allocate new data block
 * 
 * @return ** int32_t data block index
 * -1 on not available 
 */
static int32_t
assign_dblock(){
    int32_t i;
    for(i=0;i<fs.dblock_num;i++) if(fs.dmap[i]==0) return i;
    return -1;
}


/**
 * @brief create a file in file system 
 * @param fname - name of the file
 * @return ** int32_t - 
 * -1 on no space
 * 0 on succuss
 */
static int32_t 
create_file(const uint8_t* fname,int32_t nbytes){
    if(nbytes>fs.filename_size) return -1; /* file name too long */
    int32_t new_inode=assign_inode();
    int32_t new_dblock=assign_dblock(); 
    inode_t* inode_addr= (inode_t*)INODE_ADDR(new_inode);

    if(new_inode==-1||new_dblock==-1){
        printf("no available inode or datablock : file creation failed\n");
        return -1;
    }
    wdentry_t* new_dentry=(wdentry_t*)DENTRY_ADDR(fs.file_num++);
    uint32_t  cp_len=nbytes;
    if(cp_len>fs.filename_size) cp_len=fs.filename_size;
    /* fill dentry */
    strncpy((int8_t*)new_dentry->filename,(int8_t*)fname,cp_len);
    if(cp_len<fs.filename_size) new_dentry->filename[cp_len]='\0';

    new_dentry->filetype=DESCRIPTOR_ENTRY_FILE;
    new_dentry->inode_num=new_inode;
    /* "in-disk" field */
    inode_addr->filelength=0; /* file length */
    inode_addr->dblock[0]=new_dblock; /* 1st data block */
    /* fs parameters */
    fs.imap[new_inode]=1;
    fs.dmap[new_dblock]=1;
    fs.flength[new_inode]=0;
    
    return 0;
}


/**
 * @brief rename the file from src file name to 
 * dest file name
 * @param src - source file name
 * @param dest - destination file name
 * @param nbytes - number of characters in dest file name 
 * @return ** int32_t - 0 on success
 * -1 on failure 
 */
static int32_t
rename_file(const uint8_t* src,const uint8_t* dest,int32_t nbytes){
    if(src==NULL||dest==NULL||nbytes>fs.filename_size) return -1; /* bad file name */
    wdentry_t* dentry=find_file(src);
    if(dentry==NULL||fs_sanity_check(dentry->inode_num,fs.sys_st_addr)) return -1; /* bad file */
    uint32_t  cp_len=nbytes;
    if(cp_len>fs.filename_size) cp_len=fs.filename_size;
    strncpy((int8_t*)dentry->filename,(int8_t*)dest,cp_len);
    dentry->filename[cp_len]='\0';
    return 0;
}

/**
 * @brief remove an inode
 * Internally used
 * @param inode inode index to remove
 * @return ** int32_t 
 * always 0 right now
 */
static int32_t
remove_inode(int32_t inode){
    inode_t* inode_addr=(inode_t*)INODE_ADDR(inode);
    int32_t  num_dblock=((inode_addr->filelength-1)/(int32_t)fs.block_size)+1;
    int32_t i;
    for(i=0;i<num_dblock;i++){
        /* clear fs parameters */
        fs.dmap[inode_addr->dblock[i]]=0;
        /* clear "in-disk" fields */
        inode_addr->dblock[i]=0;
    }
    /* clear fs parameters */
    fs.flength[inode]=0;
    fs.imap[inode]=0;
    /* clear "in-disk" fields */
    inode_addr->filelength=0;
    return 0;
}

/**
 * @brief remove a given file from filesystem
 * 
 * @param fname - filename to remove
 * @param nbytes - number of bytes of file name
 * @return ** int32_t - -1 on error
 * 0 on success 
 */
static int32_t 
remove_file(const uint8_t* fname,int32_t nbytes){
    if(fname==NULL||nbytes>fs.filename_size) return -1; /* bad file name */
    wdentry_t* dentry=find_file(fname);
    int32_t i;
    if(dentry==NULL||fs_sanity_check(dentry->inode_num,fs.sys_st_addr)) return -1; /* bad file */
    /* clear "in-disk" fields */
    for(i=0;i<fs.filename_size;i++) dentry->filename[i]='\0';
    dentry->filetype=0;
    if(-1==remove_inode(dentry->inode_num)) return -1;
    dentry->inode_num=0;
    fs.file_num--;
    /* swap the end dentry in bootblock to deleted dentry */
    wdentry_t* swap_dentry=(wdentry_t*)DENTRY_ADDR(fs.file_num);
    wdentry_t  temp_dentry;
    temp_dentry=*dentry;
    *dentry=*swap_dentry;
    *swap_dentry=temp_dentry;
    return 0;

}

/**
 * @brief write to the data block
 * @param offset - pointer to offset
 * @param dnum - data block number, specifying which data block this function reads
 * @param buf - read result will go to buf
 * @param length - maximum length can be read from this data block
 * @param buf_ptr - pointer pointing at bottom of buf
 * @return ** int32_t - number of bytes write
 * <=0 when failed
 */
static int32_t
write_to_block(uint32_t* offset, uint32_t dnum,const uint8_t* buf, uint32_t length, uint32_t* buf_ptr) {
    /* data block starting addres : starting address + (number of blocks before this dblock)*block_size */
    /* number of blocks before dblock : dblock_index + 1 (bootblock) + number of iblocks (N) */
    if (offset == NULL || buf == NULL || buf_ptr == NULL)
        return -1;
    uint32_t data_block_addr = fs.sys_st_addr + (1 + fs.iblock_num + dnum) * fs.block_size;
    
    length = length > (fs.block_size-(*offset)) ? 
    (fs.block_size-(*offset)) : length; /* length = min (length, block_size) */

    data_block_addr += (*offset);
    *offset = 0;
    // if (fs_sanity_check(0, data_block_addr))
    //     return -1;
    uint32_t i;

    for (i = 0; i < length; i++) {
        if (fs_sanity_check(0, data_block_addr + i))
            return -1;
        *((uint8_t*)(data_block_addr + i))=buf[(*buf_ptr)++];
    }
    return length;
}

/**
 * @brief write data to file given inode offset,
 * write buffer and write length 
 * @param inode - number of inode
 * @param offset - offset in file from which the read starts
 * @param buf - where the read result goes
 * @param length - read at most length bytes
 * @return ** int32_t bytes write
 * >=0 on success
 * <0 read failed
 */
static int32_t
write_data(uint32_t inode, uint32_t offset,const uint8_t* buf, uint32_t length) {
    uint32_t data_block_num, buf_ptr = 0, ret;
    uint32_t inode_addr;
    uint32_t file_length, data_block_offset, data_block_entry_addr;
    int32_t write_length,rest_len;
    if (buf == NULL|| 0 == fs.imap[inode])
        return -1;
    if (fs_sanity_check(inode, fs.sys_st_addr))
        return -1; /* inode out of bound */
    if (length <= 0)
        return 0; /* no need to read */

    /* calculate inode address */
    inode_addr = INODE_ADDR(inode);

    if (fs_sanity_check(0, inode_addr) || fs_sanity_check(0, inode_addr + 3))
        return -1;

    /* extract file length */
    file_length = read_4B(inode_addr);
    /* "in-disk" length = max(file_length, offset+length) */
    *((uint32_t*)inode_addr)=file_length>(offset+length)?file_length:(offset+length);
    /* file system properties */
    fs.flength[inode]=read_4B(inode_addr);

    rest_len=file_length-offset; /* how many bytes left to overwrite */

    /* calculate in-block offset*/
    data_block_offset = offset / fs.block_size;
    offset %= fs.block_size; /* discard offset that fully occupies previous blocks*/

    /* calculate first data block address */
    data_block_entry_addr = inode_addr +
        fs.dblock_entry_offset +
        data_block_offset * fs.dblock_entry_size;

    if (fs_sanity_check(0, data_block_entry_addr))
        return -1;

    /* !This always assume length in inode is accurate! */
    ret = length;            
    // printf("%d\n",rest_len);                             /* set return value to this length */
    while (length > 0) {
        if(rest_len>=0&&offset) /* equality with non-zero in-block offset : last file still at current block */
            data_block_num = read_4B(data_block_entry_addr); 
        else{
            data_block_num=assign_dblock();
            /* "in-disk" field */
            *((uint32_t*)data_block_entry_addr)=data_block_num==-1?0:data_block_num;
        }
        if(data_block_num==-1){
            printf("file system full\n");
            return -1;
        }
        /* update file system properties */
        fs.dmap[data_block_num]=1;
        write_length = write_to_block(&offset,
                                      data_block_num,
                                      buf,
                                      length,
                                      &buf_ptr); /* read from data block, update buf using buf_ptr */
        
        if (write_length <= 0)
            return -1;
        length -= write_length;   
        rest_len -= write_length;                /* the length left to read */
        data_block_entry_addr += fs.dblock_entry_size; /* move to next data block */
    }
    return ret;
}

/** TODO Race Condition
 * @brief cli sti for system call
 */


