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

const uint32_t USER_CODE_START = 0xfffff000;

unsigned char code[4096] __attribute__ ((aligned(4096)));

/* the following array will become the other page of memory for the
 * machine. The code above uses it to contain the stack.
 */
unsigned char data[4096] __attribute__ ((aligned(4096)));

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
	cout << "Risultato programma (keycode): " << regs.rax << endl;
}

void trace_user_program(int vcpu_fd, kvm_run *kr) {
	kvm_regs regs;
	if (ioctl(vcpu_fd, KVM_GET_REGS, &regs) < 0) {
		cerr << "get regs: " << strerror(errno) << endl;
		return;
	}

	printf("RIP: %x\n", (unsigned int)regs.rip);
	printf("RSP: %x\n", (unsigned int)regs.rsp);
}

/* CR0 bits */
#define CR0_PE 1
#define CR0_MP (1 << 1)
#define CR0_EM (1 << 2)
#define CR0_TS (1 << 3)
#define CR0_ET (1 << 4)
#define CR0_NE (1 << 5)
#define CR0_WP (1 << 16)
#define CR0_AM (1 << 18)
#define CR0_NW (1 << 29)
#define CR0_CD (1 << 30)
#define CR0_PG (1 << 31)

void fill_segment_descriptor(uint64_t *dt, struct kvm_segment *seg)
{
	uint16_t index = seg->selector >> 3;
	uint32_t limit = seg->g ? seg->limit >> 12 : seg->limit;
	dt[index] = (limit & 0xffff) /* Limit bits 0:15 */
		| (seg->base & 0xffffff) << 16 /* Base bits 0:23 */
		| (uint64_t)seg->type << 40
		| (uint64_t)seg->s << 44 /* system or code/data */
		| (uint64_t)seg->dpl << 45 /* Privilege level */
		| (uint64_t)seg->present << 47
		| (limit & 0xf0000ULL) << 48 /* Limit bits 16:19 */
		| (uint64_t)seg->avl << 52 /* Available for system software */
		| (uint64_t)seg->l << 53 /* 64-bit code segment */
		| (uint64_t)seg->db << 54 /* 16/32-bit segment */
		| (uint64_t)seg->g << 55 /* 4KB granularity */
		| (seg->base & 0xff000000ULL) << 56; /* Base bits 24:31 */
}

static void setup_protected_mode(int vcpu_fd , unsigned char *data_mem, uint64_t entry_point)
{
	kvm_sregs sregs;
	if (ioctl(vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
		perror("KVM_GET_SREGS:");
		exit(1);
	}

	kvm_segment seg;
	seg.base = 0;
	seg.limit = 0xffffffff;
	seg.present = 1;
	seg.dpl = 0;
	seg.db = 1;
	seg.s = 1; /* Code/data */
	seg.l = 0;
	seg.g = 1; /* 4KB granularity */

	uint64_t *gdt;

	sregs.cr0 |= CR0_PE; /* enter protected mode */
	sregs.gdt.base = 0x1000;
	sregs.gdt.limit = 3 * 8 - 1;

	gdt = (uint64_t *)(data_mem + sregs.gdt.base);
	/* gdt[0] is the null segment */

	seg.type = 11; /* Code: execute, read, accessed */
	seg.selector = 1 << 3;
	fill_segment_descriptor(gdt, &seg);
	sregs.cs = seg;

	seg.type = 3; /* Data: read/write, accessed */
	seg.selector = 2 << 3;
	fill_segment_descriptor(gdt, &seg);
	sregs.ds = sregs.es = sregs.fs = sregs.gs
		= sregs.ss = seg;

	if (ioctl(vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
		perror("KVM_SET_SREGS:");
		exit(1);
	}

	/* Clear all FLAGS bits, except bit 1 which is always set. */
	kvm_regs regs;
	memset(&regs, 0, sizeof(regs));

	regs.rflags = 2;
	regs.rip = entry_point;

	if (ioctl(vcpu_fd, KVM_SET_REGS, &regs) < 0) {
		perror("KVM_SET_REGS: ");
		exit(1);
	}
}

extern uint32_t estrai_segmento(char *fname, void *dest, uint64_t dest_offset);
int main(int argc, char **argv)
{
	// controllo parametri in ingresso
	if(argc != 2) {
		cout << "Formato non corretto. Uso: kvm <elf file>" << endl;
		return 1;
	}

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
	uint32_t entry_point = estrai_segmento(elf_file_path, (void*)code, USER_CODE_START);

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
		USER_CODE_START,				// guest physical addr
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

	cout << endl << "================== Memory Dump ==================" << endl;
	for(int i=0; i<4096; i++)
		printf("%x",((unsigned char*)code)[i]);
	cout << endl << "=================================================" << endl;

	//passiamo alla modalità protetta
	setup_protected_mode(vcpu_fd, data, entry_point);

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
				else if (kr->io.size == 1 && kr->io.count == 1 && kr->io.port == 0x02F8 && kr->io.direction == KVM_EXIT_IO_OUT)
				{
					// usato per debuggare i programmi
					printf("kvm: Risultato su Porta parallela: %d\n", *io_param);
				}
				else
				{
					cerr << "kvm: Unhandled VM IO: " <<  ((kr->io.direction == KVM_EXIT_IO_IN)?"IN":"OUT") << " on kr->io.port " << kr->io.port << endl;
					break;
				}
				break;
			}
			case KVM_EXIT_MMIO:
				cerr << "kvm: KVM_EXIT_MMIO"
						<< " address=" << std::hex << (uint64_t)kr->mmio.phys_addr
						<< " len=" << (uint32_t)kr->mmio.len
						<< " data=" << (uint32_t)((kr->mmio.data[3] << 24) | (kr->mmio.data[2] << 16) | (kr->mmio.data[1] << 8) | kr->mmio.data[0])
						<< " is_write=" << (short)kr->mmio.is_write << endl;
				trace_user_program(vcpu_fd, kr);
				return 1;
				break;
			case KVM_EXIT_FAIL_ENTRY:
				cerr << "kvm: KVM_EXIT_FAIL_ENTRY reason=" << (unsigned long long)kr->fail_entry.hardware_entry_failure_reason << endl;
				trace_user_program(vcpu_fd, kr);
				return 1;
				break;
			case KVM_EXIT_INTERNAL_ERROR:
				cerr << "kvm: KVM_EXIT_INTERNAL_ERROR suberror=" << kr->internal.suberror << endl;
				trace_user_program(vcpu_fd, kr);
				return 1;
				break;
			default:
				cerr << "kvm: Unhandled VM_EXIT reason=" << kr->exit_reason << endl;
				trace_user_program(vcpu_fd, kr);
				return 1;
		}
	}

	// procediamo con la routine di ripristino dell'IO
	endIO();

	return 0;
}