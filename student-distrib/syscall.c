/**
 * @file syscall.c
 * @author haor2
 * @brief Reserved for system call. In cp1, this file DOESN't go to another layer of indirection.
 * Instead, this file just prints out one system call is invoked
 * @version 0.1
 * @date 2022-10-15
 * 
 * 
 * @version 1.0
 * @author haor2
 * @brief support multi-terminal
 * @date 2022-11-19
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
#include "signal.h"
#include "terminal.h"
#include "cursor.h"
#include "keyboard.h"
#include "vga.h"

extern void swtchret(void);
extern void pseudoret(void);


int32_t is_base=1;

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

int32_t _execute(const uint8_t* command,uint32_t pid,uint32_t ppid){
    uint8_t _command[CMD_MAX_LEN]; /* move user level data to kernel space */
    uint32_t esp;
    int32_t ret;
    pcb_t* p = PCB(pid);
    
    /* ret : temporary here, have meaning at the end of execute */
    if((ret=copy_to_command(command,_command,CMD_MAX_LEN))==-1){
        return ERR_NO_CMD;
    }
    

    /* check executable */ 
    if(fs.check_exec(_command)!=1){
        return ERR_NO_CMD;
    }

    /* create PCB */
    if(pcb_create(pid)==-1){
        printf("too many executable\n");
        return ERR_BAD_PID; /* errono to be defined */
    }

    /* set argument for this process :  */
    /* position of this function is IMPORTANT! */
    set_proc_args(command,ret,CMD_MAX_LEN,pid);


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
    fs.load_prog(_command,IMG_START,PROG_SIZE);

    p->state = RUNNABLE;
    p->terminal = pid;

    // Manually construct the context structs for base shell #2 & #3.
    esp = (uint32_t) PCB_BASE - (pid - 1) * PCB_SIZE - 4;
    *(uint32_t*) esp = USER_DS;
    esp -= 4;
    *(uint32_t*) esp = p->uesp;
    esp -= 8;
    *(uint32_t*) esp = USER_CS;
    esp -= 4;
    *(uint32_t*) esp = p->ueip;
    esp -= 4;
    *(uint32_t*) esp = (uint32_t) pseudoret;
    esp -= sizeof(*p->context);
    p->context = (context_t*) esp;
    memset(p->context, 0, sizeof(*p->context));
    p->context->eip = (uint32_t) swtchret;

    return 0;
}

/** 
 * @brief currently only support command without parameters
 * 
 * @param command - executable file name 
 * @return ** int32_t - errno 
 */
int32_t execute (const uint8_t* command){
    uint8_t _command[CMD_MAX_LEN]; /* move user level data to kernel space */
    uint32_t pid=0,ppid;
    int32_t i,ret;
    
    /* ret : temporary here, have meaning at the end of execute */
    if((ret=copy_to_command(command,_command,CMD_MAX_LEN))==-1){
        return ERR_NO_CMD;
    }
    

    /* check executable */ 
    if(fs.check_exec(_command)!=1){
        return ERR_NO_CMD;
    }

    /* Attempt to find new pid, if none of in-stack pcb is availabe, allocate space for it */
    for(i=1;i<=PCB_MAX;i++){
        pcb_t* _pcb_ptr=(pcb_t*)(PCB_BASE-i*PCB_SIZE);
        if(UNUSED==_pcb_ptr->state){
            pid=i;
            break;
        }
    }
    if(pid==0){
        printf("no more pid available\n");
        return 0;
    }


    ppid=(pid<=3)?0:get_pid();

    /* create PCB */
    if(pcb_create(pid)==-1){
        printf("too many executable\n");
        return ERR_BAD_PID; /* errono to be defined */
    }

    /* set argument for this process :  */
    /* position of this function is IMPORTANT! */
    set_proc_args(command,ret,CMD_MAX_LEN,pid);


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
    fs.load_prog(_command,IMG_START,PROG_SIZE);

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
    /* sti(); */

    
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
    if(strncmp((int8_t*)"vga",(int8_t*)filename,4)==0){
        /* if vga */
        if(-1==vga_open(file_entry,filename,0)){
            return -1;
        }
    }
    else if(strncmp((int8_t*)"rtc",(int8_t*)filename,4)==0){
        /* if rtc */
        if(rtc[terminal_index].ioctl.open(file_entry,filename,terminal_index)==-1){
            return -1;
        }
    }
    else{
        /* if regular file */
        if(fs.openr(file_entry,filename,0)==-1){
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

/** 
 * @brief Reads program's arguments from its respective pcb_t into a user-level buffer.
 * 
 * @param buf Pointer to the buffer to be filled with the arguments.
 * @param nbytes Number of bytes to be read.
 * @return int32_t 0 on success, -1 on failure.
 * SIDE EFFECT: buf is filled with the arguments.
 */
int32_t getargs (uint8_t* buf, uint32_t nbytes){
    uint32_t pid;
    pcb_t* _pcb_ptr;
    /* sanity check start */
    if (buf == NULL) return -1; /* no buf */
    // get pointer to pcb_t
    pid = get_pid();
    _pcb_ptr = (pcb_t*)(PCB_BASE - pid * PCB_SIZE);
    if (strlen(_pcb_ptr->args) > nbytes) return -1; /* no enough buf length */
    if (_pcb_ptr->args[0] == '\0') return -1; /* no argument */
    /* sanity check end */


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

/**
 * @brief install handler by user
 * 
 * @param signum - signal number 
 * @param handler_address - handler address
 * @return ** int32_t -1 on illegal signum
 * success : 0
 */
int32_t set_handler (uint32_t signum, void* handler_address){
    if(handler_address==NULL){/* restore default handler */
        set_default_handler(signum);
        return 0;
    }
    return install_sighandler(signum,handler_address);
}
/**
 * @brief recover hardware context stored in user stack to kernel stack and 
 * return to the context before signal is handled
 * @return ** int32_t 
 */
int32_t sigreturn (void){
    uint32_t *uesp,*kebp;
    int32_t counter;
    uint32_t swap;


    /* extract uesp */
    /** kernel stack frame right now
     * local variables
     * old ebp <- ebp
     * ret address 
     * para1~3
     * eflag
     * ------------------ should discard before iret
     * 8regs
     * ret
     * CS
     * eflags
     * uesp
     */
    asm volatile("                \n\
    movl 68(%%ebp),%%ebx          \n\
    movl %%ebp,%%ecx              \n\
    "
    :"=b"(uesp),"=c"(kebp)
    :
    :"memory"
    ); /* from old ebp to uesp 17 * 4 = 68 Bytes */


    /** user stack right now
     * signum <- uesp
     * 8 regs
     * return address
     * CS
     * Eflags
     * old uesp
     * SS
     */

    kebp=kebp+6; /* 6 : from old ebp to 8 regs */
    uesp=uesp+1; /* 1: from signum to 8 regs */
    counter=8+5; /* 8 : 8 regs + 5 : 5 parameters to IRET */

    while(counter--){ /* overwrite kernel stack with user stack */
        (*(kebp++))=(*(uesp++));
    }

    /* move eax back to 8-th */
    swap=*(kebp-7);
    *(kebp-7)=*(kebp-6);
    *(kebp-6)=swap;

    /* discard kernel stack and iret to normal program */
    asm volatile("              \n\
    leave                       \n\
    addl    $20,%%esp           \n\
    popal                       \n\
    iret                        \n\
    "
    :
    :
    );

    return -1;  
}

/**
 * @brief create a new file
 * 
 * @param fname - filename to create
 * @return ** int32_t - 0 on success
 * -1 on failure
 */
int32_t file_create(const uint8_t* fname){
    return fs.f_rw.create_file(fname,strlen((int8_t*)fname));
}

/**
 * @brief delete a file
 * 
 * @param fname - file name to delete 
 * @return ** int32_t - 0 on success
 * -1 on failure
 */
int32_t file_remove(const uint8_t* fname){
    return fs.f_rw.remove_file(fname,strlen((int8_t*)fname));
}
/**
 * @brief rename a file from src to dest
 * 
 * @param src - original file name
 * @param dest - changed file name 
 * @return ** int32_t - 0 on success
 * -1 on failure
 */
int32_t file_rename(const uint8_t* src, const uint8_t* dest){
    return fs.f_rw.rename_file(src,dest,strlen((int8_t*)dest));
}

/**
 * @brief get user character
 * 
 * @return ** int32_t user character
 * 0 on not have any 
 */
int32_t getc(){
    return get_c();
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
    syscall_table[SYS_SET_CURSOR]=(uint32_t)set_cursor;
    syscall_table[SYS_GET_CURSOR]=(uint32_t)get_cursor;
    syscall_table[SYS_FILE_CREATE]=(uint32_t)file_create;
    syscall_table[SYS_FILE_REMOVE]=(uint32_t)file_remove;
    syscall_table[SYS_FILE_RENAME]=(uint32_t)file_rename;
    syscall_table[SYS_GETC]=(uint32_t)getc;
}

