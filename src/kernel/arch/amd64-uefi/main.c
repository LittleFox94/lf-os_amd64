#define GNU_EFI_USE_EXTERNAL_STDARG
#include "stdarg.h"
#include <efi.h>
#include <efilib.h>

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
                                                                                \
    if(single_stepping) {                                                       \
        unsigned char c;                                                        \
        do {                                                                    \
            asm volatile("inb $0x60, %0":"=r"(c));                              \
        } while(c != 0x1c);                                                     \
                                                                                \
        do {                                                                    \
            asm volatile("inb $0x60, %0":"=r"(c));                              \
        } while(c != 0x9c);                                                     \
    }                                                                           \
                                                                                \
    code                                                                        \
    fbconsole.current_col = 0;                                                  \
    fbconsole_write("\e[38;2;109;128;255mok\e[38;5;15m\n");

extern void jump_higher_half(ptr_t newBase, ptr_t base);

extern const char* build_id;

/** Load the initial ramdisk from the boot device.
 *
 * \param path Path to the initial ramdisk
 * \param buffer Output argument for the location where the ramdisk has been stored.
 * \returns size of the initial ramdisk or a negative value on error
 */
int64_t load_initrd(EFI_DEVICE_PATH* path, void** buffer) {
    if(path == NULL) {
        Print(L"Could not get path initrd\n");
        return EFI_NOT_FOUND;
    }

    EFI_HANDLE device_handle;
    SIMPLE_READ_FILE read_handle;

    EFI_STATUS status = OpenSimpleReadFile(FALSE, NULL, 0, &path, &device_handle, &read_handle);

    if(status == EFI_SUCCESS) {
        void* temp_buffer          = NULL;
        const unsigned buffer_size = 512;
        UINTN file_buffer_size     = 0;
        UINTN read_size            = buffer_size;
        UINTN file_size            = 0;

        while(read_size == buffer_size) {
            read_size        = buffer_size;
            file_buffer_size = file_size + buffer_size;
            temp_buffer      = ReallocatePool(temp_buffer, file_size, file_buffer_size);
            status           = ReadSimpleReadFile(read_handle, file_size, &read_size, temp_buffer + file_size);

            file_size += read_size;

            if(status == EFI_SUCCESS) {
                fbconsole_write(".");
            }
            else {
                fbconsole_write("x");
            }
        }

        fbconsole_write(" -> 0x%x (%u bytes, %B)", (long)temp_buffer, file_size, file_size);

        temp_buffer = ReallocatePool(temp_buffer, file_buffer_size, file_size);
        *buffer     = temp_buffer;
        return file_size;
    }
    else {
        return -status;
    }
}

EFI_STATUS initialize_console() {
    EFI_STATUS status;

    EFI_GUID gop_uuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

    UINTN handle_count        = 0;
    EFI_HANDLE* handle_buffer = NULL;
    status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &gop_uuid, NULL, &handle_count, &handle_buffer);

    if(status != EFI_SUCCESS) {
        return status;
    }
    else if(handle_count == 0) {
        return EFI_NOT_FOUND;
    }

    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    status = uefi_call_wrapper(BS->HandleProtocol, 2, handle_buffer[0], &gop_uuid, &gop);

    if(status != EFI_SUCCESS) {
        return status;
    }

    UINTN width  = gop->Mode->Info->HorizontalResolution;
    UINTN height = gop->Mode->Info->VerticalResolution;
    uint8_t* fb  = (uint8_t*)gop->Mode->FrameBufferBase;

    Print(L"  got UEFI GOP framebuffer %dx%d located at 0x%x @ 0x%x\n", width, height, (ptr_t)fb, mm_get_mapping((ptr_t)fb));
    fbconsole_init(width, height, fb);

    #include "bootlogo.c"

    for(int y = 0; y < gimp_image.height; y++) {
        for(int x = 0; x < gimp_image.width; x++) {
            int coord = ((y * gimp_image.width) + x) * gimp_image.bytes_per_pixel;
            fbconsole_setpixel(
                fbconsole.width - gimp_image.width + x - 5, y + 5,
                gimp_image.pixel_data[coord + 0],
                gimp_image.pixel_data[coord + 1],
                gimp_image.pixel_data[coord + 2]
            );
        }
    }

    fbconsole_write("   Initializing framebuffer console @ 0x%x", (long)fb);

    return EFI_SUCCESS;
}

void relocate_high(ptr_t imageBase, ptr_t imageSize) {
    ptr_t newImageBase = 0xFFFF800000000000; // start of higher half

    for(size_t i = 0; i < imageSize; i+= 4096) {
        vm_context_map(VM_KERNEL_CONTEXT, newImageBase + i, imageBase + i);
    }

    for(size_t i = 0; i < 1024 * 1024 * 1024; i+=4096) {
        vm_context_map(VM_KERNEL_CONTEXT, i, i);
    }

    for(size_t i = 0; i < fbconsole.width * fbconsole.height * 4; i+=4096) {
        vm_context_map(VM_KERNEL_CONTEXT, (ptr_t)(fbconsole.fb + i), (ptr_t)(fbconsole.fb + i));
    }

    vm_context_activate(VM_KERNEL_CONTEXT);
}

void settings_ui(EFI_SYSTEM_TABLE *system_table, ptr_t imageBase, bool* single_stepping) {
    int draw = 1;

    while(1) {
        if(draw) {
            uefi_call_wrapper(system_table->ConOut->ClearScreen, 1, system_table->ConOut);
            Print(L"LF OS amd64-uefi kernel - version: %a\n\n", build_id);

            Print(L"Kernel configuration flags:\n");
            Print(L"      init location:       $ESP%s\n", INIT_LOCATION);
            Print(L"      image base:          0x%x\n",   imageBase);
            Print(L" [S]  single stepping:     %a\n",     *single_stepping ? "on (press RETURN for stepping)" : "off");

            Print(L"\nPress key from first column to change setting.");
            Print(L"\nPress RETURN to boot kernel with the given settings.\n\n");

            Print(L"You can also attach the debugger now:\n");
            Print(L"    add-symbol-file main.efi.debug 0xFFFF800000003000\n");
            Print(L"    target remote :1234\n");
            Print(L"    continue\n");
        }

        unsigned char code = 0;
        asm volatile("inb $0x60, %0":"=r"(code));

        if(draw == code) {
            continue;
        }

        draw = code;

        switch(code) {
            case 0x1c:
                return;
            case 0x1F:
                *single_stepping = !*single_stepping;
                break;
            default:
                draw = 0;
        }
    }
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
    InitializeLib(image_handle, system_table);

    EFI_STATUS status;
    EFI_LOADED_IMAGE* li;
    ptr_t imageBase;
    size_t imageSize;

    uefi_call_wrapper(BS->HandleProtocol, 3, image_handle, &LoadedImageProtocol, (void**)&li);

    imageBase = (ptr_t)li->ImageBase;
    imageSize = li->ImageSize;

    bool single_stepping = false;

    settings_ui(system_table, imageBase, &single_stepping);

    INIT_STEP(
        "",
        Print(L"\nInitializing console ...\n");

        status = initialize_console();

        if(status != EFI_SUCCESS) {
            Print(L"  not able to get a console: %r\n", status);
            return EFI_UNSUPPORTED;
        }
    )

    INIT_STEP(
        "Loading init",
        EFI_DEVICE_PATH* init_path = FileDevicePath(li->DeviceHandle, INIT_LOCATION);

        if(!init_path) {
            fbconsole_write("\nCould not find init to load\n");
            while(1);
        }

        void*   initrd_buffer      = NULL;
        int64_t initrd_size        = load_initrd(init_path, &initrd_buffer);

        if(initrd_size < 0) {
            fbconsole_write("\nCould not load init: %d\n", -initrd_size);
            return -initrd_size;
        }
    )

    INIT_STEP(
        "Retrieving memory map",
        UINTN descriptor_size             = 0;
        UINT32 descriptor_version         = 0;
        UINTN map_key                     = 0;
        UINTN memory_map_size             = 128;
        EFI_MEMORY_DESCRIPTOR* memory_map = AllocatePool(memory_map_size);

        do {
            status = uefi_call_wrapper(BS->GetMemoryMap, 5, &memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);

            if(status == EFI_SUCCESS) {
                break;
            }

            if(status != EFI_BUFFER_TOO_SMALL) {
                fbconsole_write("\nUnexpected status from GetMemoryMap call: %d\n", status);
                while(1);
            }

            FreePool(memory_map);
            memory_map = AllocatePool(memory_map_size);
        } while(status == EFI_BUFFER_TOO_SMALL);
    )

    INIT_STEP(
        "Exiting UEFI boot services",
        asm volatile("cli");
        status = uefi_call_wrapper(BS->ExitBootServices, 2, image_handle, map_key);

        init_gdt();
        init_sc();
    )

    if(status == EFI_SUCCESS) {
        INIT_STEP(
            "Initializing physical memory management",

            uint64_t pages_free         = 0;
            EFI_MEMORY_DESCRIPTOR* desc = memory_map;

            while((void*)desc < (void*)memory_map + memory_map_size) {
                if(desc->Type == EfiConventionalMemory) {
                    pages_free += desc->NumberOfPages;
                    mm_mark_physical_pages(desc->PhysicalStart, desc->NumberOfPages, MM_FREE);
                }

                desc = (void*)desc + descriptor_size;
            }
            fbconsole_write(" -> %u pages (%B) free", pages_free, pages_free * 4096);
        )

        INIT_STEP(
            "Initializing virtual memory management",
            init_vm();
        )

        INIT_STEP(
            "Relocating kernel",
            relocate_high(imageBase, imageSize);
            jump_higher_half(0xFFFF800000000000, imageBase);
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

            ptr_t entry = load_elf((ptr_t)initrd_buffer, init_context);
            start_task(init_context, entry);
        )
    }

    asm("sti");
    while(1);
}
