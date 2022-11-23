#include "signal.h"
#include "process.h"
#include "lib.h"
#include "syscall.h"

static void sigkill_handler (int32_t signum);
static void sigignore_handler(int32_t signum);

sig_handler_t sig_table[NUM_SIGNALS]={
    {.handler=sigkill_handler,.user_space=0},
    {.handler=sigkill_handler,.user_space=0},
    {.handler=sigkill_handler,.user_space=0},
    {.handler=sigignore_handler,.user_space=0},
    {.handler=sigignore_handler,.user_space=0}
};
char* signame[NUM_SIGNALS]={
    "DIV_ZERO",
	"SEGFAULT",
	"INTERRUPT",
	"ALARM",
	"USER1"
};

/**
 * @brief copy items from kernel stack to user stack
 * Internally used
 * @param kesp - pointer to kernel stack
 * @param uesp - pointer to user stack
 * @return ** uint32_t user stack pointer 
 */
static uint32_t 
copy_to_user_stack(uint32_t* kesp,uint32_t* uesp){
    int32_t counter=8+5; /* 8 : saved register - 5 : IRET parameters */ 
    uint32_t swap;
    kesp=kesp+counter;
    while(counter--){
        (*(--uesp))=(*(--kesp));/* pointer arithmetic : each time 32 bits */
    }
    /* swap eax to 7-th register : will recover in sigreturn */
    swap=*(uesp+7);
    *(uesp+7)=*(uesp+6);
    *(uesp+6)=swap;
    /* two addresses here should be always valid */
    return (uint32_t)uesp;
}

/**
 * @brief copy program from prog_start to prog_end onto user stack
 * @param uesp - user stack top pointer
 * @param prog_start - starting address of executing sigreturn assembly code
 * @param prog_end - ending address of executing sigreturn assembly code
 * @return ** uint32_t user stack pointer 
 */
static uint32_t
copy_prog(uint8_t* uesp,uint8_t* prog_start,uint8_t* prog_end){
    uint8_t* i=prog_end; /* pointer arithmetic : each time one instruction */
    while((uint32_t)i>(uint32_t)prog_start){
        *(--uesp)=*(--i); /* pointer arithmetic : each time one instruction */
    }
    return (uint32_t)uesp;
}

/**
 * @brief push return address (entry to assembly linkage for sigreturn)
 * and signal num
 * to the top of user stack
 * @param ret_addr - return address (entry to assembly linkage for sigreturn)
 * @param uesp - user stack top pointer
 * @param signum - signal number
 * @return ** uint32_t user stack pointer 
 */
static uint32_t
push_ret_addr(uint32_t ret_addr,uint32_t* uesp,uint32_t signum){
    (*(--uesp))=signum;
    (*(--uesp))=ret_addr;
    return (uint32_t)uesp;
}


/**
 * @brief set-up kernel stack to enter signal handler
 * @param handler - change eip of kernel stack to this value
 * @param kesp 
 * @param uesp 
 * @return ** void 
 */
static void 
set_kernel_stack(sig_handler_t handler,uint32_t* kesp,uint32_t *uesp){
    /* 
    * move the stack to original version
    * turn off interrupt for signal handler 
    * set the user stack for signal handler 
    * iret to signal handler 
    */

    set_proc_signal(-1); /* remove the currnet signal */

    asm volatile("              \n\
    movl    %%ecx,%%esp         \n\
    addl    $32,%%esp           \n\
                                \n\
    movl    %%eax,(%%esp)       \n\
                                \n\
    pushfl                      \n\
    popl    %%edx               \n\
    orl     $0x0200, %%edx      \n\
    movl    %%edx,8(%%esp)      \n\
                                \n\
    movl    %%ebx,12(%%esp)     \n\
    iret                        \n\
    "
    :
    :"a"(handler.handler),"b"(uesp),"c"(kesp)
    );
    
}

/**
 * @brief handle signal 
 * 
 * @param kesp - pointer to kernel stack
 * @param uesp - pointer to user stack
 * @param prog_start - starting address of executing sigreturn assembly code
 * @param prog_end - ending address of executing sigreturn assembly code
 * @return ** int32_t 
 */
int32_t 
do_signal(uint32_t kesp,uint32_t uesp,uint8_t* prog_start,uint8_t* prog_end){
    uint32_t ret_addr;
    uint32_t pid=get_pid();
    pcb_t*   _pcb_ptr=(pcb_t*)(PCB_BASE-pid*PCB_SIZE);
    if(pid==0||_pcb_ptr->sig_num==-1||sig_table[_pcb_ptr->sig_num].handler==NULL){
        /* no signal, change back to original stack */
        asm volatile("              \n\
        movl    %%ecx,%%esp         \n\
        popal                       \n\
        iret                        \n\
        "
        :
        :"c"(kesp)
        );
        printf("double fault in do_signal\n");
        while(1);
    }
    

    if(!sig_table[_pcb_ptr->sig_num].user_space){
        sig_table[_pcb_ptr->sig_num].handler(_pcb_ptr->sig_num);
        asm volatile("              \n\
        movl    %%ecx,%%esp         \n\
        popal                       \n\
        iret                        \n\
        "
        :
        :"c"(kesp)
        );
        while(1);
    }

    


    /* note that because intruction are not 32-bit aligned, uesp will get pretty ugly */
    uesp=copy_prog((uint8_t*)uesp,prog_start,prog_end); 
    ret_addr=uesp;/* start of execution of sigreturn in user stack */
    
    uesp=copy_to_user_stack((uint32_t*)kesp,(uint32_t*)uesp); /* copy kernel stack to user stack */
    
    /* push return address */
    uesp=push_ret_addr(ret_addr,(uint32_t*)uesp,_pcb_ptr->sig_num);

    /* modify kernel stack to enter signal handler */
    set_kernel_stack(sig_table[_pcb_ptr->sig_num],(uint32_t*)kesp,(uint32_t*)uesp); 

    /* the following part of code will never be accessed */
    printf("signal entry error\n");
    while(1);
    return -1;

}

/**
 * @brief install handler by user
 * 
 * @param signum - signal number 
 * @param handler_address - handler address
 * @return ** int32_t -1 on illegal signum
 * success : 0
 */
int32_t 
install_sighandler(uint32_t signum, void* handler_address){
    if(signum<0||signum>NUM_SIGNALS){
        return -1;
    }
    sig_table[signum].handler=handler_address;
    sig_table[signum].user_space=1; /* specify it is not default handler */
    return 0;
}


/**
 * @brief set signal number for current process
 * @param signum - signal number
 * @return ** int32_t -1 on failure
 */
int32_t 
set_proc_signal(int32_t signum){
    uint32_t pid=get_pid();
    pcb_t*   _pcb_ptr=(pcb_t*)(PCB_BASE-pid*PCB_SIZE);
    /* sanity check */
    if(signum!=-1&&(signum<0||signum>=NUM_SIGNALS)) return -1;
    if(pid<0||pid>PCB_MAX) return -1;

    _pcb_ptr->sig_num=signum;
    return 0;
}


/**
 * @brief set signal number for given process 
 * 
 * @param signum - signal number
 * @param pid - set pid
 * @return ** int32_t -1 on failure
 */
int32_t 
set_async_signal(int32_t signum,int32_t pid){
    pcb_t*   _pcb_ptr=(pcb_t*)(PCB_BASE-pid*PCB_SIZE);
    /* sanity check */
    if(signum!=-1&&(signum<0||signum>=NUM_SIGNALS)) return -1;
    if(pid<0||pid>PCB_MAX) return -1;

    _pcb_ptr->sig_num=signum;
    return 0;
}


/**
 * @brief default signal handler for ALARM and USER1 signal
 * ignore signal
 * @param signum - signal number
 * @return ** void 
 */
static void
sigignore_handler(int32_t signum){
    printf("Program recieve signal %s\n",signame[signum]);
    set_proc_signal(-1); /* remove the signal from this process */
    return;
}


/**
 * @brief default signal handler for DIV_ZERO, SEGFAULT, INTERRUPT signals
 * kill the task
 * @param signum - signal number
 * @return ** void 
 */
static void
sigkill_handler (int32_t signum){
    printf("Program recieve signal %s\n",signame[signum]);
    set_proc_signal(-1);
    halt(0);
    return ;
}

