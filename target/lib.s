.global shutdown
shutdown:
	lidt triple_fault
	int $0

triple_fault:
	.quad 0
	.quad 0

.global exit
exit:
        movq %rdi, %rax
        hlt
        ret

.global outputb
outputb:
        pushq %rax
        pushq %rdx
        movb %dil, %al
        movw %si, %dx
        outb %al, %dx
        popq %rdx
        popq %rax
        ret

.global inputb
inputb:
        pushq %rax
        pushq %rdx
        movw %di, %dx
        inb %dx, %al
        movb %al, (%rsi)
        popq %rdx
        popq %rax
        ret

.global _start
_start:
	movl $0x000fffff, %esp
	call main
	jmp shutdown
