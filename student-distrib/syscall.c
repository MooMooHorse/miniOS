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
/**
 * @brief sys_call_dispatcher() is called when idt[0x80] is invoked
 * 
 * @return ** void 
 */
void sys_call_dispatcher(){
    printf("system call is invoked\n");
}

/**
 * @brief install system call to idt
 * 
 * @return ** void 
 */
void install_syscall(){
    SET_IDT_ENTRY(idt[0x80],syscall_entry_ptr);
}

