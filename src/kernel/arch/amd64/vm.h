#ifndef _VM_H_INCLUDED
#define _VM_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

struct region_t {
    const char* name;
    uint64_t start;
    uint64_t end;
};

static const region_t ALLOCATOR_REGION_NULL           { .name = "NULL", .start = 0, .end = 0 };

// x86-64 SysV ABI defines the stack has to be 16-bytes aligned "right before the call instruction", which
// pushes the return address onto the stack. Since we do not enter user code via call, we emulate this by
// misaligning by 8 bytes.
static const region_t ALLOCATOR_REGION_USER_STACK     { .name = "User stack",          .start = 0x00003F0000001000, .end = 0x00003FFFFFFFEFF8 };
static const region_t ALLOCATOR_REGION_USER_HARDWARE  { .name = "User MMIO",           .start = 0x00007F0000000000, .end = 0x00007FFFFFFFDFFF };
static const region_t ALLOCATOR_REGION_USER_IOPERM    { .name = "User ioperm bitmask", .start = 0x00007FFFFFFFE000, .end = 0x00007FFFFFFFFFFF };

static const region_t ALLOCATOR_REGION_SCRATCHPAD     { .name = "Kernel scratchpad", .start = 0xFFFFFFFF80000000, .end = 0xFFFFFFFF80FFFFFF };
static const region_t ALLOCATOR_REGION_KERNEL_BINARY  { .name = "Kernel binary",     .start = 0xFFFFFFFF81000000, .end = 0xFFFFFFFF88FFFFFF };
static const region_t ALLOCATOR_REGION_SLAB_4K        { .name = "4k slab allocator", .start = 0xFFFFFFFF89000000, .end = 0xFFFFFFFF8FFFFFFF };
static const region_t ALLOCATOR_REGION_KERNEL_HEAP    { .name = "Kernel heap",       .start = 0xFFFFFFFF90000000, .end = 0xFFFFFFFFFFFFFFFF };

// this one must be PML4 aligned! (PDP, PD and PT indexes must be zero for the start and 511 for the end)
// must also be the last region
static const region_t ALLOCATOR_REGION_DIRECT_MAPPING { .name = "Physical mapping",  .start = 0xFFFF800000000000, .end = 0xFFFF83FFFFFFFFFF };

// Some usage flags for pages
static const uint32_t PageUsageKernel          = 1;  //! Page is used by kernel, userspace otherwise
static const uint32_t PageUsageHardware        = 2;  //! Page accesses MMIO hardware, plain memory otherwise
static const uint32_t PageCoW                  = 4;  //! Copy page on fault and give RW access on new page, normal #PF logic otherwise
static const uint32_t PageSharedMemory         = 8;  //! Page is mapped in multiple processes as shared memory
static const uint32_t PageLocked               = 16; //! Page is locked and cannot be unmapped
static const uint32_t PageUsagePagingStructure = 32; //! Page is used as paging structure

// Sizes a page can have
static const uint8_t  PageSize4KiB = 0;
static const uint8_t  PageSize2MiB = 1;
static const uint8_t  PageSize1GiB = 2;

struct vm_table;
extern struct vm_table* VM_KERNEL_CONTEXT;

void init_vm(void);
void cleanup_boot_vm(void);

//! Like malloc but allocates full pages only. 16 byte data overhead.
void* vm_alloc(size_t size);

//! the matching free() like function for vm_alloc
void  vm_free(void* ptr);

struct vm_table* vm_context_new(void);

struct vm_table* vm_current_context(void);

void vm_context_activate(struct vm_table* context);

void vm_context_map(struct vm_table* context, uint64_t virt, uint64_t physical, uint8_t pat);
void vm_context_unmap(struct vm_table* context, uint64_t virt);

uint64_t vm_context_find_free(struct vm_table* context, region_t region, size_t num);

int   vm_table_get_free_index1(struct vm_table* table);
int   vm_table_get_free_index3(struct vm_table* table, int start, int end);

uint64_t vm_context_get_physical_for_virtual(struct vm_table* context, uint64_t virt);

uint64_t vm_context_alloc_pages(struct vm_table* context, region_t region, size_t num);

void vm_copy_range(struct vm_table* dst_ctx, struct vm_table* src_ctx, uint64_t addr, size_t size);

//! Map a given memory area in the currently running userspace process at a random location
uint64_t vm_map_hardware(uint64_t hw, size_t len);

#endif
