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
#include "mmu.h"

/**
 * @brief TODO
 * 
 * @param status 
 * @return ** int32_t 
 */
int32_t halt (uint8_t status){
    printf("start halting, but I haven't finished it yet\n");
    while(1);
    return 0;
}
/** TODO sanity check 
 * @brief currently only support command without parameters
 * 
 * @param command - executable file name 
 * @return ** int32_t 
 */
int32_t execute (const uint8_t* command){
    uint32_t pid=(PCB_ptr-PCB_BASE)/PCB_SIZE;
    /* check if this file is a program (NULL check included) : TODO */


    /* program is under PCB base */
    open_page(pid*PROG_SIZE+PCB_BASE); /* assume never fail, TLB flushed */
    /* program loader */
    if(readonly_fs.load_prog(command,VPROG_START_ADDR,PROG_SIZE)==-1){
        printf("illegal command\n");
        return -1; /* errono to be defined */
    }

    /* check if executable has magic number starting from VPROG_START_ADDR : TO DO*/
    
    /* create PCB */
    if(pcb_create(pid)==-1){
        printf("illegal pid\n");
        return -1; /* errono to be defined */
    }
    /* open PCB */
    if(pcb_open(pid,command)==-1){
        printf("not enough space for PCB\n");
        return -1; /* errono to be defined */
    }
    /* set up TSS, only esp0 is needed to be modified */
    setup_tss();
    /* iret */
    switch_user();

    return 0;

}
int32_t read (uint32_t fd, void* buf, uint32_t nbytes){
    fd_t* fd_entry;
    if((fd_entry=get_fd_entry(fd))==NULL){
        printf("invalid file descriptor\n");
        return -1; /* errono to be defined */
    }
    return fd_entry->file_operation_jump_table.read(fd_entry,buf,nbytes);
}
int32_t write (uint32_t fd, const void* buf, uint32_t nbytes){
    fd_t* fd_entry;
    if((fd_entry=get_fd_entry(fd))==NULL){
        printf("invalid file descriptor\n");
        return -1; /* errono to be defined */
    }
    return fd_entry->file_operation_jump_table.write(fd_entry,buf,nbytes);
    
}

int32_t open (const uint8_t* filename){
    return 0;
}
int32_t close (uint32_t fd){
    return 0;
}
int32_t getargs (uint8_t* buf, uint32_t nbytes){
    return 0;
}
int32_t vidmap (uint8_t** screen_start){
    return 0;
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

