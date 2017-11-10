.global _start
_start:
	movl $0x000fffff, %esp
	call main
	hlt
