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
#include "bootloader/Bootloader.h"

#include "backend/ConsoleOutput.h"
#include "frontend/vgaController.h"

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

// memoria del guest
const uint32_t GUEST_PHYSICAL_MEMORY_SIZE = 8*1024*1024; // Memoria Fisica del guest a 2MB
unsigned char guest_physical_memory[GUEST_PHYSICAL_MEMORY_SIZE] __attribute__ ((aligned(4096)));
unsigned char dumb_stack_memory[4096] __attribute__ ((aligned(4096)));

// logger globale
ConsoleLog& logg = *ConsoleLog::getInstance();

// tastiera emulata (frontend)
keyboard keyb;

// COM1 emulata (frontend)
serial_port* com1;

// gestione input console (backend)
ConsoleInput* console_in;

ConsoleOutput* console_out;
VGAController vga;

void endIO(int val)
{
	// questa operazione va fatta perchè altrimenti la console
	// non tornebbe nello stato di funzionamento precedente
	// all'instanziazione dell'oggetto ConsoleInput
	console_out->resetConsole();
	console_in->resetConsole();

	// chiudiamo il programma
	exit(0);
}

void initIO()
{
	// colleghiamo la tastiera emulata all'input della console
	console_in = ConsoleInput::getInstance();
	console_in->attachKeyboard(&keyb);

	// avviamo il thread che si occuperà di gestire l'input della console
	console_in->startEventThread();

	// inizializziamo la porta seriale
	com1 = new serial_port(0x3f8, logg);

	vga.setVMem((uint16_t*)(guest_physical_memory + 0xB8000));
	console_out = ConsoleOutput::getInstance();
	console_out->attachVGA(&vga);
	console_out->startThread();

	atexit([](){endIO(0);});
	signal(SIGINT, endIO);
}


// funzione chiamata su HLT del programma della vm per ottenere
// un risultato dal programma
void fetch_application_result(int vcpu_fd, kvm_run *kr) {
	/* we can obtain the the contents of all the registers
	 * in the vm.
	 */
	kvm_regs regs;
	if (ioctl(vcpu_fd, KVM_GET_REGS, &regs) < 0) {
		logg << "get regs: " << strerror(errno) << endl;
		return;
	}
	/* (this is for the general purpose registers, we can also
	 * obtain the 'special registers' with KVM_GET_SREGS).
	 */

	logg << std::dec << "Risultato programma (keycode): " << regs.rax << endl;
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

	logg << "\tSREGS base: " << (unsigned int)sregs.ds.base << endl;
	logg << "\tSREGS limit: " << (unsigned int)sregs.ds.limit << endl;
	logg << "\tSREGS selector: " << (unsigned int)sregs.ds.selector << endl;
	logg << "\tSREGS present: " << (unsigned int)sregs.ds.present << endl;
	logg << "\tSREGS type: " << (unsigned int)sregs.ds.type << endl;
	logg << "\tSREGS dpl: " << (unsigned int)sregs.ds.dpl << endl;
	logg << "\tSREGS db: " << (unsigned int)sregs.ds.db << endl;
	logg << "\tSREGS s: " << (unsigned int)sregs.ds.s << endl;
	logg << "\tSREGS l: " << (unsigned int)sregs.ds.l << endl;
	logg << "\tSREGS g: " << (unsigned int)sregs.ds.g << endl;
	logg << "\tSREGS type: " << (unsigned int)sregs.ds.type << endl;
	logg << "\tSREGS selector: " << (unsigned int)sregs.ds.selector << endl;

	logg << "\tIDT base: " << (void *)sregs.idt.base << endl;
	logg << "\tIDT limit: " << (unsigned int)sregs.idt.limit << endl;
	logg << "\tGDT base: " << (void *)sregs.gdt.base << endl;
	logg << "\tGDT limit: " << (unsigned int)sregs.gdt.limit << endl;

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

extern uint64_t estrai_segmento(char *fname, void *dest, uint64_t dest_size);
int main(int argc, char **argv)
{
	// controllo parametri in ingresso
	if(argc != 2 && (argc ==1 || argc == 3 || (argc == 4 && strcmp(argv[2], "-logfile"))) ) {
		cout << "Formato non corretto. Uso: kvm <elf file> [-logfile filefifo]" << endl;
		return 1;
	}

	// ci è stato richiesto di utilizzare un file specifico per il logging
	if(argc == 4 && !strcmp(argv[2], "-logfile"))
		logg.setFilePath(argv[3]);
	else
		logg.setFilePath("console.log");

	// controllo validità del path
	char *elf_file_path = argv[1];
	FILE *elf_file = fopen(elf_file_path, "r");
	if(!elf_file) {
		cout << "Il file selezionato non esiste" << endl;
		return 1;
	}
	fclose(elf_file);

	/* the first thing to do is to open the /dev/kvm pseudo-device,
	 * obtaining a file descriptor.
	 */
	int kvm_fd = open("/dev/kvm", O_RDWR);
	if (kvm_fd < 0) {
		/* as usual, a negative value means error */
		cout << "/dev/kvm: " << strerror(errno) << endl;
		return 1;
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
		return 1;
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

	kvm_userspace_memory_region mrd2 = {
		1,					// slot
		0,					// no flags,
		0xfffff000,					// guest physical addr
		4096,					// memory size
		reinterpret_cast<__u64>(dumb_stack_memory)		// userspace addr
	};

	/* now we can add the memory to the vm */
	if (ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &mrd2) < 0) {
		cout << "set memory (dumb_stack_memory): " << strerror(errno) << endl;
		return 1;
	}

	// carichiamo l'eseguibile da file
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

	Bootloader bootloader(vcpu_fd,guest_physical_memory,entry_point,0x400000L);
	bootloader.run_long_mode();
	//bootloader.run_protected_mode();

	#ifndef SUPPRESS_DEBUG
	uint64_t dump_addr = 0x200100;
	logg << endl << "================== Memory Dump (" << std::hex << dump_addr << " 4KB) ==================" << endl;
	for(int i=dump_addr; i<dump_addr+512; i++)
	{
		fprintf(stderr, "%02x", (unsigned int)((unsigned char*)guest_physical_memory)[i]);
		logg << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)((unsigned char*)guest_physical_memory)[i];
		//cerr << std::hex << (unsigned int)((unsigned char*)guest_physical_memory)[i];
	}
	logg << endl << "=================================================" << endl;
	#endif

	// a questo punto possiamo inizializzare le strutture per l'emulazione dei dispositivi di IO
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
				return 1;
			case KVM_EXIT_IO:
			{
				// questo è il puntatore alla sez di memoria che contiene l'operando da restituire o leggere
				// (in base al tipo di operazione che la vm vuole fare, cioè in o out)
				uint8_t *io_param = (uint8_t*)kr + kr->io.data_offset;

				// ======== Tastiera ========
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
				// ======== Porta Seriale Primaria ========
				else if (kr->io.size == 1 && kr->io.count == 1 && (kr->io.port >= 0x03f8 && kr->io.port <= 0x03ff))
				{
					if(kr->io.direction == KVM_EXIT_IO_OUT)
						com1->write_reg_byte(kr->io.port, *io_param);
					else if(kr->io.direction == KVM_EXIT_IO_IN)
							*io_param = com1->read_reg_byte(kr->io.port);
				}
				// ======== Porta Seriale Secondaria ========
				else if (kr->io.size == 1 && kr->io.count == 1 && kr->io.port == 0x02F8 && kr->io.direction == KVM_EXIT_IO_OUT)
				{
					// usato per debuggare i programmi
					logg << "kvm: Risultato su Porta parallela: " << std::hex << (unsigned int)*io_param << endl;
				}
				// ======== Registri Controller PCI ========
				else if(kr->io.port == 0xcfc || kr->io.port == 0xcf8)
				{
					// skippiamo i warning perchè i programmi target iterano sui dispositivi del bus pci
					// e rallentano l'esecuzione del programma
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
				trace_user_program(vcpu_fd, kr);
				return 1;
			case KVM_EXIT_FAIL_ENTRY:
				logg << "kvm: KVM_EXIT_FAIL_ENTRY reason=" << std::dec << (unsigned long long)kr->fail_entry.hardware_entry_failure_reason << endl;
				trace_user_program(vcpu_fd, kr);
				return 1;
				break;
			case KVM_EXIT_INTERNAL_ERROR:
				logg << "kvm: KVM_EXIT_INTERNAL_ERROR suberror=" << std::dec <<kr->internal.suberror << endl;
				trace_user_program(vcpu_fd, kr);
				return 1;
				break;
			default:
				logg << "kvm: Unhandled VM_EXIT reason=" << std::dec << kr->exit_reason << endl;
				trace_user_program(vcpu_fd, kr);
				return 1;
		}
	}

	// procediamo con la routine di ripristino dell'IO
	endIO(0);

	return 0;
}