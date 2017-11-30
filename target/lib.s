.global shutdown
shutdown:
	lidt triple_fault
	int $0

triple_fault:
	.quad 0
	.quad 0

.global _start
_start:
	movl $0x000fffff, %esp
	call main
	jmp shutdown
