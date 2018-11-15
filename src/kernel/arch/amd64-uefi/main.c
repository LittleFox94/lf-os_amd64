#include "config.h"

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

char* LAST_INIT_STEP;

#define INIT_STEP(message, code)                                                \
    LAST_INIT_STEP = message;                                                   \
    fbconsole_write("   " message);                                             \
    code                                                                        \
    fbconsole.current_col = 0;                                                  \
    fbconsole_write("\e[38;2;109;128;255mok\e[38;5;15m\n");

extern void jump_higher_half(ptr_t newBase, ptr_t base);

extern const char* build_id;

void initialize_console() {
    //UINTN width  = gop->Mode->Info->HorizontalResolution;
    //UINTN height = gop->Mode->Info->VerticalResolution;
    //uint8_t* fb  = (uint8_t*)gop->Mode->FrameBufferBase;

    //fbconsole_init(width, height, fb);

    //#include "bootlogo.c"

    //for(int y = 0; y < gimp_image.height; y++) {
    //    for(int x = 0; x < gimp_image.width; x++) {
    //        int coord = ((y * gimp_image.width) + x) * gimp_image.bytes_per_pixel;
    //        fbconsole_setpixel(
    //            fbconsole.width - gimp_image.width + x - 5, y + 5,
    //            gimp_image.pixel_data[coord + 0],
    //            gimp_image.pixel_data[coord + 1],
    //            gimp_image.pixel_data[coord + 2]
    //        );
    //    }
    //}

    //fbconsole_write("   Initializing framebuffer console @ 0x%x", (long)fb);
}

void _start() {
    init_gdt();
    init_sc();

    INIT_STEP(
        "Initializing physical memory management",

//        uint64_t pages_free         = 0;
//        EFI_MEMORY_DESCRIPTOR* desc = memory_map;
//
//        while((void*)desc < (void*)memory_map + memory_map_size) {
//            if(desc->Type == EfiConventionalMemory) {
//                pages_free += desc->NumberOfPages;
//                mm_mark_physical_pages(desc->PhysicalStart, desc->NumberOfPages, MM_FREE);
//            }
//
//            desc = (void*)desc + descriptor_size;
//        }
//        fbconsole_write(" -> %u pages (%B) free", pages_free, pages_free * 4096);
    )

    INIT_STEP(
        "Initializing virtual memory management",
        init_vm();
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
        vm_table_t* init_context = vm_context_new();
        memcpy(init_context, VM_KERNEL_CONTEXT, 4096);

//        ptr_t entry = load_elf((ptr_t)initrd_buffer, init_context);
//        start_task(init_context, entry);
    )

    asm("sti");
    while(1);
}
