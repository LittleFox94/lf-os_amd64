# Memory management. Some notes

* allocators required for
  - physical memory pages
    + continuous
    + regions (DMA needs low physical addresses)
  - virtual memory pages
    + per context
    + regions (kernel heap, user heap, shared memory, libs, ....)

## data structures

### Physical memory pages

Linked list of regions. Region: start address, number of pages, current state

### Virtual memory pages

* could use the paging structures
  - bits are available in entries, could be used for details
* also could use another linked list of memory regions


Paging structures           | Extra linked list
============================|========================================
+ no extra data             | + can store anything and
  structure required        |   is easily extended
                            |
+ 14 bits available         | + fast to allocate continuos
  for details, should       |   chunks of memory
  be enough?                |
                            | - extra memory required
+ ideal for userspace brk() |
                            | - extra implementation required
+ paging structure has to   |
  be implemented anyways    | - over-complicated for userspace
                            |   brk()
- might not be fast enough  |
  to allocate continuous    |
  chunks of memory          |


## Virtual memory management

* easy mapping between physical and virtual address is required
  - we use the paging structures needed by the cpu to do memory management
  - the CPU always wants physical addresses in the `next_base` part of each entry
  - we need the virtual addresses for those physical addresses
* some operating systems map all physical memory in kernel space
  - accessing physical memory addresses is then possible by just adding the start address of that region
  - quite a lot of memory required when using 4k pages ...^^'
  - large page mapping required


## Kernel memory map

* addresses are given relative to the start of the higher half on 48bit AMD64, `0xFFFF800000000000`.
* the regions are defined in `src/kernel/arch/amd64/vm.h`

Start address       |   Size    |   Description of usage
====================|===========|=============================================
                  0 |     16 MB | * scratchpad prepared by loader
0xFFFF800000000000  |           | * kernel stack at the top of this region
--------------------|-----------|---------------------------------------------
              16 MB |    128 MB | * kernel binary
0xFFFF800001000000  |           | * also some loader data like the loader
                    |           |   struct, memory map or init executables
--------------------|-----------|---------------------------------------------
             144 MB |    112 MB | * slab allocator 4k
0xFFFF800009000000  |           |
--------------------|-----------|---------------------------------------------
             256 MB |    768 MB | * _unused_
0xFFFF800010000000  |           |
--------------------|-----------|---------------------------------------------
               1 GB |     31 GB | * kernel heap
0xFFFF800040000000  |           |
--------------------|-----------|---------------------------------------------
              32 GB |    992 GB | * _unused_
0xFFFF800080000000  |           |
--------------------|-----------|---------------------------------------------
               1 TB |      3 TB | * _unused_
0xFFFF810000000000  |           |
--------------------|-----------|---------------------------------------------
               4 TB |      4 TB | * direct mapping of all physical memory
0xFFFF840000000000  |           |
