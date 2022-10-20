#include "filesystem.h"
#include "multiboot.h"
#include "lib.h"
static int32_t open_fs(uint32_t addr);
static int32_t close_fs();

fs_t readonly_fs={
    .open_fs=open_fs,
    .close_fs=close_fs
};

static int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
static int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
static int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
static int32_t fake_write (void);

 
/**
 * @brief read 4 Bytes from memory
 * @param addr memory address 
 * @return ** uint32_t 4 Bytes contained in memory
 */
static uint32_t read_4B(uint32_t addr){
    return *((uint32_t*)addr);
}

/**
 * @brief read 1 Byte from memory
 * @param addr memory address 
 * @return ** uint32_t 1 Byte contained in memory
 */
static uint8_t read_1B(uint32_t addr){
    return *((uint8_t*)addr);
}

/**
 * @brief Check whether inode is legal and addr within the filesystem
 * Internal used. 
 * @param inode - number of inode
 * @param addr - address within the filesystem to check
 * @return ** int32_t 0 - success
 * else - failed
 */
static int32_t fs_sanity_check(uint32_t inode,uint32_t addr){
    return (inode<0||inode>readonly_fs.iblock_num||addr<readonly_fs.sys_st_addr||addr>readonly_fs.sys_ed_addr);
}


/**
 * @brief Initialization for Fileystem
 * @param addr The address of Filesystem in multiboot, should be cast into uint32_t
 * @return ** int32_t 
 */
static int32_t 
open_fs(uint32_t addr){
    module_t* _addr=(module_t*)addr;
    // load all functions into struct
    readonly_fs.f_rw.write=fake_write;
    readonly_fs.f_rw.read_data=read_data;
    readonly_fs.f_rw.read_dentry_by_index=read_dentry_by_index;
    readonly_fs.f_rw.read_dentry_by_name=read_dentry_by_name;
    // initialize all shared variables
    readonly_fs.sys_st_addr=_addr->mod_start;
    readonly_fs.sys_ed_addr=_addr->mod_end;
    readonly_fs.file_num=read_4B(_addr->mod_start); /* first 4 Bytes, number of dentries */
    readonly_fs.iblock_num=read_4B(_addr->mod_start+4);
    printf("Opening file system which contains %d files&dir, %d iblocks(N)\n",readonly_fs.file_num,readonly_fs.iblock_num);
    readonly_fs.r_times=readonly_fs.w_times=0;

    readonly_fs.block_size=4096;
    readonly_fs.dblock_entry_size=4;
    readonly_fs.dblock_entry_offset=4; /* the number of bytes in inode before data block # : In this case, length */
    readonly_fs.dblock_offset=readonly_fs.block_size*(readonly_fs.iblock_num+1); /* +1 : number of boot block */
    readonly_fs.boot_block_padding=64;
    readonly_fs.dentry_size=64;
    readonly_fs.filename_size=32;
    return 0;
}

/**
 * @brief Tear down File System
 * 
 * @return ** int32_t 
 */
static int32_t 
close_fs(){
    // Unload all functions to struct
    
    // erase all shared variables
    return 0;
}

/**
 * @brief For read-only file system, no use, directly return 0
 * 
 * @return ** int32_t - 0 for read only file system
 */
static int32_t fake_write (){
    return 0;
}



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
static int32_t read_from_block(uint32_t *offset,uint32_t dnum,uint8_t* buf, uint32_t length,uint32_t *buf_ptr){
    /* data block starting addres : starting address + (number of blocks before this dblock)*block_size */
    /* number of blocks before dblock : dblock_index + 1 (bootblock) + number of iblocks (N) */
    if(offset==NULL||buf==NULL||buf_ptr==NULL) return -1;
    uint32_t data_block_addr=readonly_fs.sys_st_addr+(1+readonly_fs.iblock_num+dnum)*readonly_fs.block_size; 
    data_block_addr+=(*offset);
    *offset=0;
    if(fs_sanity_check(0,data_block_addr)) return -1;
    uint32_t i;
    length=length>readonly_fs.block_size?readonly_fs.block_size:length; /* length = min (length, block_size) */
    
    for(i=0;i<length;i++){
        if(fs_sanity_check(0,data_block_addr+i)) return -1;
        buf[(*buf_ptr)++]=read_1B(data_block_addr+i);
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
static int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    uint32_t data_block_num,buf_ptr=0,read_length,ret;
    uint32_t inode_addr,file_length,data_block_offset,data_block_entry_addr;
    if(buf==NULL) return -1; 
    if(fs_sanity_check(inode,readonly_fs.sys_st_addr)) return -1; /* inode out of bound */
    if(length<=0) return 0; /* no need to read */
    
    /* calculate inode address */
    inode_addr=(uint32_t)readonly_fs.sys_st_addr+(inode+1)*readonly_fs.block_size; /* inode+1 : boot block and number of inode */
    if(fs_sanity_check(0,inode_addr)||fs_sanity_check(0,inode_addr+3)) return -1;
    
    /* extract file length */
    file_length=read_4B(inode_addr);
    if(offset>=file_length) return -1; /* invalid index for reading */

    /* calculate in-block offset*/
    data_block_offset=offset/readonly_fs.block_size;
    offset%=readonly_fs.block_size; /* discard offset that fully occupies previous blocks*/

    /* calculate first data block address */
    data_block_entry_addr=inode_addr+
    readonly_fs.dblock_entry_offset+
    data_block_offset*readonly_fs.dblock_entry_size;
    if(fs_sanity_check(0,data_block_entry_addr)) return -1;
    

    /* !This always assume length in inode is accurate! */
    length=file_length>length?length:file_length; /* Actual reading length : length = min(length, file_length) */
    ret=length; /* set return value to this length */
    while(length>0){
        data_block_num=read_4B(data_block_entry_addr);
        read_length=read_from_block(&offset,data_block_num,buf,length,&buf_ptr); /* read from data block, update buf using buf_ptr */
        if(read_length<=0) return -1;
        length-=read_length; /* the length left to read */
        data_block_entry_addr+=readonly_fs.dblock_entry_size; /* move to next data block */
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
static int32_t read_after_fname(uint32_t dentry_addr,dentry_t* dentry){
    uint32_t i;
    dentry_addr+=readonly_fs.filename_size;
    if(fs_sanity_check(0,dentry_addr)||fs_sanity_check(0,dentry_addr+3)) return -1;
    dentry->filetype=read_4B(dentry_addr);
    dentry_addr+=4;
    if(fs_sanity_check(0,dentry_addr)||fs_sanity_check(0,dentry_addr+3)) return -1;
    dentry->inode_num=read_4B(dentry_addr);
    dentry_addr+=4;
    for(i=0;i<6;i++){
        if(fs_sanity_check(0,dentry_addr)||fs_sanity_check(0,dentry_addr+3)) return -1;
        dentry->reserved[i]=read_4B(dentry_addr);
        dentry_addr+=4;
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
static int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry){
    if(dentry==NULL||index>readonly_fs.iblock_num||index<0){
        return -1;
    }
    uint32_t dentry_addr=readonly_fs.sys_st_addr+readonly_fs.boot_block_padding+index*readonly_fs.dentry_size;
    if(fs_sanity_check(0,dentry_addr)) return -1;
    uint32_t i,check_EOS=0;
    for(i=0;i<readonly_fs.filename_size;i++){
        if(fs_sanity_check(0,dentry_addr+i)) return -1;
        dentry->filename[i]=read_1B(dentry_addr+i);
        if(dentry->filename[i]=='\0') check_EOS=1;
    }
    if(!check_EOS) dentry->filename[readonly_fs.filename_size]='\0'; /* no zero padding, add at the end */
    return read_after_fname(dentry_addr,dentry);
}

/**
 * @brief read dentry accoding to name
 * 
 * @param fname 
 * @param dentry 
 * @return ** int32_t -1 on failure or fname unfound
 * other on success
 */
static int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry){
    if(dentry==NULL||fname==NULL){
        return -1;
    }
    uint32_t dentry_addr=readonly_fs.sys_st_addr+readonly_fs.boot_block_padding;
    if(fs_sanity_check(0,dentry_addr)) return -1;
    uint32_t i,dentry_index;
    uint32_t check_EOS;
    for(dentry_index=0;dentry_index<readonly_fs.iblock_num;dentry_index++){
        if(fs_sanity_check(0,dentry_addr)) return -1;
        check_EOS=0;
        for(i=0;i<readonly_fs.filename_size;i++){
            if(fs_sanity_check(0,dentry_addr+i)) return -1;
            dentry->filename[i]=read_1B(dentry_addr+i);
            if(dentry->filename[i]=='\0') check_EOS=1;
        }
        if(!check_EOS) dentry->filename[readonly_fs.filename_size]='\0'; /* no zero padding, add at the end */
        if(strncmp((int8_t*)fname,(int8_t*)dentry->filename,strlen((int8_t*)fname)+1)==0){
            /* strlen((int8_t*)fname)+1 : both string ends at the same time */
            return read_after_fname(dentry_addr,dentry);
        }
        dentry_addr+=readonly_fs.dentry_size;
    }
    return -1;
}

