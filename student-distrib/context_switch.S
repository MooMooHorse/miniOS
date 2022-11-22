.globl swtch
swtch:
    movl 4(%esp), %eax      # Pointer to pointer to the base struct.
    movl 8(%esp), %ecx
    
    # Save callee-saved registers.
    pushl %ebp
    pushl %edi
    pushl %esi
    pushl %ebx

    # Swap stacks.
    movl %esp, (%eax)       # Load base address of the context struct.
    movl %ecx, %esp

    # Restore callee-saved registers from *current* stack.
    popl %ebx
    popl %esi
    popl %edi
    popl %ebp
    ret                     # Implicitly jump to address in user `eip`.
