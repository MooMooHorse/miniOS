/**
 * @file syscall.c
 * @author haor2
 * @brief Reserved for system call. In cp1, this file DOESN't go to another layer of indirection.
 * Instead, this file just prints out one system call is invoked
 * @version 0.1
 * @date 2022-10-15
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "x86_desc.h"
#include "syslink.h"
#include "syscall.h"
#include "process.h"
#include "filesystem.h"
#include "rtc.h"
#include "mmu.h"
#include "err.h"
#include "tests.h"

/**
 * @brief halt the program, return value is passed via eax to exec
 * 
 * @param status - return status
 * @return ** int32_t 
 */
int32_t halt (uint8_t status){
    uint32_t pid=get_pid();
    if(-1==discard_proc(pid,status)){
        printf("halt failed : discard process error\n");
        while(1);
    }
    printf("halt failed : unkown\n");
    while(1);
    
}
/** 
 * @brief currently only support command without parameters
 * 
 * @param command - executable file name 
 * @return ** int32_t - errno 
 */
int32_t execute (const uint8_t* command){
    uint8_t _command[CMD_MAX_LEN]; /* move user level data to kernel space */
    uint32_t pid,ppid,i,ret;
    
    /* ret : temporary here, have meaning at the end of execute */
    if((ret=copy_to_command(command,_command,CMD_MAX_LEN))==-1){
        return ERR_NO_CMD;
    }
    

    /* check executable */ 
    if(readonly_fs.check_exec(_command)!=1){
        return ERR_NO_CMD;
    }

    /* Attempt to find new pid, if none of in-stack pcb is availabe, allocate space for it */
    pid=(PCB_BASE-PCB_ptr)/PCB_SIZE+1; 
    for(i=pid-1;i>=1;i--){
        pcb_t* _pcb_ptr=(pcb_t*)(PCB_BASE-i*PCB_SIZE);
        if(0==_pcb_ptr->active){
            pid=i;
            break;
        }
    }
    /* special for cp4 sanity check, to make running program <= 3 */
    if((pid==(PCB_BASE-PCB_ptr)/PCB_SIZE+1) && pid == 4){
        printf("limit number of program below 4\n");
        return 0;
    }

    ppid=get_pid();

    /* create PCB */
    if(pcb_create(pid)==-1){
        printf("too many executable\n");
        return ERR_BAD_PID; /* errono to be defined */
    }

    /* set argument for this process :  */
    /* position of this function is IMPORTANT! */
    set_proc_args(command,ret,CMD_MAX_LEN);


    /* open PCB */
    if(pcb_open(ppid,pid,_command)==-1){
        printf("not enough space for PCB\n");
        return ERR_BAD_PID; /* errono to be defined */
    }

    /* program is under PCB base */
    if (0 != uvmmap_ext((pid-1)*PROG_SIZE+PCB_BASE)) {
        return ERR_VM_FAILURE;
    } /* TLB flushed */

    /* to this point, everything is checked, nothing should fail */


    /* program loader */
    readonly_fs.load_prog(_command,IMG_START,PROG_SIZE);

    /* set up TSS, only esp0 is needed to be modified */
    setup_tss(pid);
    /* iret */
    ret=switch_user(pid);
    if(ret==0){
    #ifdef RUN_TESTS
        printf("Your last program exits normally with return value 0\n");
    #endif
    }
    else{
        printf("Your last program exits with return value %d\n",ret);
    }
    return ret;

}

/**
 * @brief read from a file
 * 
 * @param fd - fild escriptor, an integer within range of [0,FILE_ARRAY_MAX)
 * @param buf - read buffer : user have to be responsible for buffer size
 * @param nbytes - number of bytes to read
 * @return ** int32_t number of bytes written -1 on failure
 */
int32_t read (int32_t fd, void* buf, uint32_t nbytes){
    if(fd<0||fd>=FILE_ARRAY_MAX){
        return -1;
    }
    file_t* file_entry;
    sti();

    
    uint32_t pid=get_pid();
    pcb_t* _pcb_ptr=(pcb_t*)(PCB_BASE-pid*PCB_SIZE);
    /* DO NOT move the second condition into get_file_entry(), this function is generally used */
    if(((file_entry=get_file_entry(fd))==NULL)||(!(_pcb_ptr->file_entry[fd].flags&F_OPEN))){
        return -1;
    }
    return file_entry->fops.read(file_entry, buf, nbytes);
}

/**
 * @brief write to a file(terminal for readonly filesystem )
 * 
 * @param fd - fild escriptor, an integer within range of [0,FILE_ARRAY_MAX)
 * @param buf - write buffer : user have to be responsible for buffer size
 * @param nbytes - number of bytes to write
 * @return ** int32_t number of bytes written -1 on failure
 */
int32_t write (int32_t fd, const void* buf, uint32_t nbytes){
    if(fd<0||fd>=FILE_ARRAY_MAX){
        return -1;
    }
    file_t* file_entry;
    sti();
    
    uint32_t pid=get_pid();
    pcb_t* _pcb_ptr=(pcb_t*)(PCB_BASE-pid*PCB_SIZE);
    /* DO NOT move the second condition into get_file_entry(), this function is generally used */
    if(((file_entry=get_file_entry(fd))==NULL)||(!(_pcb_ptr->file_entry[fd].flags&F_OPEN))){
        return -1;
    }
    return file_entry->fops.write(file_entry, buf, nbytes);
    
}

/**
 * @brief open a file. It will return -1 under situations below
 * 
 * @param filename 
 * @return ** int32_t 
 */
int32_t open (const uint8_t* filename){
    /* terminal is opened in exec, not in this system call */
    file_t* file_entry;
    uint32_t pid=get_pid(),i,filenum;
    uint8_t if_file_available=0;
    pcb_t* _pcb_ptr=(pcb_t*)(PCB_BASE-pid*PCB_SIZE);
    /* find unopened space in fda */
    for(i=0;i<FILE_ARRAY_MAX;i++){
        if(!(_pcb_ptr->file_entry[i].flags&F_OPEN)){
            if((file_entry=get_file_entry(filenum=i))==NULL){
                printf("invalid file struct\n");
                return -1; 
            }
            if_file_available=1; /* there is availabe space for this file in fda */
            break;
        }
    }
    /* no space in fda */
    if(!if_file_available){
        return -1;
    }
    /* if rtc */
    if(strncmp((int8_t*)"rtc",(int8_t*)filename,4)==0){
        if(rtc[0].ioctl.open(file_entry,filename,0)==-1){
            return -1;
        }
    }
    else{
        /* if regular file */
        if(readonly_fs.openr(file_entry,filename,0)==-1){
            return -1;
        }
    }
    /* terminal is opened in process.c init_file_entry() where we add bad call to fops jumptable */
    return filenum;
}
/**
 * @brief close file. On situation below, close should fail
 * fd exeeds the scope
 * fd isn't opened
 * close STDIN/STDOUT (in halt, they are directly closed using fops jumptable)
 * @param fd 
 * @return ** int32_t 
 */
int32_t close (int32_t fd){
    if(fd<2||fd>=FILE_ARRAY_MAX) return -1;
    file_t* file_entry;
    uint32_t pid=get_pid();
    pcb_t* _pcb_ptr=(pcb_t*)(PCB_BASE-pid*PCB_SIZE);
    /* DO NOT move the second condition into get_file_entry(), this function is generally used */
    if(((file_entry=get_file_entry(fd))==NULL)||(!(_pcb_ptr->file_entry[fd].flags&F_OPEN))){
        return -1;
    }
    return file_entry->fops.close(file_entry);
}

/** TO DO : nbytes unused 
 * @brief Reads program's arguments from its respective pcb_t into a user-level buffer.
 * 
 * @param buf Pointer to the buffer to be filled with the arguments.
 * @param nbytes Number of bytes to be read.
 * @return int32_t 0 on success, -1 on failure.
 * SIDE EFFECT: buf is filled with the arguments.
 */
int32_t getargs (uint8_t* buf, uint32_t nbytes){
    uint32_t pid,ppid;
    pcb_t* _pcb_ptr;
    if (buf == NULL) {
        // if buf is NULL, return -1
        return -1;
    }
    

    // get pointer to pcb_t
    pid = get_pid();
    _pcb_ptr = (pcb_t*)(PCB_BASE - pid * PCB_SIZE);
    ppid = _pcb_ptr->ppid;
    _pcb_ptr=(pcb_t*)(PCB_BASE - ppid * PCB_SIZE);

    if (strlen(_pcb_ptr->args) > nbytes) {
        printf("getargs: args are longer than number of bytes specified\n");
        return -1;
    }


    // arg existence check
    if (_pcb_ptr->args[0] == '\0') {
        
        printf("getargs: args are nonexistent\n");
        return -1;
    }

    // arg string-correctness check
    if (_pcb_ptr->args[strlen(_pcb_ptr->args)] != '\0') {
        printf("getargs: args are not null-terminated\n");
        return -1;
    }

    // copy args to pcb_ptr->args
    strcpy((int8_t*)buf, (int8_t*)_pcb_ptr->args);

    return 0;
}

/*!
 * @brief Maps video memory in the user page table so that user programs can modify video memory in
 * their virtual address space. Writes the start of user virtual address of video memory to the pointer
 * passed in.
 * @param screen_start is pointer to user buffer which holds starting virtual address of video memory.
 * @return 0 if successful, non-zero otherwise.
 * @sideeffect It modifies page table to enable user access of video memory.
 */
int32_t vidmap (uint8_t** screen_start){
    if (NULL == screen_start) {
        return -1;
    }

    return uvmmap_vid(screen_start);
}

int32_t set_handler (uint32_t signum, void* handler_address){
    return 0;
}
int32_t sigreturn (void){
    return 0;
}



/**
 * @brief install system call to idt, install system call to system call table
 * @param none
 * @return ** void 
 */
void 
install_syscall(){
    SET_IDT_ENTRY(idt[0x80],syscall_entry_ptr);
    idt[0x80].dpl=3; /* set dpl = 3 to allow user to access system call */
    syscall_table[SYS_CLOSE]=(uint32_t)close;
    syscall_table[SYS_HALT]=(uint32_t)halt;
    syscall_table[SYS_EXECUTE]=(uint32_t)execute;
    syscall_table[SYS_GETARGS]=(uint32_t)getargs;
    syscall_table[SYS_READ]=(uint32_t)read;
    syscall_table[SYS_WRITE]=(uint32_t)write;
    syscall_table[SYS_OPEN]=(uint32_t)open;
    syscall_table[SYS_CLOSE]=(uint32_t)close;
    syscall_table[SYS_VIDMAP]=(uint32_t)vidmap;
    syscall_table[SYS_SET_HANDLER]=(uint32_t)set_handler;
    syscall_table[SYS_SIGRETURN]=(uint32_t)sigreturn;
}

