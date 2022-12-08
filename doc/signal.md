# Signal

* This document is for our group to understand what I did for my first extra credit - signal.

I would expect to use today approx. 12 hours on signal, so tomorrow I can work on writable filesystem. 

## Delivery Signal

* Check it in assembly linkage 

  * interrupt
  * exception
  * system call

* You should make sure, before you go into `sig_enter` assembly linkage, the kernel stack looks like

  * ```
    |saved
    |registers
    ------
    |return address(eip)
    |0 | CS
    |eflags
    |esp
    |0 | SS
    ```

  * This **hardware context** will **NOT** be used for you as an path after `sigreturn`

1. You should previous mentioned hardware context as **entry to signal handler**

   * by IRET
   * so after IRET you kernel stack is empty

2. Execute signal handler 

   * after which you will jump to assembly linkage for `sigreturn`
   * note that this jump happens at user level 

3. `sigreturn` is signal version of `halt`

   * After `sigreturn` you should return to the position in **user level program** where checking assembly linkage took place (**in kernel**).

     * This is a little abstract : I will explain in this way : When you check assembly linkage, there are only 3 possibilities, interrupt, exception, system call, all of which move you from user level program to kernel code, in kernel code, just before IRET, you have a hardware context that contain the info of where you'll return to user level program, now we decide not to do `iret` afterwards, we decide, to `iret` in `sigreturn`. So how to make `sigreturn` work? We copy the IRET parameters and register values(which can be altered by follow-up signal handling) and before signal handler, we **save a snapshot in user level program**, so in `sigreturn` we just have to **overwrite** the kernel stack back to the **saved version**.

     * Why overwrite, you should realize that after going to signal handler, our **kernel stack is empty**, when calling `sigreturn`, we push hardware context into kernel stack. So now, the kernel stack is in form below

     * ```
       |saved
       |registers
       |for sigreturn
       |(need overwrite)
       ------
       |return address(eip) (at sigreturn, needs overwrite)
       |0 | CS
       |eflags (current flags, needs overwrite to previous flags)
       |esp (for current user stack, needs overwrite to old(previous) user stack)
       |0 | SS
       ```

       
