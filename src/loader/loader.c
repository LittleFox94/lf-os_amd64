#include <stdint.h>

#include <efi.h>
#include <protocol/efi-sfsp.h>
#include <protocol/efi-fp.h>
#include <protocol/efi-lip.h>
#include <protocol/efi-gop.h>

#include <loader.h>
#include <config.h>

#include "vm.h"
#include "elf.h"
#include "string.h"

#if DEBUG
#  define DEBUG_TRACE(args...) wprintf(args);
static const int gDebugMode = 1;
#else
#  define DEBUG_TRACE
static const int gDebugMode = 0;
#endif

#define EFI_CHECK_ERROR_CALL_NORET(protocol, function, ...) \
    if(gDebugMode) \
        wprintf(L"[%s:%d] " #protocol "->" #function "..", WIDEN(__FUNCTION__), __LINE__); \
    status = protocol->function(__VA_ARGS__); \
    if(status & EFI_ERR) { \
        if(!gDebugMode) \
            wprintf(L"[%s:%d] " #protocol "->" #function " ", WIDEN(__FUNCTION__), __LINE__, status); \
        wprintf(L" failed: %d\n", status); \
    } \
    else { \
        DEBUG_TRACE(L" succeeded: %d\n", status); \
    }

#define EFI_CHECK_ERROR_CALL(protocol, function, ...) \
    EFI_CHECK_ERROR_CALL_NORET(protocol, function, __VA_ARGS__) \
    if(status & EFI_ERR) { \
        return status; \
    }

#define EFI_CHECK_ERROR_THISCALL_NORET(protocol, function, ...) \
    EFI_CHECK_ERROR_CALL_NORET(protocol, function, protocol, __VA_ARGS__)

#define EFI_CHECK_ERROR_THISCALL(protocol, function, ...) \
    EFI_CHECK_ERROR_CALL(protocol, function, protocol, __VA_ARGS__)


static EFI_GUID gEfiLoadedImageProtocolGuid      = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID gEfiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
static EFI_GUID gEfiGraphicsOutputProtocolGuid   = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

static const size_t UNIT_CONV = 1024;
static const size_t KB        = UNIT_CONV;
static const size_t MB        = UNIT_CONV * UNIT_CONV;
static const size_t PAGE_SIZE = 4 * KB;

struct LoaderFileDescriptor {
    uint8_t  name[256];
    uint64_t size;
    uint8_t* data;
};

struct LoaderState {
    struct LoaderStruct* loaderStruct;

    EFI_LOADED_IMAGE_PROTOCOL* loadedImage;
    ptr_t                      loaderStart;
    size_t                     loaderSize;

    ptr_t    physicalKernelLocation;
    size_t   kernelSize;
    uint64_t kernelEntry;

    uint64_t             mapKey;
    struct MemoryRegion* memoryMapLocation;
    size_t               memoryMapEntryCount;

    EFI_MEMORY_DESCRIPTOR* efiMemoryMap;
    size_t                 efiMemoryMapSize;
    size_t                 efiMemoryMapEntrySize;
    uint32_t               efiMemoryMapEntryVersion;

    struct LoaderFileDescriptor* fileDescriptors;

    ptr_t    scratchpad;

    ptr_t    loaderScratchpad;
    ptr_t    loaderScratchpadFree;
    size_t   loaderScratchpadSize;
};

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

    memset(ret, 0, len);

    return ret + alignmentOffset;
}

EFI_STATUS handle_loaded_file(struct LoaderState* state, void* buffer, size_t size, CHAR16* path) {
    EFI_STATUS status = 0;

    CHAR16* name = 0;
    size_t plen = wcslen(path);
    for(size_t i = plen - 1; i > 0; --i) {
        if(path[i] == L'/') {
            name = &path[i+1];
            break;
        }
    }

    if(!name) {
        name = path;
    }

    static const CHAR16* kernel_filename = L"KeRnEl"; // funny-case to trigger fails in wcscasecmp in dev earlier

    if(plen >= wcslen(kernel_filename) && wcscasecmp(name, kernel_filename) == 0) {
        elf_file_header_t* elf_header = (elf_file_header_t*)buffer;
        uint64_t ph_header = (uint64_t)buffer + (uint64_t)elf_header->programHeaderOffset;

        size_t kernelSize = 0;
        for(size_t i = 0; i < elf_header->programHeaderCount; ++i) {
            elf_program_header_t* ph = (elf_program_header_t*)(ph_header + (i * elf_header->programHeaderEntrySize));

            if(ph->type == 1) {
                size_t length = ph->memLength;

                if(ph->align) {
                    size_t padding = length % ph->align;
                    length += padding;
                }

                kernelSize += length;
            }
        }
        kernelSize += PAGE_SIZE;

        ptr_t physicalKernelLocation;

        EFI_CHECK_ERROR_CALL(BS, AllocatePages, AllocateAnyPages, EfiLoaderCode, (kernelSize + PAGE_SIZE - 1) / PAGE_SIZE, (EFI_PHYSICAL_ADDRESS*)&physicalKernelLocation);

        for(size_t i = 0; i < elf_header->programHeaderCount; ++i) {
            elf_program_header_t* ph = (elf_program_header_t*)(ph_header + (i * elf_header->programHeaderEntrySize));

            if(ph->type == 1) {
                size_t size   = ph->fileLength;
                uint64_t src  = (uint64_t)buffer + ph->offset;
                uint64_t dest = (uint64_t)(physicalKernelLocation + ph->vaddr) - 0xffffffff81000000;

                memset((void*)dest, 0, ph->memLength);
                memcpy((void*)dest, (ptr_t)src, size);
            }
        }

        state->physicalKernelLocation = physicalKernelLocation;
        state->kernelSize             = kernelSize;
        state->kernelEntry            = (uint64_t)elf_header->entrypoint - 0xffffffff81000000;

        wprintf(L" @ 0x%x (0x%x bytes, entry 0x%llx)", physicalKernelLocation, kernelSize, elf_header->entrypoint);
    }

    state->fileDescriptors = state->fileDescriptors ? realloc(state->fileDescriptors, (state->loaderStruct->num_files + 1) * sizeof(struct LoaderFileDescriptor))
                                                    : malloc((state->loaderStruct->num_files + 1) * sizeof(struct LoaderFileDescriptor));

    struct LoaderFileDescriptor* desc = state->fileDescriptors + state->loaderStruct->num_files;
    memset(desc->name, 0, 256);
    desc->size = size;

    EFI_CHECK_ERROR_CALL(BS, AllocatePages, AllocateAnyPages, EfiLoaderData, (desc->size + PAGE_SIZE - 1) / PAGE_SIZE, (EFI_PHYSICAL_ADDRESS*)&desc->data);

    memcpy(desc->data, buffer, desc->size);

    size_t nameLen = wcslen(name);
    if(nameLen > sizeof(desc->name) - 1) {
        nameLen = sizeof(desc->name) - 1;
    }

    wcstombs((char*)desc->name, name, nameLen);

    ++state->loaderStruct->num_files;

    return status;
}

EFI_STATUS load_file(struct LoaderState* state, EFI_FILE_PROTOCOL* dirHandle, uint16_t* name, size_t fileSize) {
    wprintf(L"  %s ...", name);

    EFI_STATUS status;

    EFI_FILE_PROTOCOL* fileHandle;
    EFI_CHECK_ERROR_THISCALL(dirHandle, Open, &fileHandle, name, EFI_FILE_MODE_READ, 0);

    VOID* buffer = 0;

    do {
        buffer = realloc(buffer, fileSize);
        status = fileHandle->Read(fileHandle, &fileSize, buffer);
    } while(status == EFI_BUFFER_TOO_SMALL);

    EFI_CHECK_ERROR_THISCALL(fileHandle, Close);

    status = handle_loaded_file(state, buffer, fileSize, name);
    free(buffer);

    if(status & EFI_ERR) {
        wprintf(L" failed: %d\n", status);
        return status;
    }

    wprintf(L" ok\n");
    return EFI_SUCCESS;
}

EFI_STATUS initPxeSfs(EFI_HANDLE* deviceHandle);

EFI_STATUS load_files(uint16_t* path, struct LoaderState* state) {
    EFI_STATUS status;
    EFI_HANDLE deviceHandle = state->loadedImage->DeviceHandle;

    status = initPxeSfs(&deviceHandle);
    if(status != EFI_SUCCESS) {
        wprintf(L" Not booting via PXE\n", status);
    }

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* volume;
    EFI_CHECK_ERROR_CALL(BS, HandleProtocol, deviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&volume);

    EFI_FILE_PROTOCOL* rootHandle;
    EFI_CHECK_ERROR_THISCALL(volume, OpenVolume, &rootHandle);

    EFI_FILE_PROTOCOL* dirHandle;
    EFI_CHECK_ERROR_THISCALL_NORET(rootHandle, Open, &dirHandle, path, EFI_FILE_MODE_READ, 0);

    if(status == EFI_SUCCESS) {
        size_t read = 64;
        do {
            EFI_FILE_INFO* buffer = malloc(read);

            status = dirHandle->Read(dirHandle, &read, buffer);
            if (status & EFI_ERR) {
                if(status != EFI_BUFFER_TOO_SMALL) {
                    free(buffer);
                    wprintf(L"Error in Read: %d\n", status);
                    return status;
                }
            }
            else if(read > 0 && (buffer->Attribute & EFI_FILE_DIRECTORY) == 0) {
                uint8_t* fileBuffer;
                EFI_CHECK_ERROR_CALL_NORET(BS, AllocatePages, AllocateAnyPages, EfiLoaderCode, (buffer->FileSize + PAGE_SIZE - 1) / PAGE_SIZE, (EFI_PHYSICAL_ADDRESS*)&fileBuffer);

                if(status & EFI_ERR) {
                    free(fileBuffer);
                    free(buffer);
                    return status;
                }

                status = load_file(state, dirHandle, buffer->FileName, buffer->FileSize);
                if(status & EFI_ERR) {
                    free(buffer);
                    return status;
                }
            }

            free(buffer);
        } while (read > 0);

        EFI_CHECK_ERROR_THISCALL(dirHandle, Close);
    }
    else {
        // we cannot read directories, maybe we are PXE-booted
        // lets try with a static file list instead
        CHAR16* files[] = {
            L"kernel",
            L"init",
        };

        size_t plen = wcslen(path);
        size_t fileCount = sizeof(files) / sizeof(CHAR16*);

        for(size_t i = 0; i < fileCount; ++i) {
            size_t flen = wcslen(files[i]);
            CHAR16* fullPath = malloc(sizeof(CHAR16) * (plen + flen + 2));
            memset(fullPath, 0, sizeof(CHAR16) * (plen + flen + 2));
            wcscpy(fullPath, path);
            fullPath[plen] = L'/';
            wcscpy(fullPath + plen + 1, files[i]);

            if((status = load_file(state, rootHandle, fullPath, 0)) != EFI_SUCCESS) {
                return status;
            }
        }
    }

    EFI_CHECK_ERROR_THISCALL(rootHandle, Close);

    return EFI_SUCCESS;
}

EFI_STATUS retrieve_memory_map(struct LoaderState* state) {
    state->efiMemoryMapSize  = 512;
    int completed;

    EFI_STATUS status;

    do {
        completed = 1;

        EFI_MEMORY_DESCRIPTOR* memory_map = malloc(state->efiMemoryMapSize);

        do {
            status = BS->GetMemoryMap(&state->efiMemoryMapSize, memory_map, &state->mapKey, &state->efiMemoryMapEntrySize, &state->efiMemoryMapEntryVersion);

            if(status & EFI_ERR) {
                if(status != EFI_BUFFER_TOO_SMALL) {
                    wprintf(L"\nUnexpected status from GetMemoryMap call: %d\n", status);
                    return status;
                }
                else {
                    memory_map = realloc(memory_map, state->efiMemoryMapSize);
                    state->mapKey = 0;
                }
            }
        } while(status == EFI_BUFFER_TOO_SMALL);

        struct MemoryRegion* regionBuffer = allocate_from_loader_scratchpad(state, (state->efiMemoryMapSize / state->efiMemoryMapEntrySize) * sizeof(struct MemoryRegion), PAGE_SIZE);

        size_t   lfos_mem_map_index  = 0;
        EFI_MEMORY_DESCRIPTOR* desc = memory_map;

        while((void*)desc < (void*)memory_map + state->efiMemoryMapSize) {
            uint64_t flags = 0;
            if(desc->Type == EfiConventionalMemory) {
                flags = MEMORY_REGION_USABLE;
            }
            else if(desc->Attribute & EFI_MEMORY_RUNTIME) {
                flags = MEMORY_REGION_FIRMWARE;

                if((desc->Attribute & EFI_MEMORY_XP) == 0) {
                    flags |= MEMORY_REGION_CODE;
                }
            }

            if(flags) {
                if(lfos_mem_map_index > 0) {
                    // merge with previous region if following directly after and flags match
                    if(
                        regionBuffer[lfos_mem_map_index-1].start_address + (regionBuffer[lfos_mem_map_index-1].num_pages * PAGE_SIZE) == (ptr_t)desc->PhysicalStart &&
                        regionBuffer[lfos_mem_map_index-1].flags == flags
                    ) {
                        regionBuffer[lfos_mem_map_index-1].num_pages += desc->NumberOfPages;
                        continue;
                    }
                }

                regionBuffer[lfos_mem_map_index].start_address = (ptr_t)desc->PhysicalStart;
                regionBuffer[lfos_mem_map_index].num_pages     = desc->NumberOfPages;
                regionBuffer[lfos_mem_map_index].flags         = flags;

                ++lfos_mem_map_index;

                if(lfos_mem_map_index >= state->efiMemoryMapSize / state->efiMemoryMapEntrySize) {
                    completed = 0;
                    break;
                }
            }

            desc = (void*)desc + state->efiMemoryMapEntrySize;
        }

        if(completed) {
            state->memoryMapLocation          = regionBuffer;
            state->memoryMapEntryCount        = lfos_mem_map_index;
            state->loaderStruct->num_mem_desc = lfos_mem_map_index;
            state->efiMemoryMap               = (EFI_MEMORY_DESCRIPTOR*)memory_map;
            return EFI_SUCCESS;
        }
    } while(!completed);

    return EFI_SUCCESS;
}

EFI_STATUS prepare_framebuffer(struct LoaderState* state) {
    EFI_STATUS status;

    UINTN handle_count        = 0;
    EFI_HANDLE* handle_buffer = 0;

    EFI_CHECK_ERROR_CALL(BS, LocateHandleBuffer, ByProtocol, &gEfiGraphicsOutputProtocolGuid, 0, &handle_count, &handle_buffer);

    if(handle_count == 0) {
        return EFI_UNSUPPORTED;
    }

    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    EFI_CHECK_ERROR_CALL(BS, HandleProtocol, handle_buffer[0], &gEfiGraphicsOutputProtocolGuid, (void**)&gop);

    size_t width  = gop->Mode->Info->HorizontalResolution;
    size_t height = gop->Mode->Info->VerticalResolution;
    size_t stride = gop->Mode->Info->PixelsPerScanLine;
    ptr_t  fb     = (uint8_t*)gop->Mode->FrameBufferBase;

    state->loaderStruct->fb_width    = width;
    state->loaderStruct->fb_height   = height;
    state->loaderStruct->fb_stride   = stride;
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
            .next_base = ((uint64_t)allocate_from_loader_scratchpad(state, sizeof(vm_table_t), PAGE_SIZE) >> 12),
        };
    }

    vm_table_t* pdp = (vm_table_t*)((uint64_t)pml4->entries[pml4_idx].next_base << 12);

    if(!pdp->entries[pdp_idx].present) {
        pdp->entries[pdp_idx] = (vm_table_entry_t){
            PAGING_ENTRY_DEFAULT_SETTINGS,
            .next_base = ((uint64_t)allocate_from_loader_scratchpad(state, sizeof(vm_table_t), PAGE_SIZE) >> 12),
        };
    }

    vm_table_t* pd = (vm_table_t*)((uint64_t)pdp->entries[pdp_idx].next_base << 12);

    if(!pd->entries[pd_idx].present) {
        pd->entries[pd_idx] = (vm_table_entry_t){
            PAGING_ENTRY_DEFAULT_SETTINGS,
            .next_base = ((uint64_t)allocate_from_loader_scratchpad(state, sizeof(vm_table_t), PAGE_SIZE) >> 12),
        };
    }

    vm_table_t* pt = (vm_table_t*)((uint64_t)pd->entries[pd_idx].next_base << 12);

    pt->entries[pt_idx] = (vm_table_entry_t){
        PAGING_ENTRY_DEFAULT_SETTINGS,
        .next_base = ((uint64_t)physical >> 12),
    };
}

void initialize_virtual_memory(struct LoaderState* state, EFI_SYSTEM_TABLE* system_table) {
    const ptr_t higherHalf = (ptr_t)0xffffffff80000000;

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

#if LF_OS_ENABLE_EFI_RUNTIME_SERVICES
    state->loaderStruct->firmware_info = system_table;

    // Remap UEFI runtime memory
    for(size_t i = 0; i < state->efiMemoryMapSize / state->efiMemoryMapEntrySize; ++i) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((void*)state->efiMemoryMap + (i * state->efiMemoryMapEntrySize));

        if(desc->Attribute & EFI_MEMORY_RUNTIME) {
            desc->VirtualStart = (EFI_VIRTUAL_ADDRESS)filesStart;

            for(size_t j = 0; j < desc->NumberOfPages; ++j) {
                map_page(state, pml4, filesStart, (ptr_t)desc->PhysicalStart + (j * PAGE_SIZE));
                filesStart += PAGE_SIZE;
            }
        }
    }

    system_table->RuntimeServices->SetVirtualAddressMap(state->efiMemoryMapSize, state->efiMemoryMapEntrySize, state->efiMemoryMapEntryVersion, state->efiMemoryMap);
#endif // LF_OS_ENABLE_EFI_RUNTIME_SERVICES

    // prepare final loaderStuct, memory map and files location
    ptr_t loaderStructsArea = allocate_from_loader_scratchpad(state,
        system_table->Hdr.HeaderSize +
        state->loaderStruct->size                           +
        state->memoryMapEntryCount * sizeof(struct MemoryRegion)   +
        state->loaderStruct->num_files * sizeof(struct FileDescriptor),
        4096
    );

#if LF_OS_ENABLE_EFI_RUNTIME_SERVICES
    state->loaderStruct->firmware_info = filesStart;

    memcpy(loaderStructsArea,
            system_table, system_table->Hdr.HeaderSize);
#endif

    memcpy(loaderStructsArea + system_table->Hdr.HeaderSize,
            state->loaderStruct, state->loaderStruct->size);
    memcpy(loaderStructsArea + system_table->Hdr.HeaderSize + state->loaderStruct->size,
            state->memoryMapLocation, state->memoryMapEntryCount * sizeof(struct MemoryRegion));

    struct LoaderStruct* loaderStructFinal = filesStart + system_table->Hdr.HeaderSize;
    for(size_t i = 0; i < state->loaderStruct->size + system_table->Hdr.HeaderSize; i += PAGE_SIZE) {
        map_page(state, pml4, (ptr_t)(filesStart) + i, loaderStructsArea + i);
        filesStart += PAGE_SIZE;
    }

    struct FileDescriptor* fileDescriptors = (struct FileDescriptor*)(loaderStructsArea +
                                                        system_table->Hdr.HeaderSize +
                                                        state->loaderStruct->size +
                                                        state->memoryMapEntryCount * sizeof(struct MemoryRegion));

    for(size_t i = 0; i < state->loaderStruct->num_files; ++i) {
        memcpy((fileDescriptors + i)->name, (state->fileDescriptors + i)->name, 256);
        (fileDescriptors + i)->size   = (state->fileDescriptors + i)->size;
        (fileDescriptors + i)->offset = (filesStart - (ptr_t)loaderStructFinal);

        for(size_t j = 0; j < (fileDescriptors + i)->size; j += PAGE_SIZE) {
            map_page(state, pml4, filesStart, (state->fileDescriptors + i)->data + j);
            filesStart += PAGE_SIZE;
        }
    }

    asm(
        "mov %3,    %%cr3\n"
        "mov %%rax, %%rsp\n"
        "subq $16,  %%rsp\n"
        "mov %%rsp, %%rbp\n"

        "addq %%rbx, %%rax\n"
        "jmpq *%%rax"
        ::
        "D"(loaderStructFinal),
        "a"(kernelStart),
        "b"(state->kernelEntry),
        "r"(pml4)
    );

    while(1);
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
    EFI_STATUS status;

    init_stdlib(image_handle, system_table);

    struct LoaderState state;
    memset(&state, 0, sizeof(state));

    EFI_LOADED_IMAGE_PROTOCOL* li;
    EFI_CHECK_ERROR_CALL(BS, HandleProtocol, image_handle, &gEfiLoadedImageProtocolGuid, (void**)&li);

    state.loadedImage = li;
    state.loaderStart = (ptr_t)li->ImageBase;
    state.loaderSize  = (size_t)li->ImageSize;

    state.scratchpad  = malloc(16 * MB);
    memset(state.scratchpad, 0, 16 * MB);

    state.loaderScratchpadSize = 4 * MB;
    state.loaderScratchpadFree = state.loaderScratchpad
                               = malloc(state.loaderScratchpadSize);

    state.loaderStruct                = allocate_from_loader_scratchpad(&state, sizeof(struct LoaderStruct), PAGE_SIZE);
    state.loaderStruct->signature     = LFOS_LOADER_SIGNATURE;
    state.loaderStruct->size          = sizeof(struct LoaderStruct);

    wprintf(
        L"LF OS amd64 uefi loader (%u bytes at 0x%x).\n\n",
        state.loaderSize, state.loaderStart
    );

    wprintf(
        L"Location of data structures in memory:\n"
        "  state:             0x%x (%d bytes)\n"
        "  loaderStruct:      0x%x (%d bytes)\n"
        "  scratchpad:        0x%x (%d bytes)\n"
        "  loader scratchpad: 0x%x (%d bytes)\n",
        &state,                 sizeof(state),
        state.loaderStruct,     sizeof(struct LoaderStruct),
        state.scratchpad,       16 * MB,
        state.loaderScratchpad, state.loaderScratchpadSize
    );

    if(sizeof(vm_table_t) != 4096) {
        wprintf(L"\nCOMPILATION ERROR sizeof(vm_table_t) = %u\n", sizeof(vm_table_t));
        while(1);
    }

    wprintf(L"Loading files\n");
    status = load_files(WIDEN(LF_OS_LOCATION), &state);
    if(status != EFI_SUCCESS) {
        wprintf(L"\nCould not load files: %d\n", status);
        return status;
    }

    wprintf(L"Preparing console ...\n");
    status = prepare_framebuffer(&state);
    if(status != EFI_SUCCESS) {
        wprintf(L" error: %d\n", status);
        return status;
    }

    unsigned int try_counter = 3;

    wprintf(L"Preparing memory map and exiting boot services ...");

    while(--try_counter) {
        wprintf(L".");
        if(retrieve_memory_map(&state) != EFI_SUCCESS) {
            continue;
        }

        status = BS->ExitBootServices(image_handle, state.mapKey);

        if(status == EFI_SUCCESS) {
            asm volatile("cli");
            break;
        }
    }

    if(status != EFI_SUCCESS) {
        wprintf(L" ExitBootServices failed: %d\n", status);
        while(1);
    }

    initialize_virtual_memory(&state, system_table);
    return EFI_ABORTED; // will never get here
}
