# CheckPoint 1 : MP3

author : Hao Ren

This document cites important information for checkpoint 1. MP3.

It also gives an outline for checkpoint 1

## GDT, LDT

We start from GRUB.

>  GRUB drops you into protected mode, but with **paging turned off** and all the descriptor tables (**GDT, LDT, IDT, TSS) in an undefined state**. 
>
>  IA-32 Intel® Architecture Software Developer’s Manual - Page 50 ~ 53 (following is paging with will be useful for following sections)
>
>  

Then we want to enable GDT, to makes segmentation possible

* set up a GDT, LDT, and TSS (set by the `boot.S` already, but it seems to miss something)
  * `boot.S` line 27 `load GDT` missing

> GRUB has loaded the file system image at some place in physical memory, as a module
>
> You will need to keep the base address of this file system module around, since many operations will need to interact with the file system. 

* extract boot information
* This will be important in checkpoint 2

> 14.1 Debugging with QEMU

* Use dev to make
* Change the `.lnk` file to certain pattern
* I rename test machine to `mp3_test` 

> When compiling your kernel in MP3, you must first close the test machine. 

* Very important for anyone to debug

> Grading

* interface + comments
  * helps collaboration

> Initialize IDT
>
> IA-32 Intel® Architecture Software Developer’s Manual (Page 156)
>
> https://www.oreilly.com/library/view/understanding-the-linux/0596002130/ch04s04.html

> Appendix D: System Calls, Exceptions, and Interrupts (details for IDT)

* Each IDT entry should be initialized with **all interrupt gates**. It means clear interrupt flag every time interrupt is invoked.



* Exception doesn't push exception code in format of negation of index, instead, there is error code on stack. We choose to **omit that** and do it for processor.



## Paging

We start from segmentation, it's well-document in Intel manual page 73

* >  If paging is not used, the processor maps the linear address directly to a physical address (that is, the linear address goes out on the processor’s address bus).
  >
  > IA-32 Intel® Architecture Software Developer’s Manual Page 73

* We always have base descriptors **0**, to make segmentation trivial.

* > Each system must have one GDT defined, which may be used for all programs and tasks in the system. Optionally, one or more LDTs can be defined. For example, an LDT can be defined for each separate task being run, or some or all tasks can share the same LDT.
  >
  > IA-32 Intel® Architecture Software Developer’s Manual Page 82

* Although above, we're **not doing that**.

* Page 86 explains difference between Segmentation and Paging, really worth reading.

### Before Enable Paging

Now it's time to enable paging : **What will happen to the code that we place before paging is enabled**?

* We map the 0~8 MB linear address directly to 0~8 MB physical address. Nothing changes.

* > To keep things simple, the kernel and video memory will be at the same location in virtual memory as they are in physical memory.
  >
  > MP3 document

### Placing the PDE,PTE in `x86_desc.S`

* Quite intuitive
* Definition  in page 85

* > When the PSE flag in CR4 is set, both 4-MByte pages and page tables for 4-KByte pages can be accessed from the same page directory. If the PSE flag is clear, only page tables for 4-KByte pages can be accessed
  >
  > Page 88

### Processor maintain PDE and PTE in TLB

> The processor maintains 4-MByte page entries and 4-KByte page entries in separate TLBs. So, placing often used code such as the kernel in a large page, frees up 4-KByte-page TLB entries for application programs and tasks.
>
> Page 89

* Downside of using 4MB page : waste space & continuous memory

* Upside : TLB covers more, speed up

### Set Registers to access 4KB 4MB page Mixing

> Table 3-3. Page Sizes and Physical Address Sizes
>
> Page 86

* We're only using the two entries where PSE is set.

### Set PDE and PTE

> Figure 3-14. Format of Page-Directory and Page-Table Entries for 4-KByte Pages and 32-Bit Physical Addresses
>
> Page 90

* Only 20 bytes for base address because **PDE and PTE** are stored in first 20 bits

  * ~~PTE are in first 8 MB~~ Wrong Answer!

  * > The bits in this field are interpreted as the 20 most-significant bits of the physical address, which forces page tables to be aligned on 4-KByte boundaries.

  * And for 4MB it's the same, meaning 4MB page must be aligned to 4MB boundries

  * In other words, the index bits (20 or 10), are just the number for the page table. Like indexing bits 1 means the first page table regardless of the fact that 1 is bit shifted. 



### Flush TLB

> All of the (non-global) TLBs are automatically invalidated any time the CR3 register is loaded
>
> Page 103

* This is not used for cp1, since there are no switching context in cp1



### Turn on Paging by setting CR0~CR4

> Page 54

* PG to enable paging
* WP to facilitate copy on write



### Paging part is finished



## Keyboard

Whew, it's tightly linked to cp2. Well, not as tight as file system and paging, but. It's tightly linked to cp2.

### What are documents that we should look at?

* OS dev keyboard
* OS dev "8042" PS/2 Controller

### "Open the device"

* If the device isn't connect to PIC, how can you use it?
  * send pic a message, `enable_interrupt`
* If the device has some critical setting that you didn't set, how can you use it?
  * Look at  mannual.
  * NULL for keyboard
  * A small bunch for RTC

### Get the keys

#### Single Character

* Write the hanlder

* > A scan code set is a set of codes that determine when a key is pressed or repeated, or released. There are 3 different sets of scan codes. ~~The oldest is "scan code set 1", the default is "scan code set 2", and there is a newer (more complex) "scan code set 3". *Note: Normally on PC compatible systems the keyboard itself uses scan code set 2 and the keyboard controller translates this into scan code set 1 for compatibility. See ["8042"_PS/2_Controller](https://wiki.osdev.org/"8042"_PS/2_Controller#Translation) for more information about this translation.*~~
  >
  > Modern keyboards should support all three scan code sets, however some don't. Scan code set 2 (the default) is the only scan code set that is guaranteed to be supported.
  >
  > https://wiki.osdev.org/PS2_Keyboard#Driver_Model

* We use the scan code set 1

#### Key Combo 

* > Scan Code Sets, Scan Codes and Key Codes

  * This section is good on handling keys being pressed for a long time.

    

  * Key combo

* Have a set of struct of array to keep track of flags

---

#### Single scancode

* note : this isn't the same as single character

#### Compound scancode

> Note that scancodes with extended byte (E0) generates two different interrupts: the first containing the E0 byte, the second containing the scancode

### Communicate with Device

* Send EOI (to pic of course)
* Command?
  * Don't do it.
  * It's just not necessary.

## RTC

* OSdev

* CLI and STI can cause issue
* Now the RTC and keyboard can't work togather
* Send EOI and PIC are so problematic



# Teamwork

### General Strategy On Developing

* No test
  * Must be fixed
* Know the picture before implementation
* Start from basic, but basic must be used

### Work Flow

* Claim mission
  * If you're not working on it, don't claim it
  * Working in chunk of time
    * If you're working on a functionality, do the whole thing at once.
    * We're breaking tasks in atomic parts, meaning non-separable. 
    * e.g. it's ok to break for half a day, and continue working on it.
    * e.g. it's not ok if you didn't do anything (meaning a functionality) this day and still claim it, instead, work it out in that day.
  * Talk  to us. 
    * Talk to your peers. If you're stuck. Tell me you're stuck. Don't fake it. It will improve your efficiency. If you spend >1 hours reading doc, you're doing the wrong thing. If you're programming before thinking it, you're doing the wrong thing. If you're writing code without testing, your code is never going to work.
* Work Flow
  * Don't expect to work it all out in Sunday.
  * Don't expect to debug it all out in Sunday.
  * Don't do anything in Sunday.
  * Use your time. It's not even demanding.

* I will push
  * I won't just wait now

