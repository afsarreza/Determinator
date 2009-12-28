#if LAB >= 1
// Kernel-private virtual memory definitions.
// See COPYRIGHT for copyright information.
#ifndef PIOS_KERN_VM_H
#define PIOS_KERN_VM_H
#ifndef PIOS_KERNEL
# error "This is a PIOS kernel header; user programs should not #include it"
#endif


// Global segment descriptor numbers used by the kernel
#define GD_TSS    0x08     // Task segment selector
#define GD_KT     0x10     // kernel text
#define GD_KD     0x18     // kernel data
#define GD_UT     0x20     // user text
#define GD_UD     0x28     // user data


/*
 * Kernel-private virtual memory map:                 Permissions
 *                                                    kernel/user
 *
 *    4 Gig ---------> +==============================+
 *                     |                              | RW/--
 *                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
 *                     :              .               :
 *                     :              .               :
 *                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
 *                     |                              | RW/--
 *                     |   Remapped Physical Memory   | RW/--
 *                     |      (up to 1GB usable)      | RW/--
 *                     |                              | RW/--
 *    VM_PHYSLO -----> +------------------------------+ 0xc0000000
 *                     |                              | RW/--
 *                     |  Kernel stacks, one per CPU  | RW/--
 *    VM_KSTACKS,      |                              | RW/--
 *    VM_KERNLO -----> +==============================+ 0x80000000
 *                     |                              | RW/RW
 *                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
 *                     :              .               :
 *                     :              .               :
 *                     |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
 *                     |                              | RW/RW
 *                     |      User process area       | RW/RW
 *                     |        (see inc/vm.h)        | RW/RW
 *                     |                              | RW/RW
 *    0 -------------> +==============================+
 *
 * (*) Note: "Unmapped Memory" is normally unmapped, but user programs may
 *     map pages there if desired.
 */


// All usable physical memory is mapped starting at this address (3GB).
// Since 1GB of our 4GB of total addressable space is above VM_PHYSLO,
// this means well be able to use up to 1GB of physical memory;
// any physical memory above that point will be wasted.
#define	VM_PHYSLO	0xc0000000


// We map kernel-mode (ring 0) stacks starting at this point,
// one kernel stack per CPU in a multiprocessor or multicore system.
// Although we could just use remapped physical memory above VM_PHYSLO,
// placing the kernel stacks in virtual memory allows us
// to place a "red zone" below each stack and catch stack overflows.
// This way, kernel stack overflows cause an immediate double fault
// instead of potentially much more subtle memory corruption failures.
#define VM_KSTACKS	0x80000000

// Each kernel stack consists of KSTACKRED bytes of unmapped address space
// followed by KSTACKSIZE bytes of usable memory mapped read/write,
// for a total of KSTACKSPACE bytes of address space per stack (i.e., per CPU).
#define VM_KSTACKSIZE	(8*PGSIZE)
#define VM_KSTACKRED	(8*PGSIZE)
#define VM_KSTACKSPACE	(VM_KSTACKRED+VM_KSTACKSIZE)


#endif /* !PIOS_KERN_VM_H */
#endif // LAB >= 1