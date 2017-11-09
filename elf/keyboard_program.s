.global _start
_start:
	movb $0x60, %al
	outb %al, $0x64
	outb %al, $0x60
wait_char:
	inb $0x64, %al
	test $0x01, %al
	jz wait_char
	inb $0x60, %al

	movw $0x2f8, %dx
	outb %al, %dx
	jmp wait_char
