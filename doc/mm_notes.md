# Memory management. Some notes

* allocators required for
  - physical memory pages
  - virtual memory pages (kernel)
  - virtual memory pages (per process)

## data structures

### Physical memory pages

Linked list of regions. Region: start address, number of pages, current state

### Virtual memory pages

* could use the paging structures
  - bits are available in entries, could be used for details
* also could use another linked list of memory regions


Paging structures           | Extra linked list
-......---------------------|-----------------------------
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
  to allocate continuos     |
  chunks of memory          |
