# Bug Log for ECE 391 MP3

| Stat                                                | Bug No.                      |
| --------------------------------------------------- | ---------------------------- |
| :white_check_mark:                                  | `000`                        |
| :white_check_mark:                                  | `001`                        |
| :white_check_mark:                                  | `002`                        |
| :white_check_mark:                                  | `003`                        |
| :white_check_mark:                                  | `004`                        |
| :white_check_mark:                                  | `005`                        |
| :white_check_mark:                                  | `006`                        |
| :white_check_mark:                                  | `007`                        |
| :white_check_mark:                                  | `008`                        |
| :white_check_mark:                                  | `009`                        |
| :white_check_mark:                                  | `010`                        |
| :white_check_mark:                                  | `011`                        |
| :white_check_mark:                                  | `012`                        |
| :white_check_mark:                                  | `013`                        |
| :white_check_mark:                                  | `014`                        |
| :white_check_mark:                                  | `015`                        |
| :white_check_mark:                                  | `014`                        |
| :white_check_mark:                                  | `015`                        |
| :white_check_mark:                                  | `016`                        |
| :white_check_mark:                                  | `017`                        |
| :white_check_mark:                                  | `018`                        |
| :white_check_mark:                                  | `019`                        |
| :white_check_mark:                                  | `020`                        |
| :white_check_mark:                                  | `021`                        |
| :white_check_mark:                                  | `022`                        |
| This table is too much work to maintain after cp5   | abandon this table after cp5 |
| Afterwards, bugs are sorted w.r.t. their categories | No table is needed           |
|                                                     |                              |

### Bug `#000`
**Description**  

Boot loop while code is correct

**Resolution**  

* mp3.img corruption.
* Use git to revert mp3.img, recommend vscode plugin. 



### Bug `#001`

**Description**  

Assembly linkage failed for exception

**Resolution**  

* Processor didn't put exception index onto stack

### Bug `#002`

**Description**  

IDT initialization error

**Resolution**  

* Should use `$KERNEL_CS` instead of `KERNEL_CS` directly.
* Dereference defined value error

### Bug `#003`

**Description**  

C function doesn't recognize S file label/variable

**Resolution**  

* Use `#ifndef ASM` to declare label/variable in header file.

### Bug `#004` 

**Description**  

Double fault when using **assembly linkage**

**Resolution**  

* `pushal` should be looked up instead of following the slides
* Exception index isn't pushed by processor.
* Push myself instead

### Bug `#005`

**Description**  

Manual pushing exception index, without popping.

Ok if you just have a while loop

But I just record it, I will fix it when adding processes.

### Bug `#006` 

**Description**  

Kernel immediately crashed after paging is enabled. Used `gdb` to step through
in instruction level. No obvious software level exception occurred. Incorrect PDE/PTE
value suspected.

**Resolution**  

After careful examination, the physical address of the second 4-MB page is OR'ed 
to the PDE without proper offset, overwriting flag area and eventually causing 
paging to breakdown in hardware level. Added offset to the physical address.

#### Bug `#007` 

**Description**

Page fault when rtc handler

**Resolution**  

* Stop **all global + static variable**
* Otherwise to put them in kernel code section

### Bug `#008` 

**Description**

All Handlers executed with page fault with certain probability

**Resolution**  

* Misuse of STI
* Don't use sti when there are no sti called before

### Bug `#009` 

**Description**

Read inode number error

**Resolution**  

* Revise inode meaning in filesystem property

### Bug `#010` 

**Description**

Wrong value when reading a file

**Resolution**  

* Forget to add offset for file system when trying to access datablock

* Revise formulae for address.

### Bug `#011`

 **Description**

Clear screen while loosing key focus

**Resolution**  

* didn't send eoi after branching in keyboard handler

### Bug `#012`

 **Description**

When file index out-of-bound, it doesn't return -1 immediately

**Resolution**  

* Use directory number instead of inode number to do sanity check.

### Bug `#013`

**Description**

Divide by zero in RTC

**Resolution**  

* Use int32_t as frequency type

### Bug `#014`

**Description**

The keyboard is pressed but no character is printed on the screen.

**Resolution**  

* Always call `send_eoi` after handling a keyboard interrupt!

### Bug `#015`

**Description**

When a *non-special* key (e.g., A) is pressed, `terminal_read` reports a 2-character read. After careful examination of the content of `input` buffer, the 2-character read consists of the original character (e.g., an `'A'`) *and* a NUL character (displayed as a space on the screen). 
A NUL character may have been sent by the `kgetc` function after handling the key release interrupt.

**Resolution**  

* Filter out all the NUL character in the keyboard handler while loop.

### Bug `#014`

**Description**

Why, everytime I switch from user program to kernel system call linkage, IF=0? I explicitly or eflags with 0x200 for IRET. But when it switches back to kernel with system call, IF is somehow cleared, can anyone explain this?

- I even have tss.eflags|=0x200 everytime I execute program.
- I have eflags with eflgas|=0x200 as parameter for IRET
- I checked, in user level program, at the start of main, I have IF set correctly.

**Resolution**  

My brute force fix, is to `sti` when enter system call, and recover flags when out, but I don’t think this is a good fix.

### Bug `#015`

**Description**

PID index out of bound

**Resolution**  

Base address of PID is larger than PID top.

### Bug `#016`

**Description**

Can't run multi-processing

**Resolution**  

* Bad sanity check

### Bug `#017`

**Description**

Always return -1 on bad result

**Resolution**  

* Add error number in `err.c` `err.h`

#### Bug `#0183`

**Description**

getarg only one arg

**Resolution**  

* stop checking `\0`

### Bug `#019`

**Description**

getarg no stripping

**Resolution**  

* strip args

### Bug `#020`

**Description**

Open file number error.

**Resolution**  

* search the open file with constant 8 size

### Bug `#021`

**Description**

STDIN/STDOUT no bad call

**Resolution**  

* add bad call

### Bug `#022`

**Description**

system call (open/write/read/close). Boundary problem

**Resolution**  

* Add sanity check

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



### TS#5

#### Description

* Spamming terminal while running pingpong has 1/10 chance twisting the output flow to wrong shell

#### Solution

* Program output too fast, after terminal switching, PIT interrupt hasn't occurred, program output first
* If current program is running, and you do a terminal switch, re-direct the output flow, while not changing the screen_x, screen_y

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

### SIG#1

#### Description

* when you got interrupted from kernel, sometimes it will break the signal handling

#### Solution

* processor doesn't push ESP&SS when in this context
* add special case checking

### SIG#2

#### Description

* Page fault before entering handler

### Solution

* Bad code copying to user stack, for code isn't aligned
* Not adding NOP (don't know how)
* Change copy size to 8bits each time

### SIG#3

#### Description

* Page fault after sighandler

#### Solution

* Re-calculate stack offset

### SIG#4

#### Description

* Page fault in sigreturn

#### Solution

* Re-calculate stack offset in sigreturn

### SIG#5

* Page fault after sigreturn
* sigtest failed

### Solution

* bad eax position
* swap eax before entering stack

