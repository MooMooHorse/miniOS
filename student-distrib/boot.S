# boot.S - start point for the kernel after GRUB gives us control
# vim:ts=4 noexpandtab

#define ASM     1

#include "multiboot.h"
#include "x86_desc.h"

.text

    # Multiboot header (required for GRUB to boot us)
    .long MULTIBOOT_HEADER_MAGIC
    .long MULTIBOOT_HEADER_FLAGS
    .long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

# Entrypoint to the kernel
.globl start, _start

.align 4
start:
_start:
    # Make sure interrupts are off
    cli
    jmp     continue

continue:
    # Load the GDT
    lgdt  gdt_desc_ptr # load GDTR; Refer to IA-32 Intel Spec (Page 50)

    # INIT IDT (TO_DO)
    pushl %eax
    pushl %ecx
    pushl %edx
init_idt:
    movl $KERNEL_CS,%eax
    shll $16,%eax # Note that there are no handlers being installed at this point
    movl $0x00008e00,%edx
    leal idt,%edi
    movl $NUM_VEC,%ecx
loop_idt:
    movl %eax,(%edi) # Segment Selector | Offset 15..0
    movl %edx,4(%edi) # Offset 31..16 | P | DPL | 0D110 | 000 | xxxx
    addl $8, %edi
    decl %ecx
    cmpl $0,%ecx
    jne  loop_idt

    call install_exception_hanlder # install IDT for exception handlers

    popl %edx
    popl %ecx
    popl %eax
    # Load the IDT (TO_DO)
load_idt:
    lidt idt_desc_ptr 
    # Load CS with the new descriptor value
    ljmp    $KERNEL_CS, $keep_going

keep_going:
    # Set up ESP so we can have an initial stack
    movl    $0x800000, %esp

    # Set up the rest of the segment selector registers
    movw    $KERNEL_DS, %cx
    movw    %cx, %ss
    movw    %cx, %ds
    movw    %cx, %es
    movw    %cx, %fs
    movw    %cx, %gs

    # Push the parameters that entry() expects (see kernel.c):
    # eax = multiboot magic
    # ebx = address of multiboot info struct
    pushl   %ebx
    pushl   %eax

    # Jump to the C entrypoint to the kernel.
    call    entry

    # We'll never get back here, but we put in a hlt anyway.
halt:
    hlt
    jmp     halt
