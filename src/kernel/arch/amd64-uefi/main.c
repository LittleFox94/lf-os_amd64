#include <efi.h>
#include <efilib.h>

#include "mm.h"
#include "fbconsole.h"
#include "sd.h"
#include "elf.h"

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
        const unsigned buffer_size = 65536;
        UINTN file_buffer_size     = 0;
        UINTN read_size            = buffer_size;
        UINTN file_size            = 0;

        fbconsole_write("Loading init ");

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

        fbconsole_write("\n  loaded 0x%x (%u bytes, %B)\n", (long)temp_buffer, file_size, file_size);

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
                x, y,
                gimp_image.pixel_data[coord + 2],
                gimp_image.pixel_data[coord + 1],
                gimp_image.pixel_data[coord + 0]
            );
        }

        if(y % 16 == 0) {
            fbconsole_write("\n");
        }
    }

    fbconsole_write("Initialized framebuffer console\n", width, height, (long)fb);

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
    InitializeLib(image_handle, system_table);
    Print(L"LF OS amd64-uefi kernel initializing. Build: %a %a\n", __DATE__, __TIME__);

    EFI_STATUS status;
    EFI_LOADED_IMAGE* li;

    uefi_call_wrapper(BS->HandleProtocol, 3, image_handle, &LoadedImageProtocol, (void**)&li);

    static const int debugger_timeout = 3;
    Print(L"  Image base: 0x%016x - Waiting to attach debugger (%ds): ", li->ImageBase, debugger_timeout);

    for(int i = 0; i < debugger_timeout; i++) {
        Print(L".");

        EFI_EVENT event;
        uefi_call_wrapper(BS->CreateEvent, 5, EVT_TIMER, TPL_APPLICATION, NULL, NULL, &event);
        uefi_call_wrapper(BS->SetTimer, 3, event, TimerRelative, 10000000);
        UINTN index;
        uefi_call_wrapper(BS->WaitForEvent, 3, 1, &event, &index);
    }

    Print(L"\nInitializing console ...\n");

    status = initialize_console();

    if(status != EFI_SUCCESS) {
        Print(L"  not able to get a console: %r\n", status);
        return EFI_UNSUPPORTED;
    }

    EFI_DEVICE_PATH* init_path = FileDevicePath(li->DeviceHandle, L"\\LFOS\\init");

    if(!init_path) {
        fbconsole_write("Could not find init to load\n");
        while(1);
    }

    void*   initrd_buffer      = NULL;
    int64_t initrd_size        = load_initrd(init_path, &initrd_buffer);

    if(initrd_size < 0) {
        fbconsole_write("Could not load init: %d\n", -initrd_size);
        return -initrd_size;
    }

    fbconsole_write("Now trying to stand on my own feet ...\n");

    UINTN descriptor_size             = 0;
    UINT32 descriptor_version         = 0;
    UINTN map_key                     = 0;
    UINTN memory_map_size             = 0;
    EFI_MEMORY_DESCRIPTOR* memory_map = NULL;

    status = uefi_call_wrapper(BS->GetMemoryMap, 5, &memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);
    if(status != EFI_BUFFER_TOO_SMALL) {
        fbconsole_write("  Unexpected status from first GetMemoryMap call: %d\n", status);
        while(1);
    }

    memory_map = AllocatePool(memory_map_size);

    status = uefi_call_wrapper(BS->GetMemoryMap, 5, &memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);
    if(status != EFI_SUCCESS) {
        fbconsole_write("  Unexpected status from second GetMemoryMap call: %d\n", status);
        while(1);
    }

    status = uefi_call_wrapper(BS->ExitBootServices, 2, image_handle, map_key);

    if(status == EFI_SUCCESS) {
        fbconsole_write("  Initializing memory management (%u entries with %u bytes each) ...\n", memory_map_size / descriptor_size, descriptor_size);

        uint64_t pages_free         = 0;
        EFI_MEMORY_DESCRIPTOR* desc = memory_map;

        while((void*)desc < (void*)memory_map + memory_map_size) {
            if(desc->Type == EfiConventionalMemory) {
                pages_free += desc->NumberOfPages;
                mm_mark_physical_pages(desc->PhysicalStart, desc->NumberOfPages, MM_FREE);
            }

            desc = (void*)desc + descriptor_size;
        }
        fbconsole_write("    marked %u (%u bytes, %B) pages as free\n", pages_free, pages_free * 4096, pages_free * 4096);

        fbconsole_write("  Initializing service registry ...\n");
        init_sd();

        fbconsole_write("  Kernel boot completed, starting userland\n\n\n");
        load_elf((ptr_t)initrd_buffer);
    }
    else {
        fbconsole_write("  and it has gone wrong :(\nStatus: %d\n", status);
    }

    while(1);
}
