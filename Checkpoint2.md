# Checkpoint 2

This document helps each member of the group knows the ideas of what other people are doing, so that we're all on the same page.



## What is driver

e.g. Terminal

* Module (keyboard and screen)

* Ioctl (driver) - **file descriptor table**
* Operating system (Userlevel program, kernel code)

## Everything is a file

* Advantage1 : Namespace : allow for common function name being re-used
* Advantage2 : Compatibility : Minimize changes for other parts of code 
* Advantage3 : Virtualization : Allowing one device to be as multiple devices : Instead of changing 

## Stop magic number, Use define and property

* property : define variables in struct to set a feature of that instances
* define  : hard coded

## How to write a driver?

This section describes how to write a driver.

### Encapsulation 

* Function into struct
* *Global variable into struct
* Object Oriented Programming

* Do it in 1st way or 2nd way

```c
/* You can implement driver like this */
typedef struct filesystem_jump_table{
    int32_t (*read_dentry_by_name) (const uint8_t*, dentry_t*);
	int32_t (*read_dentry_by_index) (uint32_t, dentry_t*);
	int32_t (*read_data) (uint32_t, uint32_t, uint8_t*, uint32_t);
    int32_t (*fake_write) (void);
} fsjmp_t;
/* Advantage1 : Namespace : allow for common function name being re-used */
/* Advantage2 : Compatibility : Minimize changes for other parts of code */
/* Advantage3 : Virtualization : Allowing one device to be as multiple devices : Instead of changing devices, change the driver */
/* If one tries to intereact through the driver in another way, */
/* this interface doesn't change, you just re-install functions */
/* Other parts of code always calls your driver through this function, */
/* so other parts of code don't have to be changed */
typedef struct filesystem{
    fsjmp_t f_rw; // 1st way
    int32_t (*open_fs)(void);
    // int32_t (*ioctl)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t); // 2nd way depending on the context
    int32_t (*close_fs)(void);
    /* A series of shared variables you might want to make use of */
    uint32_t file_num; 
    uint32_t r_times,w_times;
    //...
} fs_t;


/* Initialization for Fileystem */
static int32_t open_fs(){
    // load all functions into struct
    
    // initialize all shared variables
}
/* Tear down File System */
static int32_t close_fs(){
    // Unload all functions to struct
    
    // erase all shared variables
}

/* A series of static functions you need for ioctl */


```



## File System

### Find the right address for filesystem

* In `kernel.c`, it's really easy to deduce the module being installed is file system, so we can track filesystem with
  * `mod->mod_start` as starting address
  * `mod->mod_end` as ending address

### Module Code

* Specific implementation
* Called by `ioctl` function
* Recommendation : Write subroutine under module functions

### Ioctl Code, File Descriptor

* Your ioctl functions should be installed in both **Module** and **File Operation Jump Table in File Descriptor**
* Therefore your ioctl functions should be in public and in the same format as in jumptable
* Ioctl code should make use of module code
* Open should be responsible for **initializing descriptor**
* Open should be installed in both jumptable and **directly in module** (Because you're installing yourself, if you can't be called, who install you?)

### System Call

* In open, system call look at flags, and decide which open it should use
  * therefore, the jump table of correct type will be installed
* In read/write/close, since jump table of correct type is installed, it will automatically use the right functions.

## RTC Driver

* Interact with virtualized RTC
* RTC is now virtualized as a file in system
  * Also implemented by Module - ioctl - system (three layer structure)

* **But the file isn't stored in our file system**
  * so in system call open, we will see if you're opening RTC, then we will install the corresponding jump table to file descriptor table. (So make sure you understand Appendix B before you program for any driver, because everything in Linux is essentially a file).
  * This urges you to have "**file content**" implemented as field in Module struct.

* No doc is needed, because init and handler function is statically installed (without encapsulation)

## Terminal

* Keyboard and Screen is just one pair of devices
* Virtualize it, wrap it into a module
* communicate to it through ioctl
* then you are free in writing other parts of your system code 
  * because you don't have to worry to interrupt or be interrupted by modules with the help of ioctl

### Goals

#### Functionalities

* Pass RTC hint, RTC test coverage in MP3 doc, RTC grade by Saturday 20:00pm with test written

#### Format

* No external variable other than RTC instances
* Fill all the field that is in RTC `open, ioctl, (a series a variables that you define)`
* Know the three layers and implement in that way

