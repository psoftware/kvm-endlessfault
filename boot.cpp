#include "boot.h"

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

/* CR4 bits */
#define CR4_VME 1
#define CR4_PVI (1 << 1)
#define CR4_TSD (1 << 2)
#define CR4_DE (1 << 3)
#define CR4_PSE (1 << 4)
#define CR4_PAE (1 << 5)
#define CR4_MCE (1 << 6)
#define CR4_PGE (1 << 7)
#define CR4_PCE (1 << 8)
#define CR4_OSFXSR (1 << 8)
#define CR4_OSXMMEXCPT (1 << 10)
#define CR4_UMIP (1 << 11)
#define CR4_VMXE (1 << 13)
#define CR4_SMXE (1 << 14)
#define CR4_FSGSBASE (1 << 16)
#define CR4_PCIDE (1 << 17)
#define CR4_OSXSAVE (1 << 18)
#define CR4_SMEP (1 << 20)
#define CR4_SMAP (1 << 21)

#define EFER_SCE 1
#define EFER_LME (1 << 8)
#define EFER_LMA (1 << 10)
#define EFER_NXE (1 << 11)

/* 32-bit page directory entry bits */
#define PDE32_PRESENT 1
#define PDE32_RW (1 << 1)
#define PDE32_USER (1 << 2)
#define PDE32_PS (1 << 7)

/* 64-bit page * entry bits */
#define PDE64_PRESENT 1
#define PDE64_RW (1 << 1)
#define PDE64_USER (1 << 2)
#define PDE64_ACCESSED (1 << 5)
#define PDE64_DIRTY (1 << 6)
#define PDE64_PS (1 << 7)
#define PDE64_G (1 << 8)

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

void setup_protected_mode(int vcpu_fd , unsigned char *data_mem, uint64_t entry_point)
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

void setup_64bit_code_segment(unsigned char *data_mem, struct kvm_sregs *sregs)
{
	kvm_segment seg;
	seg.base = 0;
	seg.limit = 0xffffffff;
	seg.selector = 3 << 3;
	seg.present = 1;
	seg.type = 11; /* Code: execute, read, accessed */
	seg.dpl = 0;
	seg.db = 0;
	seg.s = 1; /* Code/data */
	seg.l = 1;
	seg.g = 1; /* 4KB granularity */
	uint64_t *gdt = reinterpret_cast<uint64_t *>(reinterpret_cast<uint64_t>(data_mem) + sregs->gdt.base);

	sregs->gdt.limit = 4 * 8 - 1;

	fill_segment_descriptor(gdt, &seg);
}

void setup_long_mode(int vcpu_fd , unsigned char *data_mem)
{
	kvm_sregs sregs;
	if (ioctl(vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
		perror("KVM_GET_SREGS:");
		exit(1);
	}

	uint64_t pml4_addr = 0x2000;
	uint64_t *pml4 = reinterpret_cast<uint64_t *>(reinterpret_cast<uint64_t>(data_mem) + pml4_addr);

	uint64_t pdpt_addr = 0x3000;
	uint64_t *pdpt = reinterpret_cast<uint64_t *>(reinterpret_cast<uint64_t>(data_mem) + pdpt_addr);

	uint64_t pd_addr = 0x4000;
	uint64_t *pd = reinterpret_cast<uint64_t *>(reinterpret_cast<uint64_t>(data_mem) + pd_addr);

	pml4[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pdpt_addr;
	pdpt[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pd_addr;
	pd[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | PDE64_PS;

	sregs.cr3 = pml4_addr;
	sregs.cr4 = CR4_PAE;
	sregs.cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM;
	sregs.efer = EFER_LME;

	/* We don't set cr0.pg here, because that causes a vm entry
	   failure. It's not clear why. Instead, we set it in the VM
	   code. */
	setup_64bit_code_segment(data_mem, &sregs);
}