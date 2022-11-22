#include "signal.h"
#include "process.h"
#include "lib.h"

sig_handler_t sig_table[NUM_SIGNALS];

/**
 * @brief copy items from kernel stack to user stack
 * Internally used
 * @param kesp - pointer to kernel stack
 * @param uesp - pointer to user stack
 * @return ** void 
 */
static void 
copy_to_user_stack(uint32_t* kesp,uint32_t* uesp){
    int32_t counter=8+5; /* 8 : saved register - 5 : IRET parameters */ 
    kesp=kesp+counter;
    while(counter--){
        (*(--uesp))=(*(--kesp));/* pointer arithmetic : each time 32 bits */
    }
    /* two addresses here should be always valid */
}

/**
 * @brief copy program from prog_start to prog_end onto user stack
 * @param uesp - user stack top pointer
 * @param prog_start - starting address of executing sigreturn assembly code
 * @param prog_end - ending address of executing sigreturn assembly code
 * @return ** void 
 */
static void
copy_prog(uint32_t** uesp,uint32_t* prog_start,uint32_t* prog_end){
    uint32_t* i=prog_end; /* pointer arithmetic : each time one instruction */
    while((uint32_t)i>(uint32_t)prog_start){
        *(--(*uesp))=*(--i); /* pointer arithmetic : each time one instruction */
    }

}

/**
 * @brief push return address (entry to assembly linkage for sigreturn)
 * and signal num
 * to the top of user stack
 * @param ret_addr - return address (entry to assembly linkage for sigreturn)
 * @param uesp - user stack top pointer
 * @param signum - signal number
 * @return ** void 
 */
static void
push_ret_addr(uint32_t ret_addr,uint32_t* uesp,uint32_t signum){
    (*(--uesp))=signum;
    (*(--uesp))=ret_addr;
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
    asm volatile("              \n\
    movl    %%ecx,%%esp         \n\
    popal                       \n\
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
do_signal(uint32_t kesp,uint32_t uesp,uint32_t* prog_start,uint32_t* prog_end){
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

    copy_prog((uint32_t**)(&uesp),prog_start,prog_end);
    ret_addr=uesp;/* start of execution of sigreturn in user stack */
    
    copy_to_user_stack((uint32_t*)kesp,(uint32_t*)uesp); /* copy kernel stack to user stack */
    
    /* push return address */
    push_ret_addr(ret_addr,(uint32_t*)uesp,_pcb_ptr->sig_num);

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
    return 0;
}


