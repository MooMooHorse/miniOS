/**
 * @file process.c
 * @author haor2
 * @brief support for execute system call for process related functions.
 * Process related memory are specified in x86_desc.h and defined in x86_desc.S
 * @version 0.2
 * @date 2022-10-31
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "process.h"
#include "filesystem.h"
#include "terminal.h"
#include "lib.h"
#include "syscall.h"
#include "mmu.h"
/**
 * @brief Create PID by modifying PCB_ptr
 * 
 * @param pid - process id starting from 0. 
 * There will be at most 8 processes as stipulated by ECE391 staff. 
 * The memory is "allocated" using PCB_ptr pointer which is a pointer pointed at current running process.
 * PCB_ptr points at the top of the currently running process.
 * @return ** int32_t 0 on success, -1 on failure
 */
int32_t 
pcb_create(uint32_t pid){
    if(pid<=0||pid>8){
        return -1;
    }
    PCB_ptr=PCB_BASE-pid*PCB_SIZE; /* pid index from 0 : PCB base starts from 8 MB */
    return 0;
}


/* lack of open function in terminal.c , have to do in this way */
/** Internal use
 * @brief Open terminal : initialize file descriptor table for terminal
 * 
 * @param _pcb_ptr - pointer to pcb block for current process
 * @param i - index for file descriptor table
 * @return ** int32_t 
 */
static int32_t 
init_fd_entry(pcb_t* _pcb_ptr, int32_t i){
    if(_pcb_ptr==NULL || terminal.ioctl.open(&_pcb_ptr->fd_entry[i],(uint8_t*)"terminal",0)==-1){
        return -1;
    }
    /* for terminal those arguments are gargbage */
    _pcb_ptr->fd_entry[i].file_position=0;
    _pcb_ptr->fd_entry[i].flags=DESCRIPTOR_ENTRY_TERMINAL;
    _pcb_ptr->fd_entry[i].inode=-1; /* won't be used */
    _pcb_ptr->fd_entry[i].file_operation_jump_table.read=terminal.ioctl.read;
    _pcb_ptr->fd_entry[i].file_operation_jump_table.write=terminal.ioctl.write;
    _pcb_ptr->fd_entry[i].file_operation_jump_table.close=terminal.ioctl.close;
    _pcb_ptr->fd_entry[i].file_operation_jump_table.open=terminal.ioctl.open;
    return 0;
}


/**
 * @brief From esp for current kernel stack, we can know who calls the execute program
 * 
 * @return ** uint32_t ppid index ; -1 on failure
 */
uint32_t get_ppid(){
    uint32_t cur_esp;
    if(PCB_ptr==PCB_BASE){
        return 0; /* kernel spawn shell, kernel with "pid" 0*/
    }
    /* any esp in this kernel stack would work */
    asm volatile("            \n\
    movl %%esp,%%ebx          \n\
    "
    :"=b"(cur_esp)
    :
    :"memory"
    );
    if(((cur_esp&(0xf000))>>12)>=16){
        printf("wrong current esp\n");
        return -1;
    }
    return (16-((cur_esp&(0xf000))>>12));
}

/**
 * @brief Open, initialize PCB Block
 * @param pid 
 * @return ** int32_t -1 on failure 0 on success
 */
int32_t 
pcb_open(uint32_t ppid,uint32_t pid,const uint8_t* prog_name){
    if(prog_name==NULL){
        return -1;
    }
    pcb_t* _pcb_ptr=(pcb_t*)PCB_ptr; /* cast pointer to dereference memory */
    _pcb_ptr->pid=pid;
    _pcb_ptr->ppid=ppid;
    _pcb_ptr->active=1;
    _pcb_ptr->fdnum=2; /* STDIN and STDOUT */
    init_fd_entry(_pcb_ptr,0);
    init_fd_entry(_pcb_ptr,1);
    /* following fd entries are not opened yet, not initialized */
    strncpy((int8_t*)_pcb_ptr->pname,(int8_t*)prog_name,32);
    dentry_t dentry;
    uint8_t buf[5];
    /* read the dentry */
    if(readonly_fs.f_rw.read_dentry_by_name(prog_name,&dentry)==-1){
        printf("program doesn't exist\n");
        return -1;
    }
    /* mp3 document : entry in virtual address is specified at 24~27 bytes */
    if(readonly_fs.f_rw.read_data(dentry.inode_num,24,buf,4)==-1){
        printf("bad executable\n");
        return -1;
    }
    /* little Endian : calculate eip for user program */
    _pcb_ptr->ueip=buf[0]+(buf[1]<<8)+(buf[2]<<16)+(buf[3]<<24); 
    /* 128 MB + 4MB - 04 */
    _pcb_ptr->uesp=USR_STACK_PTR;
    return 0;
}

/** Halt to do
 * @brief switch to user mode 
 * Called in exec (and halt possibly)
 * Input : none
 * Output : none
 * @return ** status  
 */
int32_t
switch_user(){
    pcb_t* _pcb_ptr=(pcb_t*)PCB_ptr;
    // printf("%d\n",*(uint32_t*)(_pcb_ptr->eip+10));
    /* EIP, CS, EFLAGS, ESP, SS */
    /* http://jamesmolloy.co.uk/tutorial_html/10.-User%20Mode.html */
    /* Note that we have different value than in that tutorial */
    /* Our user_DS is 0x2B instead of 0x23 */
    /* besides that, we need to explicitly enable interrupt by setting 9-th bit on eflgas */
    /* http://www.c-jump.com/CIS77/ASM/Instructions/I77_0070_eflags_bits.htm */
        // mov     %%ax, %%es     \n
        // mov     %%ax, %%fs     \n
        // mov     %%ax, %%gs     \n
    /* get current ebp */
    asm volatile("            \n\
    movl %%ebp,%%ebx          \n\
    "
    :"=b"(_pcb_ptr->kebp)
    :
    :"memory"
    );
    /* get current esp */
    asm volatile("            \n\
    movl %%esp,%%ebx          \n\
    "
    :"=b"(_pcb_ptr->kesp)
    :
    :"memory"
    );
    _pcb_ptr->cur_PCB_ptr=(uint32_t)_pcb_ptr;
    asm volatile ("            \n\
        andl    $0x00ff, %%eax \n\
        movw    %%ax, %%ds     \n\
                                 \
        pushl   %%eax          \n\
        pushl   %%edx          \n\
                                 \
        pushfl                 \n\
        popl    %%edx          \n\
        orl     $0x0200, %%edx \n\
        pushl   %%edx          \n\
                                 \
        pushl   %%ecx          \n\
        pushl   %%ebx          \n\
        iret                   \n\
        "
        :
        : "a"(USER_DS), "b"(_pcb_ptr->ueip), "c"(USER_CS),"d"(_pcb_ptr->uesp)
        : "memory"
    );
    
    /* if reach here, you fail, return for this function should be done at halt */
    return -1;
}

/** Halt to do
 * @brief Set the up tss for each process
 * Called when exec and halt
 * Halt to do
 * @return ** void 
 */
void
setup_tss(){
    /* ss is set for us */
    /* only need to set up esp0 */
    /* NEVER let this overflow outside the kernel page*/
    tss.esp0=PCB_ptr+PCB_SIZE-0x04; /* you can see it as esp=stack_size-0x04 for each process */
    /* e.g. if you push an element into KERNEL stack, esp-=pushed_size_in_bytes to allocate space */
    /* now it's empty with some garbage because of alignment and the danger mentioned before */
}

/**
 * @brief recover tss to a given process specified by a pointer to pcb
 * @param _pcb_ptr a pointer to current process's pcb
 * @return ** void 
 */
void 
recover_tss(pcb_t* _pcb_ptr){
    tss.esp0=(uint32_t)_pcb_ptr+PCB_SIZE-0x04;
}


/**
 * @brief given file descriptor index, extract corresponding entry from file descriptor array
 * 
 * @param fd - file descriptor index
 * @return ** fd_t* NULL will illegal fd
 */
fd_t* 
get_fd_entry(uint32_t fd){
    pcb_t* _pcb_ptr=(pcb_t*)PCB_ptr;
    if(fd<0||fd>=_pcb_ptr->fdnum) return NULL;
    
    return &(_pcb_ptr->fd_entry[fd]);
}

/**
 * @brief Discard the current process given pid and return status
 * 
 * @param pid - current process index 
 * @param status - return status 
 * @return ** int32_t - return status
 */
int32_t 
discard_proc(uint32_t pid,uint32_t status){
    pcb_t* _pcb_ptr;
    uint32_t ppid;
    uint32_t cur_esp,cur_ebp;
    if(pid<=0||pid>8){
        printf("illegal pid\n");
        return -1;
    }
    _pcb_ptr=(pcb_t*)PCB_ptr; /* old pcb */
    _pcb_ptr->active=0; /* turn off old pcb */
    ppid=_pcb_ptr->ppid; /* get parent pid : pid to recover */
    cur_esp=_pcb_ptr->kesp;
    cur_ebp=_pcb_ptr->kebp;

    /* re-spawn a shell immediately */
    if(ppid==0){ 
        PCB_ptr=PCB_BASE; /* discard all process */
        /* tss and paging isn't necessary here, for execute will help us set them up */
        /* interrupt isn't turned on during this process (process here doesn't mean task) */
        printf("shell respawn\n");
        /* TODO : think about return value here */
        /* enforce shell to boot here, because execute restart the stack, regardless of */
        /* what you leave on stack right now, you don't need recover stack frame now */
        execute("shell");
    }
    else{
        /* TO DO : Print status message here */
        
        PCB_ptr+=PCB_SIZE; /* discard current process */
        _pcb_ptr=(pcb_t*)(PCB_BASE-ppid*PCB_SIZE); /* recover pid */
        recover_tss(_pcb_ptr); /* recover tss */
        open_page(PCB_BASE+(_pcb_ptr->pid-1)*PROG_SIZE); /* re-open last program */
        /* recover stack frame, return in exec() */
        asm volatile("            \n\
        andl $0xff,%%eax          \n\
        movl %%ebx,%%esp          \n\
        movl %%ecx,%%ebp          \n\
        leave                     \n\
        ret                       \n\
        "
        :
        :"a"(status),"b"(cur_esp),"c"(cur_ebp)
        :"memory"
        );
    }
    printf("halt doesn't jump to exec\n");
    return -1;
}


