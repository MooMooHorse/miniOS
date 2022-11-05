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

/**
 * @brief TODO
 * 
 * @param status 
 * @return ** int32_t 
 */
int32_t halt (uint8_t status){
    uint32_t pid=get_pid();
    if(-1==discard_proc(pid,status)){
        printf("discard process error\n");
        return -1; /* errono to be defined */
    }


    /* errono to be defined */
    return -1; /* If you reach here, you fail halt */ 
}
/** TODO sanity check 
 * @brief currently only support command without parameters
 * 
 * @param command - executable file name 
 * @return ** int32_t 
 */
int32_t execute (const uint8_t* command){
    uint8_t _command[CMD_MAX_LEN]; /* move user level data to kernel space */
    uint32_t pid,ppid,i;
    int32_t ret;
    if(strlen(command)>=CMD_MAX_LEN){
        printf("command too long\n");
        return -1;
    }
    strcpy((int8_t*)_command,(int8_t*)command);

    /* Attemp to find new pid, if none of in-stack pcb is availabe, allocate space for it */
    pid=(PCB_BASE-PCB_ptr)/PCB_SIZE+1; 
    for(i=pid-1;i>=1;i--){
        pcb_t* _pcb_ptr=(pcb_t*)(PCB_BASE-i*PCB_SIZE);
        if(0==_pcb_ptr->active){
            pid=i;
            break;
        }
    }
    

    /* check if this file is a program (NULL check included) : TODO */

    ppid=get_pid();

    /* program is under PCB base */
    open_page((pid-1)*PROG_SIZE+PCB_BASE); /* assume never fail, TLB flushed */

    /* program loader */
    if(readonly_fs.load_prog(_command,VPROG_START_ADDR,PROG_SIZE)==-1){
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
    if(pcb_open(ppid,pid,_command)==-1){
        printf("not enough space for PCB\n");
        return -1; /* errono to be defined */
    }
    /* set up TSS, only esp0 is needed to be modified */
    setup_tss(pid);
    /* iret */
    ret=switch_user(pid);
    if(ret==0){
        printf("Your last program exits normally with return value 0\n");
    }
    else{
        printf("Your last program exits with return value %d\n",ret);
    }
    return ret;

}
int32_t read (uint32_t fd, void* buf, uint32_t nbytes){
    fd_t* fd_entry;
    sti();
    if((fd_entry=get_fd_entry(fd))==NULL){
        printf("invalid file descriptor\n");
        return -1; /* errono to be defined */
    }
    return fd_entry->file_operation_jump_table.read(fd_entry,buf,nbytes);
}
int32_t write (uint32_t fd, const void* buf, uint32_t nbytes){
    fd_t* fd_entry;
    sti();
    if((fd_entry=get_fd_entry(fd))==NULL){
        printf("invalid file descriptor\n");
        return -1; /* errono to be defined */
    }
    return fd_entry->file_operation_jump_table.write(fd_entry,buf,nbytes);
    
}

int32_t open (const uint8_t* filename){
    /* terminal is opened in exec, not in this system call */
    fd_t* fd_entry;
    uint32_t pid=get_pid(),i,fdnum;
    uint8_t if_fd_empty=0;
    pcb_t* _pcb_ptr=(pcb_t*)(PCB_BASE-pid*PCB_SIZE);
    /* search for all file descriptors, trying to find one that is not occupied */
    for(i=0;i<_pcb_ptr->fdnum;i++){
        if(!(_pcb_ptr->fd_entry[i].flags&F_OPEN)){
            if((fd_entry=get_fd_entry(fdnum=i))==NULL){
                printf("invalid file descriptor\n");
                return -1; /* errono to be defined */
            }
            if_fd_empty=1;
            break;
        }
    }
    if(!if_fd_empty){
        fdnum=_pcb_ptr->fdnum++;
        if((fd_entry=get_fd_entry(fdnum))==NULL){
            printf("invalid file descriptor\n");
            return -1; /* errono to be defined */
        }
    }
    if(strncmp((int8_t*)"RTC",(int8_t*)filename,4)==0){
        if(rtc[0].ioctl.open(fd_entry,filename,2)==-1){
            return -1;
        }
    }
    else{
        if(readonly_fs.openr(fd_entry,filename,0)==-1){
            return -1;
        }
    }
    return fdnum;
}
int32_t close (uint32_t fd){
    fd_t* fd_entry;
    fd_entry=get_fd_entry(fd);
    return fd_entry->file_operation_jump_table.close(fd_entry);
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

