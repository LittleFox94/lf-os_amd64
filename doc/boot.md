# Booting LF OS

This document describes how LF OS boots (or will be, not everything is there at the time of writing
this document).

## The firmware: UEFI

This is actually out of scope for LF OS, but still might be interesting.

In the beginning is the firmware. It initializes the CPU (loads microcode) and memory (RAM does not
work before that, the firmware uses the cache as main memory) and some other hardware, mostly
keyboard and at least one block device to boot from.

UEFI loads files, so it searches every block devide for an EFI system partition (identified by a
specific GUID as partition type in the GPT) and looks for bootable EFI files. It eventually loads
and executes the LF OS loader, bringing us one step closer to LF OS running.

## The loader

The LF OS loader identifies the EFI system partition it was loaded from and loads all files from
`$ESP\LFOS`. A file named `kernel` is handled specially as it is parsed as an ELF binary and loaded
to `0xFFFF800001000000`. This file will later be executed.

After that, the loader retrieves the memory map and transforms it to an LF OS loader memory map as
defined in `loader.h`. It also prepares a framebuffer for the console and a new virtual memory mapping.

When all those things are done, a lot of information is dumped in the `LoaderStruct` (defined in
`loader.h`) and the kernels `_start` is called with the pointer to the `LoaderStruct` as argument. The
loader is useless from now on.

## The kernel

This is the only component running until the computer is shutdown (or rebooted). It does some lowlevel
initialization (make a framebuffer text console, display boot logo, initialize memory management and
scheduler, ...) and then executes the file named `init` (hopefully loaded by the loader).

The kernel is then only active while handling interrupt and syscalls. Let's just hope one of the interrupts
is not going to be an exception, as this would trigger a bluescreen ...

## init

Woah, we are already in userspace. `init` is the first program started by LF OS. It can do anything
it wants since it has root access level. But since LF OS is a microkernel based operating system, `init`
cannot do anything easily. It has do provide a root filesystem and start more programs.

Fun fact: Linux has a so-called initramfs, which is a simple filesystem served from memory after being
loaded. This filesystem contains a binary called init which just has to call other programs (and may
load more drivers). Linux init always has a filesystem available to it. LF OS init, on the other hand,
has not. In LF OS, init has to provide the filesystem. So in Linux init is inside the initramfs, in LF OS
the initramfs is inside init.

A good init may do the following things:

* register itself as filesystem driver for `/`
* have filesystem in the binary
* call driver programs from that filesystem
* call a service manager from that filesystem

LF OS also has no concept of switching roots, but the root filesystem may be different for every process.
