# Features

Hi, This is group 17. We have 3 big sections : each section contains different kinds of features/devices we integrate into our OS.

* Graphics
* Storage
* I/O
* Signal (I know this doesn't look like a category, just don't know where to put it)

## Graphics

Work load : Approx. 50 hours : Approx. 2k lines of code (written by me in total) 3k lines by library, OSdev, and ECE391 staff

* VGA , driver : **done by Prof. Lumetta** in mode X, modified to adapt to our OS

* Octree, kernel level resolution improvement

  * Tricky when implemented in kernel

* GUI, 2 FSM

  * welcome page
    * shows graphics mode
  * login page
    * login FSM 
    * Text input in graphic mode
    * Interaction with user in graphics mode
  * Layers
  * integration with RTC, PIT, mouse, VGA 
  * desktop applications : shell in graphic mode
    * desktop FSM (main and thread in uniprocessor)
  * Down sampling : Up sampling support

  * Bad things about my GUI

    * Other stuff like windowing and stuff (not implemented, just complexity of FSM2, want to develop OS instead of this haha)

    * Super low video quality (although improved by Octree) (Don't want to rewrite VGA drive for other modes)

    * Bad login because of our bad merge (workflow problem)

    * Bad mouse speed (Adapt mouse to text mode, didn't change it into graphics)

      

    

## Storage

Work load : Approx. 60 hours : Approx. 3k lines of original code 0.5k by library and OSdev

* ATA PIO mode 
  * hard drive driver : storage media (& Memory)
  * permanent storage
  * burner
* Writable Filesystem
  * Create file
  * Remove file 
  * Rename file
  * Modify file
* Editor
  * Modify file
  * Save file
  * corner cases
* Malloc
  * dynamic allocation
  * Buddy allocator 
  * Slab allocator

## I/O

Work load : Approx. 40 hours : Approx. 1k lines of orignal code + 1k by library and OSdev

* text + graphics (covered in graphics section)

* Mouse input drive

  * speed adaptation in text mode
  * support for application in graphics mode

* Cursor

  * support in all modes

* Sound drive (Sound blaster 16)

  * Able to run arbitrary `.wav` file

    

## Signal

Workload Approx. 10 hours : approx. 0.5k lines of original code

* Signal kinds

  * SIGINT

  * SIGUSR1
  * SIGALARM
  * SIGSEG
  * SIGDIV

* `sigtest` 
  * shows that we used standard version of signal handling linkage instead of 
    * making it as if there were signals
* all other user level (written by me) used signal

