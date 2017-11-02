#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <sys/mman.h>
#include "frontend/IODevice.h"

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
char code[4096] __attribute__ ((aligned(4096))) = {
	/* we setup a stack in the first physical page of the vm */
	0xbc, 0x00, 0x10,                       // mov    $0x1000,%sp
	/* then, we call the 'sum' function below, with arguments 2 and 3 */
	0x6a, 0x02,                             // push   $0x2
	0x6a, 0x03,                             // push   $0x3
	0xe8, 0x04, 0x00,                       // call   e <sum>
	/* after the return from the function, we clear up the stack
	 * and stop the (virtual) machine
	 */
	0x83, 0xc4, 0x08,                       // add    $0x8,%sp
	0xf4,                                   // hlt
	/* here begins the code of the sum function, which just returns
	 * the sum of its two arguments (received on the stack) in
	 * the %ax register
	 */
	0x55,                                   // push   %bp
	0x89, 0xe5,                             // mov    %sp,%bp
	0x8b, 0x46, 0x06,                       // mov    0x6(%bp),%ax
	0x8b, 0x56, 0x04,                       // mov    0x4(%bp),%dx
	0x01, 0xd0,                             // add    %dx,%ax
	0x5d,                                   // pop    %bp
	0xc3,                                   // ret
};

/* the following array will become the other page of memory for the
 * machine. The code above uses it to contain the stack.
 */
char data[4096] __attribute__ ((aligned(4096)));

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
	if (ioctl(vcpu_fd, KVM_RUN, 0) < 0) {
		cerr << "run: " << strerror(errno) << endl;
		return 1;
	}

	/* If we are here, the virtual machine is stopped, waiting
	 * for us to do something.
	 * The code we have prepared only exits because of the 'hlt'
	 * instruction. If everything is working correctly, the
	 * following instruction should print 'Exit reason: 5',
	 * since 5 is the code for KVM_EXIT_HLT
	 * (see the definitions in /usr/include/linux/kvm.h).
	 */
	cout << "Exit reason: " << kr->exit_reason << endl;

	/* we can also obtain the the contents of all the registers
	 * in the vm.
	 */
	kvm_regs regs;
	if (ioctl(vcpu_fd, KVM_GET_REGS, &regs) < 0) {
		cerr << "get regs: " << strerror(errno) << endl;
		return 1;
	}
	/* (this is for the general purpose registers, we can also
	 * obtain the 'special registers' with KVM_GET_SREGS).
	 */

	/* if everthing is well, we should find 5=2+3 in the %rax
	 * register (the register names follow the 64 bit convention;
	 * our %ax register is in the lowest part of %rax).
	 */
	cout << regs.rax << endl;

	return 0;
}
