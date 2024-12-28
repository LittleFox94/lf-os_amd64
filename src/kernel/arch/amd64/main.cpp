#include <config.h>

#include <loader.h>

#include <panic.h>
#include <mm.h>
#include <vm.h>
#include <fbconsole.h>
#include <sd.h>
#include <sc.h>
#include <elf.h>
#include <scheduler.h>
#include <string.h>
#include <pic.h>
#include <pit.h>
#include <slab.h>
#include <log.h>
#include <efi.h>
#include <mq.h>

char* LAST_INIT_STEP;
extern const char *build_id;

#define INIT_STEP(message, code) \
    LAST_INIT_STEP = (char*)message;    \
    code                         \
    logi("kernel", message);

void nyi(int loop);
void bootstrap_globals(void);
void init_console(struct LoaderStruct* loaderStruct);
void init_mm(struct LoaderStruct* loaderStruct);
void init_symbols(struct LoaderStruct* loaderStruct);
void init_init(struct LoaderStruct* loaderStruct);

__attribute__ ((force_align_arg_pointer))
extern "C" void main(struct LoaderStruct* loaderStruct) {
    logd("kernel", "Hello world!");

    bootstrap_globals();
    init_console(loaderStruct);

    logi("kernel", "LF OS for amd64. Build: %s", build_id);
    logi("kernel", "Initializing subsystems");

    INIT_STEP(
        "Initialized physical memory management",
        init_mm(loaderStruct);
    )

    INIT_STEP(
        "Initialized virtual memory management",
        init_vm();
    )

    INIT_STEP(
        "Initialized framebuffer console backbuffer",
        fbconsole_init_backbuffer();
    )

    INIT_STEP(
        "Initialized UEFI runtime services",
        init_efi(loaderStruct);
    )

    INIT_STEP(
        "Initialized service registry",
        init_sd();
    )

    INIT_STEP(
        "Initialized mutex subsystem",
        init_mutex();
        init_condvar();
    )

    INIT_STEP(
        "Initialized message queue subsystem",
        init_mq(&kernel_alloc);
    )

    INIT_STEP(
        "Initialized syscall interface",
        init_gdt();
        init_sc();
    )

    INIT_STEP(
        "Initialized scheduling and program execution",
        init_scheduler();
    )

    INIT_STEP(
        "Initialized interrupt management",
        init_pic();
        init_pit();
    )

    INIT_STEP(
        "Read kernel symbols",
        init_symbols(loaderStruct);
    )

    INIT_STEP(
        "Cleaned bootup memory structures",
        cleanup_boot_vm();
    )

    INIT_STEP(
        "Prepared userspace",
        init_init(loaderStruct);
    )

    logi("kernel", "Kernel initialization complete");

    LAST_INIT_STEP = (char*)"Kernel initialization complete";
    asm("sti");
    asm("jmp ."); // guess what: `while(1);` is Undefined Behavior in C++ and silently dropped (:
}

void init_console(struct LoaderStruct* loaderStruct) {
    fbconsole_init(loaderStruct->fb_width, loaderStruct->fb_height, loaderStruct->fb_stride, (uint8_t*)loaderStruct->fb_location);

    #include "../../bootlogo.cpp"
    fbconsole_blt(lf_os_bootlogo.pixel_data, lf_os_bootlogo.width, lf_os_bootlogo.height, -((int)lf_os_bootlogo.width + 5), 5);
}

void init_mm(struct LoaderStruct* loaderStruct) {
    logd("main.c", "loaderStruct->num_mem_desc: %lu", loaderStruct->num_mem_desc);

    struct MemoryRegion* memoryRegions = (struct MemoryRegion*)((uint64_t)loaderStruct + loaderStruct->size);

    SlabHeader* scratchpad_allocator = (SlabHeader*)ALLOCATOR_REGION_SCRATCHPAD.start;
    init_slab(ALLOCATOR_REGION_SCRATCHPAD.start, ALLOCATOR_REGION_SCRATCHPAD.end, 4096);

    mm_bootstrap(slab_alloc(scratchpad_allocator));

    uint64_t pages_free = 0;
    uint64_t pages_firmware = 0;

    char desc_status[81];
    memset(desc_status, 0, 81);

    for(size_t i = 0; i < loaderStruct->num_mem_desc; ++i) {
        struct MemoryRegion* desc = memoryRegions + i;

        if(desc->flags & MEMORY_REGION_USABLE) {
            desc_status[i % 80] = '-';
            pages_free += desc->num_pages;
            mm_mark_physical_pages(desc->start_address, desc->num_pages, MM_FREE);
        }
        else if(desc->flags & MEMORY_REGION_FIRMWARE) {
            desc_status[i % 80] = 'F';
            pages_firmware += desc->num_pages;
        }
        else {
            desc_status[i % 80] = 'X';
        }

        if((i % 80) == 79) {
            logd("mm", "memory descriptor status: %s", desc_status);
            memset(desc_status, 0, 81);
        }
    }

    logd("mm", "memory descriptor status: %s", desc_status);

    logi("mm", "%u pages (%B) free, %u (%B) firmware runtime memory",
        pages_free,     pages_free * 4096,
        pages_firmware, pages_firmware* 4096
    );
}

void init_symbols(struct LoaderStruct* loaderStruct) {
    struct FileDescriptor* fileDescriptors = (struct FileDescriptor*)((uint64_t)loaderStruct + loaderStruct->size + (loaderStruct->num_mem_desc * sizeof(struct MemoryRegion)));

    for(size_t i = 0; i < loaderStruct->num_files; ++i) {
        struct FileDescriptor* desc = (fileDescriptors + i);
        uint64_t kernel                = (uint64_t)loaderStruct + desc->offset;

        if(strcasecmp(desc->name, "kernel") == 0) {
            kernel_symbols = elf_load_symbols(kernel, &kernel_alloc);
        }
    }
}

void init_init(struct LoaderStruct* loaderStruct) {
    struct FileDescriptor* fileDescriptors = (struct FileDescriptor*)(
        (uint64_t)loaderStruct +
        loaderStruct->size +
        (loaderStruct->num_mem_desc * sizeof(struct MemoryRegion))
    );

    for(size_t i = 0; i < loaderStruct->num_files; ++i) {
        struct FileDescriptor* desc = (fileDescriptors + i);
        void*                  data = (uint8_t*)((uint64_t)loaderStruct + desc->offset);

        if(strcasecmp(desc->name, "kernel") != 0) {
            struct vm_table* context = vm_context_new();

            uint64_t data_start = 0;
            uint64_t data_end   = 0;
            uint64_t entrypoint = load_elf((uint64_t)data, context, &data_start, &data_end);

            if(!entrypoint) {
                logd("init" "Failed to run '%s'", desc->name);
            }

            start_task(context, entrypoint, data_start, data_end, desc->name);
        }
    }
}

void bootstrap_globals(void) {
    VM_KERNEL_CONTEXT = vm_current_context();
}

/* noinline to have a default stop point for profiling */
__attribute__((noinline)) void nyi(int loop) {
    if(loop) {
        panic_message("Not yet implemented");
    }
    else {
        fbconsole_write("\n\x1b[38;5;9mNot yet implemented. Continuing");
    }
}
