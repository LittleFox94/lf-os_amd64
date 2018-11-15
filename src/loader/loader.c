#include <efi.h>
#include <efilib.h>
#include <efiprot.h>

#include "config.h"


EFI_STATUS load_file(EFI_FILE_HANDLE dirHandle, uint16_t* name, size_t bufferSize, ptr_t buffer) {
    EFI_FILE_HANDLE fileHandle;
    EFI_STATUS status = uefi_call_wrapper(dirHandle->Open, 5, dirHandle, &fileHandle, name, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        Print(L" error opening %d\n", status);
        return status;
    }

    status = uefi_call_wrapper(fileHandle->Read, 3, fileHandle, &bufferSize, buffer);
    if (EFI_ERROR(status)) {
        Print(L" error reading %d\n", status);
        return status;
    }

    status = uefi_call_wrapper(fileHandle->Close, 1, fileHandle);
    if (EFI_ERROR(status)) {
        Print(L" error closing %d\n", status);
        return status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS load_files(EFI_HANDLE handle, uint16_t* path) {
    EFI_LOADED_IMAGE* li;
    EFI_STATUS status = uefi_call_wrapper(BS->HandleProtocol, 3, handle, &LoadedImageProtocol, &li);
    if (EFI_ERROR(status)) {
        Print(L"Error in HandleProtocol: %d", status);
        return status;
    }

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* volume;
    status = uefi_call_wrapper(BS->HandleProtocol, 3, li->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, &volume);
    if (EFI_ERROR(status)) {
        Print(L"Error in OpenProtocol: %d", status);
        return status;
    }

    EFI_FILE_HANDLE rootHandle;
    status = uefi_call_wrapper(volume->OpenVolume, 2, volume, &rootHandle);
    if (EFI_ERROR(status)) {
        Print(L"Error in OpenVolume: %d", status);
        return status;
    }

    EFI_FILE_HANDLE dirHandle;
    status = uefi_call_wrapper(rootHandle->Open, 5, rootHandle, &dirHandle, LF_OS_LOCATION, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        Print(L"Error in Open: %d", status);
        return status;
    }

    size_t read = 64;
    do {
        EFI_FILE_INFO* buffer = AllocatePool(read);

        status = uefi_call_wrapper(dirHandle->Read, 3, dirHandle, &read, buffer);
        if (EFI_ERROR(status)) {
            if(status != EFI_BUFFER_TOO_SMALL) {
                FreePool(buffer);
                Print(L"Error in Read: %d\n", status);
                return status;
            }
        }
        else if(read > 0 && (buffer->Attribute & EFI_FILE_DIRECTORY) == 0) {
            char* fileBuffer = AllocatePool(buffer->FileSize);
            Print(L"  %s ...", buffer->FileName);

            status = load_file(dirHandle, buffer->FileName, buffer->FileSize, fileBuffer);
            if(EFI_ERROR(status)) {
                FreePool(fileBuffer);
                FreePool(buffer);
                return status;
            }

            Print(L" ok\n");
        }

        FreePool(buffer);
    } while (read > 0);

    status = uefi_call_wrapper(dirHandle->Close, 1, dirHandle);
    if (EFI_ERROR(status)) {
        Print(L"Error in Close dir: %d\n", status);
        return status;
    }

    status = uefi_call_wrapper(rootHandle->Close, 1, rootHandle);
    if (EFI_ERROR(status)) {
        Print(L"Error in Close root: %d\n", status);
        return status;
    }

    while(1);

    return EFI_SUCCESS;
}

EFI_STATUS prepare_framebuffer() {
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

    //size_t width  = gop->Mode->Info->HorizontalResolution;
    //size_t height = gop->Mode->Info->VerticalResolution;
    //ptr_t  fb  = (uint8_t*)gop->Mode->FrameBufferBase;

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
    InitializeLib(image_handle, system_table);

    Print(L"LF OS amd64-uefi loader. Hello\n\nLoading files\n");
    EFI_STATUS status  = load_files(image_handle, LF_OS_LOCATION);

    if(status != EFI_SUCCESS) {
        Print(L"\nCould not load init: %d\n", status);
        return status;
    }

    size_t descriptor_size       = 0;
    uint32_t descriptor_version  = 0;
    uint64_t map_key             = 0;
    size_t memory_map_size       = 128;
    EFI_MEMORY_DESCRIPTOR* memory_map = AllocatePool(memory_map_size);

    do {
        status = uefi_call_wrapper(BS->GetMemoryMap, 5, &memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);

        if(status == EFI_SUCCESS) {
            break;
        }

        if(status != EFI_BUFFER_TOO_SMALL) {
            Print(L"\nUnexpected status from GetMemoryMap call: %d\n", status);
            while(1);
        }

        FreePool(memory_map);
        memory_map = AllocatePool(memory_map_size);
    } while(status == EFI_BUFFER_TOO_SMALL);

    asm volatile("cli");
    status = uefi_call_wrapper(BS->ExitBootServices, 2, image_handle, map_key);

    //init_gdt();

    if(status == EFI_SUCCESS) {
        uint64_t pages_free         = 0;
        EFI_MEMORY_DESCRIPTOR* desc = memory_map;

        while((void*)desc < (void*)memory_map + memory_map_size) {
            if(desc->Type == EfiConventionalMemory) {
                pages_free += desc->NumberOfPages;
                //mm_mark_physical_pages(desc->PhysicalStart, desc->NumberOfPages, MM_FREE);
            }

            desc = (void*)desc + descriptor_size;
        }
        Print(L" -> %u pages free", pages_free);
    }

    while(1);
}
