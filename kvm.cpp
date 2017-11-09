#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <sys/mman.h>

#include "frontend/IODevice.h"
#include "frontend/keyboard.h"
#include "backend/ConsoleInput.h"

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

/* this is the code that we want to execute. The machine starts
 * in "real mode", so this is 16 bit code. Note the use of 16 bits
 * registers (%bp, %ax, ...); moreover, the stack items inserted or
 * removed by push, pop, call and ret are all 2-bytes wide.
 *
 * This array will become one of the two pages comprising the
 * physical memory of the virtual machine; hence, it is page-sized
 * and page-aligned. The alignment is given by the strange looking
 * "__attribute__ ((...))" syntax, which is a gcc extension.
 */

/*unsigned char code[4096] __attribute__ ((aligned(4096))) = {
	0xb0, 0x60,								// mov		$0x60,%al
	0xe6, 0x64,								// out		%al,$0x64
	0xe6, 0x60,								// out		%al,$0x60

//wait_char:
	0xe4, 0x64,								// in		$0x64,%al
	0xa8, 0x01,								// test		$0x1,%al
	0x74, 0xfa,								// je		6 <wait_char>
	0xe4, 0x60,								// in		$0x60,%al
	0xf4,									// hlt		(diciamo al vm monitor di prelevare il risultato)
	0xeb, 0xf5,								// jmp		6 <wait_char>
};*/

unsigned char code[4096] __attribute__ ((aligned(4096)));

/* the following array will become the other page of memory for the
 * machine. The code above uses it to contain the stack.
 */
char data[4096] __attribute__ ((aligned(4096)));

// tastiera emulata (frontend)
keyboard keyb;

// gestione input console (backend)
ConsoleInput* console;

void initIO()
{
	// colleghiamo la tastiera emulata all'input della console
	console = ConsoleInput::getInstance();
	console->attachKeyboard(&keyb);

	// avviamo il thread che si occuperà di gestire l'input della console
	console->startEventThread();
}

void endIO()
{
	// questa operazione va fatta perchè altrimenti la console
	// non tornebbe nello stato di funzionamento precedente
	// all'instanziazione dell'oggetto ConsoleInput
	console->resetConsole();
}

// funzione chiamata su HLT del programma della vm per ottenere
// un risultato dal programma
void fetch_application_result(int vcpu_fd, kvm_run *kr) {
	/* we can obtain the the contents of all the registers
	 * in the vm.
	 */
	kvm_regs regs;
	if (ioctl(vcpu_fd, KVM_GET_REGS, &regs) < 0) {
		cerr << "get regs: " << strerror(errno) << endl;
		return;
	}
	/* (this is for the general purpose registers, we can also
	 * obtain the 'special registers' with KVM_GET_SREGS).
	 */

	// dobbiamo leggere al, quindi eliminiamo la parte restante da rax
	cout << "Risultato programma (keycode): " << (regs.rax & 0xff) << endl;
}

extern void estrai_segmento(char *fname, void *dest);
int main()
{
	/* the first thing to do is to open the /dev/kvm pseudo-device,
	 * obtaining a file descriptor.
	 */
	int kvm_fd = open("/dev/kvm", O_RDWR);
	if (kvm_fd < 0) {
		/* as usual, a negative value means error */
		cerr << "/dev/kvm: " << strerror(errno) << endl;
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
		cerr << "create vm: " << strerror(errno) << endl;
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

	// carichiamo l'eseguibile da file
	char elf_file[] = "prog_prova";
	estrai_segmento(elf_file, (void*)code);

	/* This is the descriptor for the 'data' page.
	 * We want to put this page at guest physical address 0
	 */
	kvm_userspace_memory_region mrd = {
		0,					// slot
		0,					// no flags,
		0,					// guest physical addr
		4096,					// memory size
		reinterpret_cast<__u64>(data)		// userspace addr
	};
	/* now we can add the memory to the vm */
	if (ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &mrd) < 0) {
		cerr << "set memory (data): " << strerror(errno) << endl;
		return 1;
	}
	/* note that the memory is shared between us and the vm.
	 * Whatever we write in the 'data' array above will be seen
	 * by the vm and, vice-versa, whatever the vm writes
	 * in its first "physical" page we can read in the in the
	 * 'data' array. We can even do this concurrently, if we
	 * use several threads.
	 */

	/* now we add the other page of memory, the one with the code.
	 * The reason for the strange guest physical address is explained
	 * below.
	 */
	kvm_userspace_memory_region mrc = {
		1,					// slot
		0,					// no flags,
		0xfffff000,				// guest physical addr
		4096,					// memory size
		reinterpret_cast<__u64>(code)		// userspace addr
	};

	if (ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &mrc) < 0) {
		cerr << "set memory (code): " << strerror(errno) << endl;
		return 1;
	}

	/* now we add a virtual cpu (vcpu) to our machine. We obtain yet
	 * another open file descriptor, which we can use to
	 * interact with the vcpu. Note that we can have several
	 * vcpus, to emulate a multi-processor machine.
	 */
	int vcpu_fd = ioctl(vm_fd, KVM_CREATE_VCPU, 0);
	if (vcpu_fd < 0) {
		cerr << "create vcpu: " << strerror(errno) << endl;
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
		cerr << "get mmap size: " << strerror(errno) << endl;
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
		cerr << "mmap: " << strerror(errno) << endl;
		return 1;
	}

	// a questo punto possiamo inizializzare le strutture per l'emulazione dei dispositivi di IO
	initIO();

	/* When we boot our virtual machine, it tries to fetch the first
	 * instruction at physical address 0xfffffff0, like a real x86
	 * machine. We have put our 'code' array at guest
	 * physical address 0xfffff000, but the instructions we want
	 * to execute are at the beginning, while the first instruction
	 * fetched starts at position 0xff0 inside the code array.
	 * Here, we place a jump back to the start of the page.
	 */
	code[0xff0] = 0xe9; // jmp
	/* the jmp needs the offset, in bytes, from the first
	 * byte after the jmp instruction itself to the target.
	 * The jmp is three bytes long, so the first byte after
	 * it is at guest physical address 0xfffffff3.
	 * The target is at 0xfffff000, so there are 0xff3 bytes
	 * between them. We need to jump back, so we need -0xff3,
	 * in 2's-complement on 16 bits: this is 0xf00d.
	 */
	code[0xff1] = 0x0d; /* recall that the machine is little endian:
				 * lowest byte goes first
				 */
	code[0xff2] = 0xf0;

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
			cerr << "run: " << strerror(errno) << endl;
			return 1;
		}

		switch(kr->exit_reason)
		{
			case KVM_EXIT_HLT:
				fetch_application_result(vcpu_fd, kr);
				break;
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
				else
				{
					cerr << "kvm: Unhandled VM IO: " <<  ((kr->io.direction == KVM_EXIT_IO_IN)?"IN":"OUT") << " on kr->io.port " << kr->io.port << endl;
					break;
				}
				break;
			}
			case KVM_EXIT_FAIL_ENTRY:
				cerr << "kvm: KVM_EXIT_FAIL_ENTRY reason=" << (unsigned long long)kr->fail_entry.hardware_entry_failure_reason << endl;
				return 1;
				break;
			case KVM_EXIT_INTERNAL_ERROR:
				cerr << "kvm: KVM_EXIT_INTERNAL_ERROR suberror=" << kr->internal.suberror << endl;
				return 1;
				break;
			default:
				cerr << "kvm: Unhandled VM_EXIT reason=" << kr->exit_reason << endl;
				return 1;
		}
	}

	// procediamo con la routine di ripristino dell'IO
	endIO();

	return 0;
}