.code32
.global _start
_start:
    movl $0x2000, %eax
    movl %eax, %cr3
    movl %cr4, %eax
    orl $0x00000020, %eax
    movl %eax, %cr4
 
    movl $0xC0000080, %ecx
    rdmsr
    orl $0x00000100, %eax
    wrmsr
 
    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0
# saltiamo all'entry point
    pushl %edi
    ret 

 
.code64
long_mode:
    pushq $0
    popfq
    movw $0, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    jmp _start