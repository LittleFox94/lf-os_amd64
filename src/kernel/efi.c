#include <loader.h>
#include <log.h>
#include <efi/efi.h>

void init_efi(LoaderStruct* loaderStruct) {
    EFI_SYSTEM_TABLE* st = (EFI_SYSTEM_TABLE*)loaderStruct->firmware_info;
    logi("efi", "firmware version %d by %ls", st->FirmwareRevision, st->FirmwareVendor);
}
