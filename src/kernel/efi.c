#include <log.h>
#include <string.h>
#include <stdbool.h>

#define EFIABI __attribute__((ms_abi))
#include <efi/efi.h>

static EFI_GUID gVendorLFOSGuid = {
    0x54a97f1c, 0x4828, 0x4bb0, { 0xaa, 0xa6, 0x95, 0x84, 0x5e, 0x2d, 0xb2, 0xee }
};

static EFI_SYSTEM_TABLE* gST = 0;

void init_efi(struct LoaderStruct* loaderStruct) {
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
}

void efi_append_log(char* msg) {
    static bool no_log;
    if(gST && !no_log) {
        no_log = true;

        EFI_STATUS status = gST->RuntimeServices->SetVariable(L"LFOS_LOG", &gVendorLFOSGuid, 0x47, strlen(msg), msg);
        if(status != EFI_SUCCESS) {
            loge("efi", "Could not log to EFI: %d", status);
        }

        no_log = false;
    }
}
