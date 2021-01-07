<img align="right" height="100" src="LF OS.svg">

# LF OS

This is the 5th(?) attempt at writing an operating system from scratch, where "attempt" means: starting from zero.

## Key difference to previous attempts

* amd64, why use i386 like everyone else when I did not even manage to get a working system there?
* uefi and custom loader, why use Grub like everyone else when I did not even ...... ^
* I'm actually able to program things now
  - I actually became a good software developer in the meantime
  - first attempt was even before finishing school
* there are some docs
* I thought about system design and even wrote some things down
* I don't write throw-away code or just paste tutorials right now
  - maybe later in user space though

## Design

This will be a microkernel providing the following inside the kernel

* base system initialization (CPU, physical memory, common bus systems)
* memory management for processes (virtual memory)
* inter-process communication facilities
  - mostly via shared memory and message queues
  - later also via helper userspace programs to make communication between processes A and B over an arbitrary
    channel between C and D, where A, B, C and D may run on different computers
  - IPC between different computers will be transparent to processes in a later version
* service registry
  - processes announce services (file system, block device, character device, ...)
  - other processes need services
  - ask kernel for process implementing service x
  - IPC to process for service x

All drivers for special hardware (everything not attached to a common bus) are userspace programs that will be
designed to recover after crashed. They can store a small amount of information in the kernel (like hard disk
x is fully initialized, just read from address $y and port $x) and when they crash, they are just restarted.

Since IPC between processes running on different computers is planned to be transparent, it's actually
possible to run a single instance of LF OS on a whole data center (or bigger, but latency).
