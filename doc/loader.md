# Loader internals

This defines what the loader does on AMD64, the only supported architecture for now. It's like the API spec for
the API between loader and kernel.

## Memory layout

All things are loaded at 0xFFFF800000000000, the start of the higher half on AMD64.

Elements are located next to each other, padded to whole 4k pages for easy remapping.

Things loaded from the hard disk are placed in memory as they are. The only exception to this is the kernel, which
has to be a valid ELF executable file interpreted by the loader when everything else was loaded successfully.

### List of elements

* Kernel (from harddisk)
  - the central part of LF OS
* 16MiB scratch pad for kernel
  - safely allocated and mapped to a sane region of physical memory.
  - mostly for memory management data structures to make use of all the memory
* Loader struct (generated)
  - format defined in loader.h
* n loaded files described in Loader struct
  - just the contents of the loaded files
