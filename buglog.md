# Bug Log for ECE 391 MP3

## CP 1 Bug Log

| Stat                                                 | Bug No. |
| ---------------------------------------------------- | ------- |
| :white_check_mark:                                   | `000`   |
| :white_check_mark:                                   | `001`   |
| :white_check_mark:                                   | `002`   |
| :white_check_mark:                                   | `003`   |
| :white_check_mark:                                   | `004`   |
| Potential, Harmless for cp1, will "fix" in later cps | `005`   |

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

