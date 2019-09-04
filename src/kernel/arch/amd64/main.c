#include <config.h>

#include "../../../loader/loader.h"

#include "bluescreen.h"
#include "mm.h"
#include "vm.h"
#include "fbconsole.h"
#include "sd.h"
#include "sc.h"
#include "elf.h"
#include "scheduler.h"
#include "string.h"
#include "pic.h"
#include "pit.h"
#include "slab.h"

char* LAST_INIT_STEP;
extern char build_id[];

#define INIT_STEP(message, code)                                                \
    LAST_INIT_STEP = message;                                                   \
    fbconsole_write("\e[38;5;7m   " message);                                   \
    code                                                                        \
    fbconsole_write("\r\e[38;2;109;128;255mok\e[38;5;7m\n");

void nyi();
void bootstrap_globals();
void init_console(LoaderStruct* loaderStruct);
void init_console_backbuffer();
void init_mm(LoaderStruct* loaderStruct, MemoryRegion* memoryRegions);
void print_memory_regions();

void main(void* loaderData) {
    LoaderStruct* loaderStruct  = (LoaderStruct*)loaderData;
    MemoryRegion* memoryRegions = (MemoryRegion*)(loaderData + loaderStruct->size);

    bootstrap_globals();
    init_console(loaderStruct);
    print_memory_regions();

    fbconsole_write("\e[38;5;15mInitializing subsystems\n");

    INIT_STEP(
        "Initializing physical memory management",
        init_mm(loaderStruct, memoryRegions);
    )

    INIT_STEP(
        "Initializing virtual memory management",
        init_vm();
    )

    INIT_STEP(
        "Speed up framebuffer console",
        init_console_backbuffer(loaderStruct);
    )

    INIT_STEP(
        "Initializing service registry",
        init_sd();
    )

    INIT_STEP(
        "Initializing syscall interface",
        init_gdt();
        init_sc();
    )

    INIT_STEP(
        "Initializing scheduling and program execution",
        init_scheduler();
    )

    INIT_STEP(
        "Initializing interrupt management",
        init_pic();
        init_pit();
    )

    INIT_STEP(
        "Preparing and starting userspace",
        asm volatile("syscall"::"a"('A'), "d"(42));
//
//        nyi(1);
//        vm_table_t* init_context = vm_context_new();
//        memcpy(init_context, VM_KERNEL_CONTEXT, 4096);
//
//        ptr_t entry = load_elf((ptr_t)initrd_buffer, init_context);
//        start_task(init_context, entry);
    )

    asm("sti");
    while(1);
}

void init_console(LoaderStruct* loaderStruct) {
    fbconsole_init(loaderStruct->fb_width, loaderStruct->fb_height, (uint8_t*)loaderStruct->fb_location);

#include "../../bootlogo.c"

    uint16_t fbconsole_width = fbconsole_instance()->width;
    fbconsole_blt(lf_os_bootlogo.pixel_data, lf_os_bootlogo.width, lf_os_bootlogo.height, fbconsole_width - lf_os_bootlogo.width - 5, 5);

    fbconsole_write("LF OS for amd64. Build: %s\n", build_id);
    fbconsole_write("  framebuffer console @ 0x%x (0x%x)\n\n", (uint64_t)loaderStruct->fb_location, (uint64_t)vm_context_get_physical_for_virtual(VM_KERNEL_CONTEXT, loaderStruct->fb_location));
}

void init_console_backbuffer(LoaderStruct* loaderStruct) {
    size_t backbuffer_size      = loaderStruct->fb_width * loaderStruct->fb_height * 4;
    size_t backbuffer_num_pages = (backbuffer_size + 4095) / 4096;

    uint8_t* backbuffer = (uint8_t*)vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_KERNEL_HEAP, backbuffer_num_pages);
    fbconsole_init_backbuffer(backbuffer);
}

void init_mm(LoaderStruct* loaderStruct, MemoryRegion* memoryRegions) {
    SlabHeader* scratchpad_allocator = (SlabHeader*)ALLOCATOR_REGION_SCRATCHPAD.start;
    init_slab(ALLOCATOR_REGION_SCRATCHPAD.start, ALLOCATOR_REGION_SCRATCHPAD.end, 4096);

    mm_bootstrap(slab_alloc(scratchpad_allocator));

    uint64_t pages_free = 0;

    for(size_t i = 0; i < loaderStruct->num_mem_desc; ++i) {
        MemoryRegion* desc = memoryRegions + i;

        if(desc->flags & MEMORY_REGION_FREE) {
            pages_free += desc->num_pages;
            mm_mark_physical_pages(desc->start_address, desc->num_pages, MM_FREE);
        }
    }

    fbconsole_write(" %u pages (%B) free", pages_free, pages_free * 4096);
}

void bootstrap_globals() {
    asm("mov %%cr3, %0":"=r"(VM_KERNEL_CONTEXT));
}

void print_memory_regions() {
    fbconsole_write("Memory regions\n");
    AllocatorRegion* region = ALLOCATOR_REGIONS_KERNEL;

    while(region->start != 0 && region->end != 0) {
        fbconsole_write("  \e[38;5;15m%20s \e[38;5;7m(0x%x - 0x%x, \e[38;5;15m% 8B\e[38;5;7m)\n", region->name, region->start, region->end, region->end - region->start + 1);
        ++region;
    }

    fbconsole_write("\n");
}

/* noinline to have a default stop point for profiling */
__attribute__((noinline)) void nyi(int loop) {
    if(loop) {
        panic_message("Not yet implemented");
    }
    else {
        fbconsole_write("\n\e[38;5;9mNot yet implemented. Continuing");
    }
}
