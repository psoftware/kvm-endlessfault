#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <cerrno>
#include <cstring>
#include <sys/mman.h>
#include <signal.h>

#include "frontend/IODevice.h"
#include "frontend/keyboard.h"
#include "frontend/serial_port.h"
#include "backend/ConsoleLog.h"
#include "backend/ConsoleInput.h"
#include "debug-server/DebugServer.h"
#include "bootloader/Bootloader.h"
#include "backend/ConsoleOutput.h"
#include "frontend/vgaController.h"
#include "INIReader.h"

#include "gdbserver/gdbserver.h"

using namespace std;

/* We are going to use the linux kvm API to crate a simple
 * virtual machine and execute some code inside it.
 *
 * The virtual machine is going to have just one CPU and
 * a physical memory consisting of just two pages
 */

/* First, we need to include the kvm.h file
 * (which you can usually found in /usr/include/linux/kvm.h).
 * The file contains the definitions of all the constants and
 * data structures that we are going to use, and it is the
 * source you should look at for the names of the fields and so on.
 *
 * Note: IA32/64 specific data structures (such as kvm_regs) are defined
 * in /usr/include/asm/kvm.h, included by this one.
 */
#include <linux/kvm.h>


// guest memory
uint32_t GUEST_PHYSICAL_MEMORY_SIZE = 8*1024*1024; //default guest physical memory to 8MB
unsigned char *guest_physical_memory = NULL;

// flag to check kvm debug mode
bool debug_mode;

// global logger
ConsoleLog& logg = *ConsoleLog::getInstance();

// emulated keyboard (frontend)
keyboard keyb;

// emulated serial port (frontend)
serial_port* com1;
serial_port* com2;
serial_port* com3;
serial_port* com4;

// keyboard backend
ConsoleInput* console_in;

//text mode video memory emulation  
ConsoleOutput* console_out;
VGAController vga; // emulated vga controller

void endIO(int val)
{
	//in order to restore the previous console state before the instantiation
	console_out->resetConsole();
	console_in->resetConsole();

	// close the program
	exit(0);
}

void initIO()
{
	// link the emulated keyboard to the console input
	console_in = ConsoleInput::getInstance();
	console_in->attachKeyboard(&keyb);

	// console input management thread
	console_in->startEventThread();

	// serial ports initialization
	com1 = new serial_port(0x3f8, logg);
	com2 = new serial_port(0x2f8, logg);
	com3 = new serial_port(0x3e8, logg);
	com4 = new serial_port(0x2e8, logg);

	vga.setVMem((uint16_t*)(guest_physical_memory + 0xB8000)); // set text mode video memory offset

	// link the emulated vga controller to the backend
	console_out = ConsoleOutput::getInstance();
	console_out->attachVGA(&vga);

	// start console output threads
	console_out->startThread();

	// handle ctrl+c termination in order to restore the console state
	atexit([](){endIO(0);});
	signal(SIGINT, endIO);
}


// function called on HLT vm program to obtain a program result
void fetch_application_result(int vcpu_fd, kvm_run *kr) {
	/* we can obtain the the contents of all the registers
	 * in the vm.
	 */
	kvm_regs regs;
	if (ioctl(vcpu_fd, KVM_GET_REGS, &regs) < 0) {
		logg << "get regs: " << strerror(errno) << endl;
		return;
	}

	logg << std::dec << "Program result (keycode): " << regs.rax << endl;
}

void trace_kvm_segment(const kvm_segment& seg)
{
	logg << "\tSREGS base: " << (unsigned int)seg.base << endl;
	logg << "\tSREGS limit: " << (unsigned int)seg.limit << endl;
	logg << "\tSREGS selector: " << (unsigned int)seg.selector << endl;
	logg << "\tSREGS present: " << (unsigned int)seg.present << endl;
	logg << "\tSREGS type: " << (unsigned int)seg.type << endl;
	logg << "\tSREGS dpl: " << (unsigned int)seg.dpl << endl;
	logg << "\tSREGS db: " << (unsigned int)seg.db << endl;
	logg << "\tSREGS s: " << (unsigned int)seg.s << endl;
	logg << "\tSREGS l: " << (unsigned int)seg.l << endl;
	logg << "\tSREGS g: " << (unsigned int)seg.g << endl;
	logg << "\tSREGS type: " << (unsigned int)seg.type << endl;
	logg << "\tSREGS selector: " << (unsigned int)seg.selector << endl;
}

void trace_user_program(int vcpu_fd, kvm_run *kr) {
	kvm_regs regs;
	if (ioctl(vcpu_fd, KVM_GET_REGS, &regs) < 0) {
		logg << "trace_user_program KVM_GET_REGS error: " << strerror(errno) << endl;
		return;
	}

	kvm_sregs sregs;
	if (ioctl(vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
		logg << "trace_user_program KVM_GET_SREGS error: " << strerror(errno) << endl;
		exit(1);
	}

	logg << "Target program dump: " << endl;
	logg << "\tRIP: " << (void *)regs.rip << endl;
	logg << "\tRSP: " << (void *)regs.rsp << endl;
	logg << "\tCR4: " << (void *)sregs.cr4 << endl;
	logg << "\tCR3: " << (void *)sregs.cr3 << endl;
	logg << "\tCR2: " << (void *)sregs.cr2 << endl;
	logg << "\tCR0: " << (void *)sregs.cr0 << endl;
	logg << "\tEFER: " << (void *)sregs.efer << endl;

	logg << "\tIDT base: " << (void *)sregs.idt.base << endl;
	logg << "\tIDT limit: " << (unsigned int)sregs.idt.limit << endl;
	logg << "\tGDT base: " << (void *)sregs.gdt.base << endl;
	logg << "\tGDT limit: " << (unsigned int)sregs.gdt.limit << endl;

	logg << "Segment CS:\n";
	trace_kvm_segment(sregs.cs);
	logg << "Segment DS:\n";
	trace_kvm_segment(sregs.ds);
	logg << "Segment ES:\n";
	trace_kvm_segment(sregs.es);
	logg << "Segment FS:\n";
	trace_kvm_segment(sregs.fs);
	logg << "Segment GS:\n";
	trace_kvm_segment(sregs.gs);
	logg << "Segment SS:\n";
	trace_kvm_segment(sregs.ss);

	kvm_vcpu_events events;
	if (ioctl(vcpu_fd, KVM_GET_VCPU_EVENTS, &events) < 0) {
		logg << "trace_user_program KVM_GET_VCPU_EVENTS error: " << strerror(errno) << endl;
		exit(1);
	}

	logg << "Exception:" << endl;
	logg << "\thas_error_code: " << (unsigned int)events.exception.has_error_code << endl;
	logg << "\terror_code: " << (unsigned int)events.exception.error_code << endl;

	logg << "Interrupt:" << endl;
	logg << "\tinjected: " << (unsigned int)events.interrupt.injected << endl;
	logg << "\tnr: " << (unsigned int)events.interrupt.nr << endl;
	logg << "\tsoft: " << (unsigned int)events.interrupt.soft << endl;
	logg << "\tshadow: " << (unsigned int)events.interrupt.shadow << endl;

	logg << "NMI:" << endl;
	logg << "\tinjected: " << (unsigned int)events.nmi.injected << endl;
	logg << "\tpending: " << (unsigned int)events.nmi.pending << endl;
	logg << "\tmasked: " << (unsigned int)events.nmi.masked << endl;
	logg << "\tpad: " << (unsigned int)events.nmi.pad << endl;
}

void dump_memory(uint64_t offset, int size)
{
	logg << endl << "================== Memory Dump (0x" << std::hex << (unsigned long)offset << " size " << std::dec << size <<" B) ==================" << endl;
	for(int i=0; i<size; i++)
		logg << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)((uint8_t*)guest_physical_memory + offset)[i];
	logg << endl << "=======================================================================" << endl;
}

void gdb_submit_registers(int vcpu_fd)
{
	kvm_regs regs;
	if (ioctl(vcpu_fd, KVM_GET_REGS, &regs) < 0) {
		logg << "trace_user_program KVM_GET_REGS error: " << strerror(errno) << endl;
		return;
	}

	kvm_sregs sregs;
	if (ioctl(vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
		logg << "trace_user_program KVM_GET_SREGS error: " << strerror(errno) << endl;
		exit(1);
	}

	gdbserver_set_register(AMD64_RAX_REGNUM, regs.rax);		/* %rax */
	gdbserver_set_register(AMD64_RBX_REGNUM, regs.rbx);		/* %rbx */
	gdbserver_set_register(AMD64_RCX_REGNUM, regs.rcx);		/* %rcx */
	gdbserver_set_register(AMD64_RDX_REGNUM, regs.rdx);		/* %rdx */
	gdbserver_set_register(AMD64_RSI_REGNUM, regs.rsi);		/* %rsi */
	gdbserver_set_register(AMD64_RDI_REGNUM, regs.rdi);		/* %rdi */
	gdbserver_set_register(AMD64_RBP_REGNUM, regs.rbp);		/* %rbp */
	gdbserver_set_register(AMD64_RSP_REGNUM, regs.rsp);		/* %rsp */
	gdbserver_set_register(AMD64_R8_REGNUM, regs.r8);		/* %r8 */
	gdbserver_set_register(AMD64_R9_REGNUM, regs.r9);		/* %r9 */
	gdbserver_set_register(AMD64_R10_REGNUM, regs.r10);		/* %r10 */
	gdbserver_set_register(AMD64_R11_REGNUM, regs.r11);		/* %r11 */
	gdbserver_set_register(AMD64_R12_REGNUM, regs.r12);		/* %r12 */
	gdbserver_set_register(AMD64_R13_REGNUM, regs.r13);		/* %r13 */
	gdbserver_set_register(AMD64_R14_REGNUM, regs.r14);		/* %r14 */
	gdbserver_set_register(AMD64_R15_REGNUM, regs.r15);		/* %r15 */
	gdbserver_set_register(AMD64_RIP_REGNUM, regs.rip);		/* %rip */
	gdbserver_set_register(AMD64_EFLAGS_REGNUM, regs.rflags);		/* %eflags */
	gdbserver_set_register(AMD64_CS_REGNUM, sregs.cs.selector);		/* %cs */
	gdbserver_set_register(AMD64_SS_REGNUM, sregs.ss.selector);		/* %ss */
	gdbserver_set_register(AMD64_DS_REGNUM, sregs.ds.selector);		/* %ds */
	gdbserver_set_register(AMD64_ES_REGNUM, sregs.es.selector);		/* %es */
	gdbserver_set_register(AMD64_FS_REGNUM, sregs.fs.selector);		/* %fs */
	gdbserver_set_register(AMD64_GS_REGNUM, sregs.gs.selector);		/* %gs */

	// extra registers
	gdbserver_set_custom_register(AMD64_CR4_REGNUM, sregs.cr4);		/* %cr4 */
	gdbserver_set_custom_register(AMD64_CR3_REGNUM, sregs.cr3);		/* %cr3 */
	gdbserver_set_custom_register(AMD64_CR2_REGNUM, sregs.cr2);		/* %cr2 */
	gdbserver_set_custom_register(AMD64_CR0_REGNUM, sregs.cr0);		/* %cr0 */
	gdbserver_set_custom_register(AMD64_EFER_REGNUM, sregs.efer);		/* %efer */
}

kvm_guest_debug guest_debug;
int debug_vcpu_id;

void kvm_load_registers_from_gdbcache()
{
	kvm_regs regs;
	if (ioctl(debug_vcpu_id, KVM_GET_REGS, &regs) < 0) {
		logg << "trace_user_program KVM_GET_REGS error: " << strerror(errno) << endl;
		return;
	}

	regs.rax = gdbserver_get_register(AMD64_RAX_REGNUM);		/* %rax */
	regs.rbx = gdbserver_get_register(AMD64_RBX_REGNUM);		/* %rbx */
	regs.rcx = gdbserver_get_register(AMD64_RCX_REGNUM);		/* %rcx */
	regs.rdx = gdbserver_get_register(AMD64_RDX_REGNUM);		/* %rdx */
	regs.rsi = gdbserver_get_register(AMD64_RSI_REGNUM);		/* %rsi */
	regs.rdi = gdbserver_get_register(AMD64_RDI_REGNUM);		/* %rdi */
	regs.rbp = gdbserver_get_register(AMD64_RBP_REGNUM);		/* %rbp */
	regs.rsp = gdbserver_get_register(AMD64_RSP_REGNUM);		/* %rsp */
	regs.r8 = gdbserver_get_register(AMD64_R8_REGNUM);		/* %r8 */
	regs.r9 = gdbserver_get_register(AMD64_R9_REGNUM);		/* %r9 */
	regs.r10 = gdbserver_get_register(AMD64_R10_REGNUM);		/* %r10 */
	regs.r11 = gdbserver_get_register(AMD64_R11_REGNUM);		/* %r11 */
	regs.r12 = gdbserver_get_register(AMD64_R12_REGNUM);		/* %r12 */
	regs.r13 = gdbserver_get_register(AMD64_R13_REGNUM);		/* %r13 */
	regs.r14 = gdbserver_get_register(AMD64_R14_REGNUM);		/* %r14 */
	regs.r15 = gdbserver_get_register(AMD64_R15_REGNUM);		/* %r15 */
	regs.rip = gdbserver_get_register(AMD64_RIP_REGNUM);		/* %rip */
	regs.rflags = gdbserver_get_register(AMD64_EFLAGS_REGNUM);		/* %eflags */

	if (ioctl(debug_vcpu_id, KVM_SET_REGS, &regs) < 0) {
		logg << "kvm_load_registers_from_gdbcache KVM_SET_REGS error: " << strerror(errno) << endl;
		return;
	}
}

void kvm_debug_set_step(bool enable_step)
{
	if(enable_step)
		guest_debug.control |= KVM_GUESTDBG_SINGLESTEP;
	else
		guest_debug.control &= ~(KVM_GUESTDBG_SINGLESTEP);

	if (ioctl(debug_vcpu_id, KVM_SET_GUEST_DEBUG, &guest_debug) < 0) {
		logg << "KVM_SET_GUEST_DEBUG: " << strerror(errno) << endl;
		exit(1);
	}
}

void kvm_enable_guest_debug(int vcpu_fd, uint64_t breakpoint_addr)
{
	debug_vcpu_id = vcpu_fd;
	guest_debug.control = KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_USE_HW_BP | KVM_GUESTDBG_USE_SW_BP;
	//guest_debug.arch.debugreg[0] = breakpoint_addr; // DR0
	//guest_debug.arch.debugreg[7] = 0x1; // DR7

	if (ioctl(vcpu_fd, KVM_SET_GUEST_DEBUG, &guest_debug) < 0) {
		logg << "KVM_SET_GUEST_DEBUG: " << strerror(errno) << endl;
		exit(1);
	}
}

void kvm_debug_completed(int vcpu_fd)
{
	guest_debug.arch.debugreg[7] = 0; // DR7

	if (ioctl(vcpu_fd, KVM_SET_GUEST_DEBUG, &guest_debug) < 0) {
		logg << "KVM_SET_GUEST_DEBUG: " << strerror(errno) << endl;
		exit(1);
	}
}

void kvm_handle_debug_exit(int vcpu_fd, kvm_debug_exit_arch dbg_arch)
{
	// refresh gdbserver registers cache 
	gdb_submit_registers(vcpu_fd);

	// restore rip to the value before the trap exception
	gdbserver_set_register(AMD64_RIP_REGNUM, dbg_arch.pc);

	// report to gdb a breakpoint exception (type 3)
	gdbserver_handle_exception(SIGTRAP);
}


extern uint64_t estrai_segmento(char *fname, void *dest, uint64_t dest_size);
int main(int argc, char **argv)
{

	uint32_t mem_size;
	uint16_t serv_port;

	// check input parameters
	if(argc != 2 && (argc ==1 || argc == 3 || (argc == 4 && strcmp(argv[2], "-logfile"))) ) {
		cout << "Format not correct. Use: kvm <elf file> [-logfile filefifo]" << endl;
		return 1;
	}


	// if a specified file to log into
	if(argc == 4 && !strcmp(argv[2], "-logfile"))
		logg.setFilePath(argv[3]);
	else
		logg.setFilePath("console.log");

	// check path validity
	char *elf_file_path = argv[1];
	FILE *elf_file = fopen(elf_file_path, "r");
	if(!elf_file) {
		cout << "The selected executable does not exist" << endl;
		return 1;
	}
	fclose(elf_file);

	// load configuration file
	INIReader reader("config.ini");

    if (reader.ParseError() < 0) {
        cout << "Can't load 'config.ini'\n";
        return 2;
    }


    mem_size = reader.GetInteger("vm-spec", "memsize", 8);
    serv_port = reader.GetInteger("debug-server", "port", -1);
             
    if( mem_size >= 8 && mem_size < 1024 ){
    		mem_size = ((mem_size & 1UL) == 0) ? mem_size : mem_size+1;
    	GUEST_PHYSICAL_MEMORY_SIZE = mem_size*1024*1024;
    }
 	logg << "GUEST_PHYSICAL_MEMORY_SIZE=" << GUEST_PHYSICAL_MEMORY_SIZE << endl;

 	guest_physical_memory = (unsigned char*)aligned_alloc(4096, GUEST_PHYSICAL_MEMORY_SIZE);
    if( guest_physical_memory == NULL )
    {
    	cout << "Cannot allocate guest_physical_memory" << endl;
    	return 3;
    }
    
    ////////////////////
	/* the first thing to do is to open the /dev/kvm pseudo-device,
	 * obtaining a file descriptor.
	 */
	int kvm_fd = open("/dev/kvm", O_RDWR);
	if (kvm_fd < 0) {
		/* as usual, a negative value means error */
		cout << "/dev/kvm: " << strerror(errno) << endl;
		return 4;
	}

	/* we interact with our kvm_fd file descriptor using ioctl()s.
	 * There are several of them, but the most important here is the
	 * one that allows us to create a new virtual machine.
	 * The ioctl() returns us a new file descriptor, which
	 * we can then use to interact with the vm.
	 */
	int vm_fd = ioctl(kvm_fd, KVM_CREATE_VM, 0);
	if (vm_fd < 0) {
		cout << "create vm: " << strerror(errno) << endl;
		return 5;
	}

	/* initially, the vm has no resources: no memory, no cpus.
	 * Here we add the (guest) physical memory, using the
	 * 'code' and 'data' arrays that we have defined above.
	 * To add memory to the machine, we need to fill a
	 * 'kvm_userspace_memory_region' structure and pass it
	 * to the vm_fd using an ioctl().
	 * The virtual machine has several 'slots' where we
	 * can add physical memory. The slot we want to fill
	 * (or replace) is the first field in the structure.
	 * Following the slot number, we can specify some flags
	 * (e.g., to say that this memory is read only, perhaps
	 * to emulate a ROM). The remaining fields should be
	 * obvious.
	 */

	kvm_userspace_memory_region mrd = {
		0,					// slot
		0,					// no flags,
		0,					// guest physical addr
		GUEST_PHYSICAL_MEMORY_SIZE,					// memory size
		reinterpret_cast<__u64>(guest_physical_memory)		// userspace addr
	};

	/* note that the memory is shared between us and the vm.
	 * Whatever we write in the 'data' array above will be seen
	 * by the vm and, vice-versa, whatever the vm writes
	 * in its first "physical" page we can read in the in the
	 * 'data' array. We can even do this concurrently, if we
	 * use several threads.
	 */

	/* now we can add the memory to the vm */
	if (ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &mrd) < 0) {
		cout << "set memory (guest_physical_memory): " << strerror(errno) << endl;
		return 1;
	}

	// load elf file
	uint64_t entry_point = estrai_segmento(elf_file_path, (void*)guest_physical_memory, GUEST_PHYSICAL_MEMORY_SIZE);

	/* now we add a virtual cpu (vcpu) to our machine. We obtain yet
	 * another open file descriptor, which we can use to
	 * interact with the vcpu. Note that we can have several
	 * vcpus, to emulate a multi-processor machine.
	 */
	int vcpu_fd = ioctl(vm_fd, KVM_CREATE_VCPU, 0);
	if (vcpu_fd < 0) {
		cout << "create vcpu: " << strerror(errno) << endl;
		return 1;
	}

	// start debug server
	try {
		DebugServer debugs(serv_port,vcpu_fd,GUEST_PHYSICAL_MEMORY_SIZE,guest_physical_memory);
		debugs.start();
	} catch( ... ) {
		logg << "Not possible to open gdb server" << endl;
	}


	/* the exchange of information between us and the vcpu is
	 * via a 'kvm_run' data structure in shared memory, one
	 * for each vpcu. To obtain a pointer to this data structure
	 * we need to mmap() the vcpu_fd file descriptor that we
	 * obtained above. First, we need to know the size of
	 * the data structure, which we can obtain with the
	 * following ioctl() on the original kvm_fd (the one
	 * we obtained from the open("/dev/kvm")).
	 */
	long mmap_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
	if (mmap_size < 0) {
		cout << "get mmap size: " << strerror(errno) << endl;
		return 1;
	}

	/* and now the mmap() */
	kvm_run *kr = static_cast<kvm_run *>(mmap(
			/* let the kernel  choose the address */
			NULL,
			/* the size we obtained above */
			mmap_size,
			/* we want to both read and write */
			PROT_READ|PROT_WRITE,
			/* this is a shared mapping. A private mapping
			 * would cause our writes to go into the swap area.
			 */
			MAP_SHARED,
			/* finally, the file descriptor we want to map */
			vcpu_fd,
			/* the 'offset' must be 0 */
			0
		));
	if (kr == MAP_FAILED) {
		cout << "mmap: " << strerror(errno) << endl;
		return 1;
	}

	Bootloader bootloader(vcpu_fd,guest_physical_memory,GUEST_PHYSICAL_MEMORY_SIZE,entry_point,0x400000L);
	bootloader.run_long_mode();

	#ifdef DEBUG_LOG
	dump_memory(0x200000, 512);
	#endif

	// ========== GDB Server ==========
	if(debug_mode = reader.GetBoolean("gdb-server", "enable", false)) {
		// enable kvm EXIT_DEBUG
		kvm_enable_guest_debug(vcpu_fd, entry_point);

		// refresh gdb cache registers
		gdb_submit_registers(vcpu_fd);

		// read parameters and start gdb server
		std::string gdb_address = reader.Get("gdb-server", "address", "127.0.0.1");
		unsigned short gdb_port = reader.GetInteger("gdb-server", "port", -1);
		gdbserver_start(gdb_address.c_str(), gdb_port);
	}
	// =================================

	// now we can initialize IO devices structures for emulation
	initIO();

	/* we are finally ready to start the machine, by issuing
	 * the KVM_RUN ioctl() on the vcpu_fd. While the machine
	 * is running our process is 'inside' the ioctl(). When
	 * the machine exits (for whatever reason), the ioctl()
	 * returns. We can then read the reason for the exit in the
	 * kvm_run structure that we mmap()ed above, take the
	 * appropriate action (e.g., emulate I/O) and re-enter
	 * the vm, by issuing another KVM_RUN ioctl().
	 */

	bool continue_run = true;
	while(continue_run)
	{
		if (ioctl(vcpu_fd, KVM_RUN, 0) < 0) {
			logg << "run: " << strerror(errno) << endl;
			return 1;
		}

		switch(kr->exit_reason)
		{
			case KVM_EXIT_HLT:
				fetch_application_result(vcpu_fd, kr);
				if(debug_mode)
					gdbserver_handle_exception(SIGTERM);
				return 1;
			case KVM_EXIT_IO:
			{
				// this is a pointer to the memory section which contains the operand to return or read (if there is a input or output operation)
				uint8_t *io_param = (uint8_t*)kr + kr->io.data_offset;

				// ======== Keyboard ========
				if (kr->io.size == 1 && kr->io.count == 1 && (kr->io.port == 0x60 || kr->io.port == 0x64))
				{
					if(kr->io.direction == KVM_EXIT_IO_OUT)
						keyb.write_reg_byte(kr->io.port, *io_param);
					else if(kr->io.direction == KVM_EXIT_IO_IN)
						*io_param = keyb.read_reg_byte(kr->io.port);
				}
				// ======== VGA Controller ========
				else if (kr->io.size == 1 && kr->io.count == 1 && (kr->io.port == 0x03D4 || kr->io.port == 0x03D5))
				{
					if(kr->io.direction == KVM_EXIT_IO_OUT)
						vga.write_reg_byte(kr->io.port, *io_param);
					else if(kr->io.direction == KVM_EXIT_IO_IN)
							*io_param = vga.read_reg_byte(kr->io.port);
				}
				// ======== COM 1 ========
				else if (kr->io.size == 1 && kr->io.count == 1 && (kr->io.port >= 0x03f8 && kr->io.port <= 0x03ff))
				{
					if(kr->io.direction == KVM_EXIT_IO_OUT)
						com1->write_reg_byte(kr->io.port, *io_param);
					else if(kr->io.direction == KVM_EXIT_IO_IN)
							*io_param = com1->read_reg_byte(kr->io.port);
				}
				// ======== COM 2 ========
				else if (kr->io.size == 1 && kr->io.count == 1 && (kr->io.port >= 0x02f8 && kr->io.port <= 0x02ff))
				{
					if(kr->io.direction == KVM_EXIT_IO_OUT)
						com2->write_reg_byte(kr->io.port, *io_param);
					else if(kr->io.direction == KVM_EXIT_IO_IN)
							*io_param = com2->read_reg_byte(kr->io.port);
				}
				// ======== COM 3 ========
				else if (kr->io.size == 1 && kr->io.count == 1 && (kr->io.port >= 0x03e8 && kr->io.port <= 0x03ef))
				{
					if(kr->io.direction == KVM_EXIT_IO_OUT)
						com3->write_reg_byte(kr->io.port, *io_param);
					else if(kr->io.direction == KVM_EXIT_IO_IN)
							*io_param = com3->read_reg_byte(kr->io.port);
				}
				// ======== COM 4 ========
				else if (kr->io.size == 1 && kr->io.count == 1 && (kr->io.port >= 0x02e8 && kr->io.port <= 0x02ef))
				{
					if(kr->io.direction == KVM_EXIT_IO_OUT)
						com4->write_reg_byte(kr->io.port, *io_param);
					else if(kr->io.direction == KVM_EXIT_IO_IN)
							*io_param = com4->read_reg_byte(kr->io.port);
				}
				// ======== Controller PCI Registers ========
				else if(kr->io.port == 0xcfc || kr->io.port == 0xcf8)
				{
					// target programs iterate on bus pci devices and slow down the execution of the program so we skip those warnings
				}
				else
				{
					logg << "kvm: Unhandled VM IO: " <<  ((kr->io.direction == KVM_EXIT_IO_IN)?"IN":"OUT")
						<< " on kr->io.port " << std::hex << (unsigned int)kr->io.port << endl;
					break;
				}
				break;
			}
			case KVM_EXIT_MMIO:
				logg << "kvm: unhandled KVM_EXIT_MMIO"
						<< " address=" << std::hex << (uint64_t)kr->mmio.phys_addr
						<< " len=" << (uint32_t)kr->mmio.len
						<< " data=" << (uint32_t)((kr->mmio.data[3] << 24) | (kr->mmio.data[2] << 16) | (kr->mmio.data[1] << 8) | kr->mmio.data[0])
						<< " is_write=" << (short)kr->mmio.is_write << endl;
				//trace_user_program(vcpu_fd, kr);
				//return 1;
				break;
			case KVM_EXIT_SHUTDOWN:
				logg << "kvm: TRIPLE FAULT. Shutting down" << endl;
				//trace_user_program(vcpu_fd, kr);
				if(debug_mode)
					gdbserver_handle_exception(SIGTERM);
				return 1;
			case KVM_EXIT_DEBUG:
				if(!debug_mode)
				{
					logg << "kvm: Unexpected KVM_EXIT_DEBUG: debug mode is not enabled" << endl;
					trace_user_program(vcpu_fd, kr);
					return 1;
				}
				logg << "kvm: KVM_EXIT_DEBUG" << endl;
				kvm_handle_debug_exit(vcpu_fd, kr->debug.arch);
				break;
			// ================== Error Conditions ==================
			case KVM_EXIT_FAIL_ENTRY:
				logg << "kvm: KVM_EXIT_FAIL_ENTRY reason=" << std::dec << (unsigned long long)kr->fail_entry.hardware_entry_failure_reason << endl;
				trace_user_program(vcpu_fd, kr);
				if(debug_mode)
					gdbserver_handle_exception(SIGILL);
				return 1;
			case KVM_EXIT_INTERNAL_ERROR:
				logg << "kvm: KVM_EXIT_INTERNAL_ERROR suberror=" << std::dec <<kr->internal.suberror << endl;
				trace_user_program(vcpu_fd, kr);
				if(debug_mode)
					gdbserver_handle_exception(SIGILL);
				return 1;
			default:
				logg << "kvm: Unhandled VM_EXIT reason=" << std::dec << kr->exit_reason << endl;
				trace_user_program(vcpu_fd, kr);
				if(debug_mode)
					gdbserver_handle_exception(SIGILL);
				return 1;
		}
	}

	// restore IO
	endIO(0);

	return 0;
}