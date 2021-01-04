# Loader interface

This defines what the loader does on AMD64, the only supported architecture for now. It's like the API spec for
the API between loader and kernel.

## Calling the kernel main function

The main function of the kernel is invoked using the SysV ABI for amd64 and with the following prototype:

```c
void _start(LoaderStruct* loaderStruct);
```

Interrupts are turned off, everything accessed by the kernel is located in the higher half. Some things
may be mapped in the lower half, for things required before the loader jumps to the kernel code.

## Memory layout

All things are loaded at 0xFFFF800000000000, the start of the higher half on AMD64.

Elements are located next to each other, padded to whole 4k pages for easy remapping.

Things loaded from the hard disk are placed in memory as they are. The only exception to this is the kernel, which
has to be a valid ELF executable file interpreted by the loader when everything else was loaded successfully.

### List of elements

* 16MiB scratch pad for kernel and kernel stack
  - safely allocated and mapped to a sane region of physical memory.
  - mostly for memory management data structures to make use of all the memory
  - stackpointer is set to the top of this area
    + remember the stack grows downwards!
* Kernel (from harddisk)
  - the central part of LF OS
* framebuffer
* firmware data
  - EFI\_SYSTEM\_TABLE + all UEFI data and code
* Loader struct (generated)
  - format defined in loader.h
* n loaded files described in Loader struct
  - just the contents of the loaded files
