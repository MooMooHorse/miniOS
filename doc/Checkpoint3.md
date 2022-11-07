# Checkpoint 3

This document helps each member of the group knows the ideas of what other people are doing, so that we're all on the same page.

* General workflow
  * System call oriented
  * Exec -> Halt -> Open/Write/Read/Close

## Open Space for Task (Process)

### Turn on the global Kernel Code 

> A global page directory entry with its **Supervisor** bit set should be set up to map the kernel to virtual address 0x400000 (4 MB). This ensures that the kernel, which is linked to run with its starting address at 4 MB, will continue to work even after paging is turned on.
>
> Appendix C

### Paging - virtual memory management - Every Exec() and Every Halt() - Remember Flush TBL

> As in Linux, the tasks will share common mappings for kernel pages, in this case a single, global 4 MB page. Unlike Linux, we will provide you with set physical addresses for the images of the two tasks, and will stipulate that they require no more than 4 MB each, so you need only allocate a single page for each task’s user-level memory. See Appendix C for more details.
>
> MP3 cp3.2 document

* Only 2 tasks will be created for now, and we statically allocate memory for them

### Loading - Filesystem functionality - Every changing Paging will accompany a load

Move the program image into the right space.

* Note that in Appendix C program image 

> The way to get this working is to set up a single 4 MB page directory entry that maps virtual address 0x08000000 (128 MB) to the right physical memory address (either 8 MB or 12 MB). Then, the program image must be copied to the correct offset (0x00048000) within that page.
>
> Appendix C 

### User Stack, Kernel Stack

* The space is there (allocated)
* By allocating a stack, I mean, assign value to stack pointer

> The final bit of per-task state that needs to be allocated is a kernel stack for each user-level program. Since you only need to support two tasks, you may simply place the first task’s kernel stack at the bottom of the 4 MB kernel page that you have already allocated. The second task’s stack can then go 8 kB above it.
>
> MP3 cp3.5
>
>  Finally, you need to set up a user-level stack for the process. For simplicity, you may simply set the stack pointer to the bottom of the 4 MB page already holding the executable image
>
> MP3 cp3.4

### PCB

* We insist to use 2 **statically** allocated PCB for stack, to make sure the 16 MB memory are not occupied by any other kernel stuff.
* The rest will be allocated dynamically with a pointer pointing at the top of PCB

> Each process’s PCB should be stored at the top of its 8 kB stack, and the stack should grow towards them. Since you’ve put both stacks inside the 4 MB page, there is no need to “allocate” memory for the process control block. To get at each process’s PCB, you need only AND the process’s ESP register with an appropriate bit mask to reach the top of its 8 kB kernel stack, which is the start of its PCB.
>
> MP3 cp3.5

## Execute the Task (Switch to user mode)

### Set up IRET arguments in kernel stack

All the citation in this section is from cp3.4

* Don't confuse ESP and SS
* SS is segment register which selects segment, in Linux, it should point to an entry with base=0 and other stuff set to satisfy user code condition

#### ESP (user stack)

> Finally, you need to set up a user-level stack for the process. For simplicity, you may simply set the stack pointer to the bottom of the 4 MB page already holding the executable image. 

#### EIP (entry of executable)

> The EIP you need to jump to is the entry point from bytes 24-27 of the executable that you have just loaded. 

#### CS and SS (User mode code)

* USER_CS
* RPL = 0

### Modify TSS

> Note that when you start a new process, just before you switch to that process and start executing its user-level code, you must alter the TSS entry to contain its new kernel-mode stack pointer
>
> Appendix E

* ESP0
* SS0



### IRET (Init_task)

* In Init Task : Execute Shell



## Execute another task

* execute 



## Return to parent process

* halt system call
* PCB

> Finally, when a new task is started with the execute system call, you’ll need to store the parent task’s PCB pointer in the child task’s PCB so that when the child program calls halt, you are able to return control to the parent task.



## Rest of System Call

* Once you have PCB, you can allocate file descriptor table in it
* Then it will be really easy to finish system call

> Each process’s PCB should be stored at the top of its 8 kB stack, and the stack should grow towards them.

## How can Segmentation protect user from accessing Kernel Memory

* No it can't, paging does this.
