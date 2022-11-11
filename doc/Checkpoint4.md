# Checkpoint 4

author : Hao Ren

This document helps each member of the group knows the ideas of what other people are doing, so that we're all on the same page.

* The only one system call in this checkpoint

## Video map

### From?

A certain piece of video memory.

* In our case
* Right next to 4MB user program
* So a new page directory is added, a new page table is created, only one item in that table is used(present)
* We need to free them in halt

### To?

* The video memory starting at `0xB8000`

### Do we need to worry about multi-terminal case from this?

* No
* Multi-terminal doesn't even use this mapping
* This mapping doesn't create new memory (one piece of physical address are mapped from 2 piece of virtual address).
* Terminal use the real(virtual and physical aligned) virtual address.

#### What about buffer for each terminal?

* That's just, for each terminal, when you want to switch them off, you store the content of video memory into **its** buffer.
* Not real-time. (dump at once)
* When you switch to the terminal, you load its buffer.
* nothing to do with this mapping, which is to let **user** write to video memory **directly**.

## Can user program have race condition with our keyboard input?

* cli

## Can user program have conflict in displayed content with keyboard input?

* yes
