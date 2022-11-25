# Check Point 5

This document is to record the implementation details so each one in the group knows how one feature is implemented.

## Terminal Switching

* Add buffers to several terminal
* Create/Modify/Update old/new(from/to) buffer when switching terminal
* Background process and Foreground process should write to different buffer

### Components of each terminal

* Video memory
* An input buffer 
* cursor coordinates

### Global Properties

* cursor coordinate
* video memory pointer 
* video memory

### Divide and Conquer Terminal Properties & Global properties

* Terminal switch only change displayed video memory and terminal cursor coordinate
* Context switch change global properties and load them into terminal properties

### Add Buffer to several terminals

* Use several 4KB pages

#### Boot-time allocation/Run-time allocation

* Boot-time -> more stable 

### Vidmap

* You should also update the mapping relation for vidmap whenver you do
  * context switch

### Terminal Buffer : Real Time?

* No, sometimes it can lag
* Only THE video memory will be updated real time
* Terminal buffer only update real time when it's not activated terminal

### Terminal buffer vs Cursor Coordinate

* They're not at the same level
* Terminal buffer don't have to be flushed during context switch, Cursor coordinate should
  * By flushing terminal buffer, I mean copy video memory into terminal buffer, it requires explicit copying
  * By flushing Cursor coordinate, I mean update cursor to the position next `putc` goes into

## Scheduling

* change the state of current process
* update terminal for old process
* find an runnable process
* change the global setting to the new process's display
* change the vidmap
* set up TSS for switched process
* modify esp, ebp to next process
* when you return, you automatically go to the next process (because you've changed esp, ebp)

### change the state of current process

* RUNNABLE
* RUNNING
* SLEEPING
* UNUSED

### update terminal for old process

Store the global setting to the old process's terminal

* terminal.screen_x,screen_y

### find an runnable process

* Make current process the last

```c
//code is written by Brant
for (i = 1; i <= PCB_MAX; ++i) {
        p = PCB((pid - 1 + i) % PCB_MAX + 1);
        if (RUNNABLE == p->state) {
            break;
        }
    }
    
pid = (pid - 1 + i) % PCB_MAX + 1;  // Commit update of `pid`.
```

### change the global setting to the new process's display

* new screen_x, screen_y, vid_mem

### change the vidmap

* paging setting
  * old mapped to terminal buffer
  * new mapped to corresponding memory location

### set up TSS for switched process

* check `SC#2`

### modify esp, ebp to next process

* You either do this, or you write an iret assembly code snippet, your choice

### when you return, you automatically go to the next process 

* because you've changed `esp, ebp`

## Bugs 

### TS#1

#### Description

* cursor coordinate becomes abnormal when doing context switch quickly

#### Solution

* Terminal switch only change displayed video memory and terminal cursor coordinate
* Context switch change global properties and load them into terminal properties

### TS#2

#### Description

* cursor coordinate becomes laggy and fixed at certain place when doing context switch 

#### Solution

* Terminal switch still changes the cursor coordinate globally

### TS#3

#### Description

* terminal switch boom when pressing Fn+# with #>3

#### Solution

* ban the illegal keypress

### TS#4

#### Description

* old buffer overflow problem

#### Solution

* original buffer length type is `uint8_t`, changing it larger will do the work

### SC#1

#### Description

* Page Fault when switching between base shells

#### Solution

* base shell is first switched to then switch to other process.
* we assume that each process that is switched to has been switched to by other process
* so we need to fake it as if there is interrupt linkage wrapper for base shell so we can have good stack to do iret

### SC#2

#### Description

* Page Fault when switching between 4-rd program

#### Solution

* bad TSS value
* TSS value should always be the bottom of the stack
* the residue on stack is saved by process that is switched out, meaning before the process is scheduled again, it is never ran, so there will be nothing that can contaminate the stack values of the process being switched out.
* As for the process that is running, it will always have an empty stack on entry because 
  * the first time we enter the process in one scheduling period, it has empty stack because of PIT assembly linkage 
  * any system call assembly linkage will clear the stack to the state where we enter the system call linkage
  * when the system call linkage is entered, the stack is empty
* So TSS should be set to an empty value

### SC#3

#### Description

* fish failed

#### Solution

* check condition when remap page for new process

### SC#4

#### Description

* When switch fast, kernel double fault

#### Solution

* bad shared variable with multi-terminal
* re-write terminal (I have a terrible afternoon because of this) so it is independent of scheduler
