#include <log.h>
#include <string.h>
#include <efi.h>
#include <acpi.h>
#include <vm.h>

static EFI_GUID gVendorLFOSGuid = {
    0x54a97f1c, 0x4828, 0x4bb0, { 0xaa, 0xa6, 0x95, 0x84, 0x5e, 0x2d, 0xb2, 0xee }
};

static EFI_SYSTEM_TABLE* gST = 0;

struct vm_table* efi_setup_mapping(struct LoaderStruct* loaderStruct) {
    struct vm_table* context = vm_context_new();

    struct MemoryRegion* memoryRegions = (struct MemoryRegion*)((uint64_t)loaderStruct + loaderStruct->size);

    for(size_t i = 0; i < loaderStruct->num_mem_desc; ++i) {
        struct MemoryRegion* desc = memoryRegions + i;

        if(desc->flags & MEMORY_REGION_FIRMWARE) {
            // TODO: this is amd64 specific code, generify VM interface for that!
            uint8_t pat = 0;

            if(desc->flags & MEMORY_REGION_WB) {
                pat = 0x06;
            }
            else if(desc->flags & MEMORY_REGION_WP) {
                pat = 0x05;
            }
            else if(desc->flags & MEMORY_REGION_WT) {
                pat = 0x04;
            }
            else if(desc->flags & MEMORY_REGION_WC) {
                pat = 0x01;
            }

            logd(
                "efi", "mapping runtime memory, %ld pages starting at 0x%lx with PAT %x",
                desc->num_pages, desc->start_address, (unsigned)pat
            );

            for(uint64_t addr = desc->start_address; addr < desc->start_address + (desc->num_pages * 0x1000); addr += 0x1000) {
                vm_context_map(context, addr, addr, pat);
            }
        }
    }

    return context;
}

void init_efi(struct LoaderStruct* loaderStruct) {
    if(!loaderStruct->firmware_info) {
        logw("efi", "No EFI system table provided by bootloader, runtime services not available");
        return;
    }

    struct vm_table* efi_context = efi_setup_mapping(loaderStruct);
    struct vm_table* original_context = vm_current_context();
    vm_context_activate(efi_context);

    gST = (EFI_SYSTEM_TABLE*)loaderStruct->firmware_info;
    logi("efi", "firmware version %d by %ls", gST->FirmwareRevision, gST->FirmwareVendor);

    uint64_t maxStorageSize, remainingStorageSize, maxVarSize;
    EFI_STATUS status = gST->RuntimeServices->QueryVariableInfo(0x7, &maxStorageSize, &remainingStorageSize, &maxVarSize);

    if(status == EFI_SUCCESS) {
        logi("efi", "firmware reports maximum variable size of %B, with %B of %B available", maxVarSize, remainingStorageSize, maxStorageSize);
    }
    else {
        loge("efi", "Could not query variable info: %d", status);
    }

    init_acpi_efi(gST);
    vm_context_activate(original_context);
}

void efi_append_log(char* msg) {
    static bool no_log;
    if(gST && !no_log) {
        no_log = true;

        EFI_STATUS status = gST->RuntimeServices->SetVariable((CHAR16*)L"LFOS_LOG", &gVendorLFOSGuid, 0x47, strlen(msg), msg);
        if(status != EFI_SUCCESS) {
            loge("efi", "Could not log to EFI: %d", status);
        }

        no_log = false;
    }
}


