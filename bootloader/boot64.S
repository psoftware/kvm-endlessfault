#g++ -m32 -nostdlib -fno-exceptions -g -fno-rtti -fno-stack-protector -mno-red-zone -gdwarf-2 -fpic -m32 -Ttext=0 bootloader.S -o bootloader -Wl,-fuse-ld=gold

.text

#define STACK_SIZE                      0x1000

	 .globl  _start
_start:
	cli
	movl    $(stack + STACK_SIZE), %esp // inizializziamo lo stack

	movl %cr4, %eax
	orl $0x00000020, %eax  //cr4.pae
	movl %eax, %cr4

	movl $0xC0000080, %ecx   //efer msr
	rdmsr
	orl $0x00000100, %eax   //efer.lme
	wrmsr

	movl %cr0, %eax
	orl $0x80000000, %eax   //cr0.pg
	movl %eax, %cr0
	lgdt gdt64_ptr
	ljmp $8, $long_mode

.code64
long_mode:
	pushq $0
	popfq
	movw $0, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	#movw $0, %ax
	movw %ax, %ss

	jmp *%rdi
.code32

////////////////////////////////////////////////////////////////
// sezione dati: tabelle e stack			      //
////////////////////////////////////////////////////////////////
.data
.balign 4096
gdt64:
	.quad 0		//segmento nullo
	.quad 0x209c0000000000 //segmento codice sistema
gdt64_end:
gdt64_ptr:
	.word gdt64_end - gdt64
	.long gdt64


	.global stack
stack:
	.space STACK_SIZE, 0
