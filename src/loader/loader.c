#include <efi.h>
#include <efilib.h>
#include <efiprot.h>

#include "config.h"

#include "loader.h"
#include "vm.h"

static const size_t UNIT_CONV = 1024;
static const size_t KB        = UNIT_CONV;
static const size_t MB        = UNIT_CONV * UNIT_CONV;
static const size_t PAGE_SIZE =  4 * KB;

struct LoaderState {
    LoaderStruct* loaderStruct;

    ptr_t    loaderStart;
    size_t   loaderSize;

    ptr_t    physicalKernelLocation;
    size_t   kernelSize;

    uint64_t mapKey;
    ptr_t    memoryMapLocation;
    size_t   memoryMapEntryCount;

    size_t   fileDescriptorCount;

    ptr_t    scratchpad;

    ptr_t    loaderScratchpad;
    ptr_t    loaderScratchpadFree;
    size_t   loaderScratchpadSize;
};

typedef void(*kernel_entry)(LoaderStruct*);

ptr_t allocate_from_loader_scratchpad(struct LoaderState* state, size_t len, size_t alignment) {
    size_t alignmentOffset = alignment - ((uint64_t)state->loaderScratchpadFree % alignment);

    if(alignmentOffset == alignment) {
        alignmentOffset = 0;
    }

    len += alignmentOffset;

    if(state->loaderScratchpadFree + len > state->loaderScratchpad + state->loaderScratchpadSize) {
        while(1);   // out of memory, just increase the size in main()
                    // not really unexpected since the scratchpad can only allocate and not free memory
    }

    ptr_t ret = state->loaderScratchpadFree;
    state->loaderScratchpadFree += len;

    for(size_t i = 0; i < len; ++i) {
        ((uint8_t*)ret)[i] = 0;
    }

    return ret + alignmentOffset;
}

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

EFI_STATUS load_files(EFI_HANDLE handle, uint16_t* path, struct LoaderState* state) {
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

            if(StrCmp(buffer->FileName, L"kernel") == 0) {
                ptr_t physicalKernelLocation;

                status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderCode, (buffer->FileSize + PAGE_SIZE - 1) / PAGE_SIZE, &physicalKernelLocation);
                if(EFI_ERROR(status)) {
                    Print(L" cannot get pages for kernel: %d\n", status);
                    FreePool(fileBuffer);
                    FreePool(buffer);
                    return status;
                }

                CopyMem(physicalKernelLocation, fileBuffer, buffer->FileSize);
                Print(L" @ 0x%x ", physicalKernelLocation);

                state->physicalKernelLocation = physicalKernelLocation;
                state->kernelSize             = buffer->FileSize;
            }

            FreePool(fileBuffer);
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

    return EFI_SUCCESS;
}

EFI_STATUS retrieve_memory_map(struct LoaderState* state) {
    size_t descriptor_size      = 0;
    uint32_t descriptor_version = 0;
    size_t efi_memory_map_size  = 128;
    size_t lfos_memory_map_size = 128;
    int completed;

    EFI_STATUS status;

    do {
        completed = 1;

        MemoryRegion* regionBuffer        = AllocatePool(lfos_memory_map_size * sizeof(MemoryRegion));
        EFI_MEMORY_DESCRIPTOR* memory_map = AllocatePool(efi_memory_map_size);

        do {
            status = uefi_call_wrapper(BS->GetMemoryMap, 5, &efi_memory_map_size, memory_map, &state->mapKey, &descriptor_size, &descriptor_version);

            if(EFI_ERROR(status)) {
                if(status != EFI_BUFFER_TOO_SMALL) {
                    Print(L"\nUnexpected status from GetMemoryMap call: %d\n", status);
                    return status;
                }
                else {
                    FreePool(memory_map);
                    memory_map = AllocatePool(efi_memory_map_size);
                }
            }
        } while(status == EFI_BUFFER_TOO_SMALL);

        uint64_t pages_free         = 0;
        size_t   lfos_mem_map_index = 0;
        EFI_MEMORY_DESCRIPTOR* desc = memory_map;

        while((void*)desc < (void*)memory_map + efi_memory_map_size) {
            if(desc->Type == EfiConventionalMemory) {

                pages_free += desc->NumberOfPages;

                if(lfos_mem_map_index > 0) {
                    if(regionBuffer[lfos_mem_map_index-1].start_address + (regionBuffer[lfos_mem_map_index-1].num_pages * PAGE_SIZE) == (ptr_t)desc->PhysicalStart) {
                        regionBuffer[lfos_mem_map_index-1].num_pages += desc->NumberOfPages;
                        continue;
                    }
                }

                regionBuffer[lfos_mem_map_index].start_address = (ptr_t)desc->PhysicalStart;
                regionBuffer[lfos_mem_map_index].num_pages     = desc->NumberOfPages;
                regionBuffer[lfos_mem_map_index].flags         = MEMORY_REGION_FREE;

                ++lfos_mem_map_index;

                if(lfos_mem_map_index >= lfos_memory_map_size) {
                    completed = 0;
                    break;
                }
            }

            desc = (void*)desc + descriptor_size;
        }

        if(completed) {
            state->memoryMapLocation          = regionBuffer;
            state->memoryMapEntryCount        = lfos_mem_map_index;
            state->loaderStruct->num_mem_desc = lfos_mem_map_index;
            return EFI_SUCCESS;
        }
        else {
            FreePool(regionBuffer);
        }
    } while(!completed);


    return EFI_SUCCESS;
}

EFI_STATUS prepare_framebuffer(struct LoaderState* state) {
    EFI_STATUS status;

    UINTN handle_count        = 0;
    EFI_HANDLE* handle_buffer = NULL;
    status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &handle_count, &handle_buffer);
    if(status != EFI_SUCCESS) {
        return status;
    }

    if(handle_count == 0) {
        return EFI_UNSUPPORTED;
    }

    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    status = uefi_call_wrapper(BS->HandleProtocol, 2, handle_buffer[0], &gEfiGraphicsOutputProtocolGuid, &gop);
    if(status != EFI_SUCCESS) {
        return status;
    }

    size_t width  = gop->Mode->Info->HorizontalResolution;
    size_t height = gop->Mode->Info->VerticalResolution;
    ptr_t  fb     = (uint8_t*)gop->Mode->FrameBufferBase;

    state->loaderStruct->fb_width    = width;
    state->loaderStruct->fb_height   = height;
    state->loaderStruct->fb_location = fb;
    state->loaderStruct->fb_bpp      = 4;

    return EFI_SUCCESS;
}

void map_page(struct LoaderState* state, vm_table_t* pml4, ptr_t virtual, ptr_t physical) {
    uint16_t pml4_idx = PML4_INDEX((uint64_t)virtual);
    uint16_t pdp_idx  = PDP_INDEX((uint64_t)virtual);
    uint16_t pd_idx   = PD_INDEX((uint64_t)virtual);
    uint16_t pt_idx   = PT_INDEX((uint64_t)virtual);

    if(!pml4->entries[pml4_idx].present) {
        pml4->entries[pml4_idx] = (vm_table_entry_t){
            PAGING_ENTRY_DEFAULT_SETTINGS,
            .next_base      = ((uint64_t)allocate_from_loader_scratchpad(state, sizeof(vm_table_t), PAGE_SIZE) >> 12),
        };
    }

    vm_table_t* pdp = (vm_table_t*)((uint64_t)pml4->entries[pml4_idx].next_base << 12);

    if(!pdp->entries[pdp_idx].present) {
        pdp->entries[pdp_idx] = (vm_table_entry_t){
            PAGING_ENTRY_DEFAULT_SETTINGS,
            .next_base      = ((uint64_t)allocate_from_loader_scratchpad(state, sizeof(vm_table_t), PAGE_SIZE) >> 12),
        };
    }

    vm_table_t* pd = (vm_table_t*)((uint64_t)pdp->entries[pdp_idx].next_base << 12);

    if(!pd->entries[pd_idx].present) {
        pd->entries[pd_idx] = (vm_table_entry_t){
            PAGING_ENTRY_DEFAULT_SETTINGS,
            .next_base      = ((uint64_t)allocate_from_loader_scratchpad(state, sizeof(vm_table_t), PAGE_SIZE) >> 12),
        };
    }

    vm_table_t* pt = (vm_table_t*)((uint64_t)pd->entries[pd_idx].next_base << 12);

    pt->entries[pt_idx] = (vm_table_entry_t){
        PAGING_ENTRY_DEFAULT_SETTINGS,
        .next_base      = ((uint64_t)physical >> 12),
    };
}

// this needs to be an extra function to have the argument in a register and not on the stack.
void __attribute__((noinline)) jump_kernel(ptr_t kernel, LoaderStruct* loaderStruct) {
    asm volatile("mov %0, %%rsp"::"r"(kernel - 16));
    asm volatile("mov %0, %%rbp"::"r"(kernel - 16));
    ((kernel_entry)kernel)(loaderStruct);
    while(1);
}

EFI_STATUS initialize_virtual_memory(struct LoaderState* state) {
    const ptr_t higherHalf = (ptr_t)0xFFFF800000000000;

    vm_table_t* pml4 = allocate_from_loader_scratchpad(state, sizeof(vm_table_t), PAGE_SIZE);

    // map loader code
    for(size_t i = 0; i <= state->loaderSize; i += PAGE_SIZE) {
        map_page(state, pml4, state->loaderStart + i, state->loaderStart + i);
    }

    // map loader scratchpad
    // (this contains the mapping tables, required by the kernel to read current mappings)
    for(size_t i = 0; i < state->loaderScratchpadSize; i += PAGE_SIZE) {
        map_page(state, pml4, state->loaderScratchpad + i, state->loaderScratchpad + i);
    }

    // map scratchpad
    for(size_t i = 0; i < 16 * MB; i += PAGE_SIZE) {
        map_page(state, pml4, higherHalf + i, state->scratchpad + i);
    }

    // map kernel code
    ptr_t kernelStart = higherHalf + (16 * MB);
    ptr_t kernelEnd   = kernelStart;
    for(size_t i = 0; i < state->kernelSize; i+= PAGE_SIZE) {
        map_page(state, pml4, kernelStart + i, state->physicalKernelLocation + i);
        kernelEnd += PAGE_SIZE;
    }

    ptr_t originalFramebuffer = state->loaderStruct->fb_location;

    ptr_t filesStart = kernelEnd;
    size_t fbSize    = state->loaderStruct->fb_width  *
                       state->loaderStruct->fb_height *
                       state->loaderStruct->fb_bpp;

    state->loaderStruct->fb_location = filesStart;

    for(size_t i = 0; i < fbSize; i += PAGE_SIZE) {
        map_page(state, pml4, filesStart, originalFramebuffer + i);
        filesStart += PAGE_SIZE;
    }

    ptr_t loaderStructsArea = allocate_from_loader_scratchpad(state,
        state->loaderStruct->size                           +
        state->memoryMapEntryCount * sizeof(MemoryRegion)   +
        state->fileDescriptorCount * sizeof(FileDescriptor),
        4096
    );

    __builtin_memcpy(loaderStructsArea, state->loaderStruct, state->loaderStruct->size);
    __builtin_memcpy(loaderStructsArea + state->loaderStruct->size, state->memoryMapLocation, state->memoryMapEntryCount * sizeof(MemoryRegion));

    LoaderStruct* loaderStructFinal = filesStart;
    for(size_t i = 0; i < state->loaderStruct->size; i += PAGE_SIZE) {
        map_page(state, pml4, (ptr_t)(loaderStructFinal) + i, loaderStructsArea + i);
        filesStart += PAGE_SIZE;
    }

    // TODO: map loaded files

    // map the tiny bit of stack we still need
    uint64_t rsp, rbp;

    asm("mov %%rsp, %0":"=r"(rsp));
    asm("mov %%rbp, %0":"=r"(rbp));

    for(size_t i = 0; i < rbp-rsp; i += 4096) {
        map_page(state, pml4, (ptr_t)(rsp + i), (ptr_t)(rsp + i));
    }

    asm volatile("mov %0, %%cr3"::"r"(pml4));

    // Page fault here
    jump_kernel(kernelStart, loaderStructFinal);

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
    InitializeLib(image_handle, system_table);

    struct LoaderState state;

    EFI_LOADED_IMAGE* li;
    EFI_STATUS status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle, &LoadedImageProtocol, (void**)&li);
    state.loaderStart = (ptr_t)li->ImageBase;
    state.loaderSize  = (size_t)li->ImageSize;

    state.scratchpad  = AllocatePool(16 * MB);

    state.loaderScratchpadSize = 1 * MB;
    state.loaderScratchpadFree = state.loaderScratchpad
                               = AllocatePool(state.loaderScratchpadSize);

    state.loaderStruct            = allocate_from_loader_scratchpad(&state, sizeof(LoaderStruct), PAGE_SIZE);
    state.loaderStruct->signature = LFOS_LOADER_SIGNATURE;
    state.loaderStruct->size      = sizeof(LoaderStruct);

    Print(
        L"LF OS amd64-uefi loader (%u bytes at 0x%x).\n\n",
        state.loaderSize, state.loaderStart
    );

    Print(
        L"Location of data structures in memory:\n"
        "  state:             0x%x (%d bytes)\n"
        "  loaderStruct:      0x%x (%d bytes)\n"
        "  scratchpad:        0x%x (%d bytes)\n"
        "  loader scratchpad: 0x%x (%d bytes)",
        &state,                 sizeof(state),
        state.loaderStruct,     sizeof(LoaderStruct),
        state.scratchpad,       16 * MB,
        state.loaderScratchpad, state.loaderScratchpadSize
    );

    Print(L"\n\nLoading files\n");
    status = load_files(image_handle, LF_OS_LOCATION, &state);
    if(status != EFI_SUCCESS) {
        Print(L"\nCould not load files: %d\n", status);
        return status;
    }

    Print(L"Preparing console ...\n");
    status = prepare_framebuffer(&state);
    if(status != EFI_SUCCESS) {
        Print(L" error: %d\n", status);
        return status;
    }

    Print(L"Preparing memory map ...\n");
    status = retrieve_memory_map(&state);
    if(status != EFI_SUCCESS) {
        Print(L" error: %d\n", status);
        return status;
    }

    asm volatile("cli");
    status = uefi_call_wrapper(BS->ExitBootServices, 2, image_handle, state.mapKey);
    if(status != EFI_SUCCESS) {
        Print(L" error: %d\n", status);
        return status;
    }

    status = initialize_virtual_memory(&state);
    if(status != EFI_SUCCESS) {
        return status;
    }

    while(1);
}
