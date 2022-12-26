/*!
 * @brief This subroutine saves the current context, swap kernel stacks, and restores
   context of the next scheduled process.
   NOTE: The pointer to the context struct has the same value as the `esp` register
   in the current context. By storing & loading `esp` registers through the pointers
   passed in by the caller, the kernel stacks are implicitly swapped after the current
   context is pushed onto the kernel stack, and new context is loaded in the subsequent
   `pop` operations.
   WARNING: This subroutine assumes that a valid context struct is constructed in the
   kernel stack of the next process.
 * @param curr is pointer to the pointer to the context struct of the current process.
 * @param next is pointer to the context struct of the next scheduled process.
 * @return None.
 * @sideeffect Swaps kernel stacks. Saves & Restores callee-saved registers.
 */
.globl swtch
swtch:
    # Load arguments from stack.
    movl 4(%esp), %eax
    movl 8(%esp), %ecx
    
    # Save callee-saved registers.
    pushl %ebp
    pushl %edi
    pushl %esi
    pushl %ebx

    # Swap kernel stacks.
    movl %esp, (%eax)       # Save the current esp and all callee-saved registers. 
    movl %ecx, %esp         # Load new kernel stack pointer.

    # Restore callee-saved registers from swapped kernel stack.
    popl %ebx
    popl %esi
    popl %edi
    popl %ebp
    ret

/*!
 * @brief This subroutine redirects the control path to the "false interrupt handler"
   `pseudoret` defined below.
   WARNING: The address of this subroutine must be manually loaded to the kernel stack
   of the base shell 2 & 3.
 * @param None.
 * @return None.
 * @sideeffect None.
 */
.globl swtchret
swtchret:
    ret

/*!
 * @brief This subroutine recovers the user `ds` segment register and put the `eflags`
   register in kernel stack. Then it returns to user space with `iret` instruction.
   WARNING: This subroutine assumes that a valid context struct is constructed in the
   kernel stack of the next process.
 * @param None.
 * @return None.
 * @sideeffect Modifies segment registers and `eip`.
 */
.globl pseudoret
pseudoret:
    movw 16(%esp), %ds
    pushfl
    popl 8(%esp)
    iret
