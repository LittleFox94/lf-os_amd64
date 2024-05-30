#include <log.h>
#include <acpi.h>
#include <hpet.h>
#include <string.h>
#include <vm.h>

struct acpi_rsdp {
    char        signature[8];
    uint8_t     checksum;
    char        oem_id[6];
    uint8_t     revision;
    uint32_t    rsdt_ptr;
    uint32_t    length;
    uint64_t    xsdt_ptr;
    uint8_t     extended_checksum;
    uint8_t     _reserved[3];
}__attribute__((packed));

struct acpi_rsdt {
    struct acpi_table_header header;
    uint32_t                 entry_ptr[0];
}__attribute__((packed));

struct acpi_xsdt {
    struct acpi_table_header header;
    uint64_t                 entry_ptr[0];
}__attribute__((packed));

static void acpi_process_table(struct acpi_table_header* table) {
    char signature[5];
    memcpy(signature, table->signature, 4);
    signature[4] = 0;

    char oem[7];
    memcpy(oem, table->oem_id, 6);
    oem[6] = 0;

    char oem_table[9];
    memcpy(oem_table, table->oem_table_id, 8);
    oem_table[8] = 0;

    char creator[5];
    memcpy(creator, table->asl_compiler_id, 4);
    creator[4] = 0;

    uint64_t length           = table->length;
    uint64_t revision         = table->revision;
    uint64_t oem_revision     = table->oem_revision;
    uint64_t creator_revision = table->asl_compiler_revision;

    bool checksum_valid = acpi_checksum(table, table->length) == 0;

    logd("acpi", "signature: \"%s\", length: %d, revision: %d, checksum: %s, oem: \"%s\"/\"%s\" (%d), creator: \"%s\" (%d)",
        signature, length, revision,
        checksum_valid  ? "ok" : "not ok",
        oem, oem_table, oem_revision,
        creator, creator_revision
    );

    if(!checksum_valid) {
        loge("acpi", "checksum for table %s invalid, aborting", signature);
        return;
    }

    if(memcmp(table->signature, "HPET", 4) == 0) {
        init_hpet(table);
    }
}

static void init_acpi_rsdp(void* rsdp_ptr) {
    struct acpi_rsdp* rsdp = (struct acpi_rsdp*)rsdp_ptr;

    char signature[9];
    memcpy(signature, rsdp->signature, 8);
    signature[8] = 0;

    char oem[7];
    memcpy(oem, rsdp->oem_id, 6);
    oem[6] = 0;

    uint64_t length   = rsdp->length;
    uint64_t revision = rsdp->revision;
    uint64_t rsdt     = rsdp->rsdt_ptr;

    bool checksum_valid  = acpi_checksum(rsdp, 20) == 0;
    bool xchecksum_valid = acpi_checksum(rsdp, rsdp->length) == 0;

    logd("acpi", "signature: \"%s\", oem: \"%s\", length: %d, revision: %d, checksum: %s/%s, rsdt: %x, xsdt: %x",
        signature, oem, length, revision,
        checksum_valid  ? "ok" : "not ok",
        xchecksum_valid ? "ok" : "not ok",
        rsdt, rsdp->xsdt_ptr
    );

    if(!checksum_valid) {
        loge("acpi", "RSDP table checksum invalid, aborting");
        return;
    }

    if(rsdp->revision >= 2 && !xchecksum_valid) {
        loge("acpi", "RSDP table revision 2 but extended checksum invalid, aborting");
        return;
    }

    if(rsdp->revision >= 2) {
        struct acpi_xsdt* xsdt = (struct acpi_xsdt*)(rsdp->xsdt_ptr + ALLOCATOR_REGION_DIRECT_MAPPING.start);
        if(acpi_checksum(xsdt, xsdt->header.length) != 0) {
            loge("acpi", "XSDT table checksum invalid, aborting");
            return;
        }

        size_t num_entries = (xsdt->header.length - sizeof(xsdt->header)) / sizeof(uint64_t);
        logd("acpi", "%d entries in XSDT", num_entries);

        for(size_t i = 0; i < num_entries; ++i) {
            acpi_process_table((struct acpi_table_header*)(xsdt->entry_ptr[i] + ALLOCATOR_REGION_DIRECT_MAPPING.start));
        }
    } else {
        struct acpi_rsdt* rsdt = (struct acpi_rsdt*)(rsdp->rsdt_ptr + ALLOCATOR_REGION_DIRECT_MAPPING.start);
        if(acpi_checksum(rsdt, rsdt->header.length) != 0) {
            loge("acpi", "RSDT table checksum invalid, aborting");
            return;
        }

        size_t num_entries = (rsdt->header.length - sizeof(rsdt->header)) / sizeof(uint32_t);
        logd("acpi", "%d entries in RSDT", num_entries);

        for(size_t i = 0; i < num_entries; ++i) {
            acpi_process_table((struct acpi_table_header*)(rsdt->entry_ptr[i] + ALLOCATOR_REGION_DIRECT_MAPPING.start));
        }
    }
}

static EFI_GUID efi_acpi_table_guid = EFI_ACPI_TABLE_GUID;

void init_acpi_efi(EFI_SYSTEM_TABLE* efiST) {
    for(size_t i = 0; i < efiST->NumberOfTableEntries; ++i) {
        EFI_CONFIGURATION_TABLE* ct = &efiST->ConfigurationTable[i];

        if(memcmp(&ct->VendorGuid, &efi_acpi_table_guid, sizeof(EFI_GUID)) == 0) {
            logi("acpi", "EFI configuration table %d is an ACPI table");
            init_acpi_rsdp((char*)ct->VendorTable + ALLOCATOR_REGION_DIRECT_MAPPING.start);
        }
    }
}

uint8_t acpi_checksum(void* data, size_t len) {
    uint8_t ret = 0;
    for(size_t i = 0; i < len; ++i) {
        ret += ((char*)data)[i];
    }

    return ret;
}
