.globl swtch
swtch:
    movl 4(%esp), %eax      # Load context struct pointer.
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
    ret                     # Will implicitly jump to address in user `eip`. 
