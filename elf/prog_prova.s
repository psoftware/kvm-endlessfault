.global _start
_start:
	mov $0xfff, %esp
	call main
	hlt
