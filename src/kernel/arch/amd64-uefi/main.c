#include <efi.h>
#include <efilib.h>

#include "mm.h"

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
    InitializeLib(image_handle, system_table);
    Print(L"LF OS amd64-uefi starting up!\n");

    EFI_STATUS status;
    EFI_LOADED_IMAGE* li;

    status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle, &LoadedImageProtocol, (void**)&li);

    EFI_DEVICE_PATH* init_path = FileDevicePath(li->DeviceHandle, L"\\LFOS\\init");

    if(init_path == NULL) {
        Print(L"Could not get init-path\n");
    }
    else {
        EFI_HANDLE device_handle;
        SIMPLE_READ_FILE read_handle;

        status = OpenSimpleReadFile(FALSE, NULL, 0, &init_path, &device_handle, &read_handle);

        if(status == EFI_SUCCESS) {
            const unsigned buffer_size = 4096;
            void* buffer = NULL;

            UINTN file_buffer_size = 0;
            UINTN read_size = buffer_size;
            UINTN file_size = 0;

            Print(L"Loading init ");

            while(read_size == buffer_size) {
                read_size = buffer_size;
                file_buffer_size = file_size + buffer_size;
                buffer = ReallocatePool(buffer, file_size, file_buffer_size);
                status = ReadSimpleReadFile(read_handle, file_size, &read_size, buffer + file_size);

                file_size += read_size;

                if(status == EFI_SUCCESS) {
                    Print(L".");
                }
                else {
                    Print(L"x");
                }
            }

            buffer = ReallocatePool(buffer, file_buffer_size, file_size);
            Print(L"\n    %d bytes at 0x%016x\n", file_size, (UINTN)buffer);

            UINT64 max_address      = 0;
            UINT64 cpuid_instruction = 0x80000008;
            asm("cpuid":"=a"(max_address):"a"(cpuid_instruction));

            UINT64 virtual_bits  = (max_address >> 8) & 0xFF;
            UINT64 physical_bits = max_address & 0xFF;

            Print(L"CPU has %d bits physical and %d bits virtual\n", physical_bits, virtual_bits);

            UINT64 kernel_memory_start = ~((1ULL << (virtual_bits - 1)) - 1);

            CHAR16 number[17];
            ValueToHex(number, kernel_memory_start);
            Print(L"  start of usable kernel memory: 0x%016s\n", number);

            ValueToHex(number, mm_get_pml4_address());
            Print(L"  address of pml4: 0x%016s\n", number);

            UINTN descriptor_size             = 0;
            UINT32 descriptor_version         = 0;
            UINTN map_key                     = 0;
            UINTN memory_map_size             = 65536;
            EFI_MEMORY_DESCRIPTOR* memory_map = AllocatePool(memory_map_size);

            status = uefi_call_wrapper(system_table->BootServices->GetMemoryMap, &memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);
            Print(L"Status: %d, Size: %d, Key: %d, DSize: %d, DVer: %d\n", status, memory_map_size, map_key, descriptor_size, descriptor_version);

            for(UINTN i = 0; i < memory_map_size / descriptor_size; i++) {
                EFI_MEMORY_DESCRIPTOR* desc = memory_map + (i * descriptor_size);

        //        if(desc->Type == EfiConventionalMemory) {
        //            mm_mark_pages_free(desc->PhysicalStart, desc->NumberOfPages);

                    CHAR16 start[17];
                    ValueToHex(start, desc->PhysicalStart);
                    Print(L"  added %d pages (%dMB) at 0x%016s\n", desc->NumberOfPages, desc->NumberOfPages * 4096 / 1024 / 1024, start);
        //        }
            }
            Print(L"done! Now exiting boot services");
            uefi_call_wrapper(BS->ExitBootServices, image_handle, map_key);
            Print(L" and being on my own feet :)");

            while(1);
        }
        else {
            Print(L"Could not open init: %d\n", status);
        }
    }

    return EFI_NOT_FOUND;
}
