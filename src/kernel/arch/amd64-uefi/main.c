#include <efi.h>
#include <efilib.h>

#include "mm.h"
#include "fbconsole.h"

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
        const unsigned buffer_size = 4096;
        UINTN file_buffer_size     = 0;
        UINTN read_size            = buffer_size;
        UINTN file_size            = 0;

        Print(L"Loading init ");

        while(read_size == buffer_size) {
            read_size        = buffer_size;
            file_buffer_size = file_size + buffer_size;
            temp_buffer      = ReallocatePool(temp_buffer, file_size, file_buffer_size);
            status           = ReadSimpleReadFile(read_handle, file_size, &read_size, temp_buffer + file_size);

            file_size += read_size;

            if(status == EFI_SUCCESS) {
                Print(L".");
            }
            else {
                Print(L"x");
            }
        }

        Print(L"\n  loaded: %d bytes at 0x%016x\n", file_size, (UINTN)temp_buffer);

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

    fbconsole_init(width, height, fb);
    fbconsole_write("Got framebuffer console %dx%d located at 0x%x\n", width, height, (long)fb);

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
    InitializeLib(image_handle, system_table);
    Print(L"LF OS amd64-uefi starting up!\n");

    EFI_STATUS status;
    EFI_LOADED_IMAGE* li;

    uefi_call_wrapper(BS->HandleProtocol, 3, image_handle, &LoadedImageProtocol, (void**)&li);

    static const int debugger_timeout = 10;
    Print(L"  Image base: 0x%016x - Waiting to attach debugger (%ds): ", li->ImageBase, debugger_timeout);

    for(int i = 0; i < debugger_timeout; i++) {
        Print(L".");

        EFI_EVENT event;
        uefi_call_wrapper(BS->CreateEvent, 5, EVT_TIMER, TPL_APPLICATION, NULL, NULL, &event);
        uefi_call_wrapper(BS->SetTimer, 3, event, TimerRelative, 10000000);
        UINTN index;
        uefi_call_wrapper(BS->WaitForEvent, 3, 1, &event, &index);
    }

    EFI_DEVICE_PATH* init_path = FileDevicePath(li->DeviceHandle, L"\\LFOS\\init");
    void*   initrd_buffer      = NULL;
    int64_t initrd_size        = load_initrd(init_path, &initrd_buffer);

    if(initrd_size < 0) {
        Print(L"Could not load initrd: %r\n", -initrd_size);
        return -initrd_size;
    }

    Print(L"Initializing console ...\n");

    status = initialize_console();

    if(status != EFI_SUCCESS) {
        Print(L"  not able to get a console: %r\n", status);
        return EFI_UNSUPPORTED;
    }

    fbconsole_write("Now trying to stand on my own feet ...\n");

    UINTN descriptor_size             = 0;
    UINT32 descriptor_version         = 0;
    UINTN map_key                     = 0;
    UINTN memory_map_size             = 0;
    EFI_MEMORY_DESCRIPTOR* memory_map = NULL;

    status = uefi_call_wrapper(BS->GetMemoryMap, 5, &memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);
    if(status != EFI_BUFFER_TOO_SMALL) {
        fbconsole_write("Unexpected status from first GetMemoryMap call: %d\n", status);
    }

    memory_map = AllocatePool(memory_map_size);

    status = uefi_call_wrapper(BS->GetMemoryMap, 5, &memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);
    if(status != EFI_SUCCESS) {
        fbconsole_write("Unexpected status from second GetMemoryMap call: %d\n", status);
    }

    uefi_call_wrapper(BS->ExitBootServices, 2, image_handle, map_key);

    if(status == EFI_SUCCESS) {
        fbconsole_write("Initializing memory management ...\n");

        for(EFI_MEMORY_DESCRIPTOR* desc = memory_map; desc < memory_map + memory_map_size; desc += descriptor_size) {
            if(desc->Type == EfiConventionalMemory) {
                //mm_mark_pages_used(desc->PhysicalStart, desc->NumberOfPages);
                fbconsole_write("  marking %d pages as free\n", desc->NumberOfPages);
            }
        }

        fbconsole_write("Kernel boot completed, starting userland ...\n");
    }
    else {
        Print(L" and it has gone wrong :(\nStatus: %r\n", status);
    }

    while(1);
}
