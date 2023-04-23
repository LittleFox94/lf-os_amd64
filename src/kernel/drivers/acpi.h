#ifndef _ACPI_H_INCLUDED
#define _ACPI_H_INCLUDED

#include <efi.h>
#include <stdint.h>

struct acpi_table_header {
    char     signature[4];
    uint32_t length;
    uint8_t  revision;
    uint8_t  checksum;
    char     oem_id[6];
    char     oem_table_id[8];
    uint32_t oem_revision;
    char     asl_compiler_id[4];
    uint32_t asl_compiler_revision;
}__attribute__((packed));

static const uint8_t acpi_address_space_memory = 0;
static const uint8_t acpi_address_space_io     = 1;

struct acpi_address {
    uint8_t  address_space;
    uint8_t  register_bit_width;
    uint8_t  register_bit_offset;
    uint8_t  _reserved;
    uint64_t address;
}__attribute__((packed));

void init_acpi_efi(EFI_SYSTEM_TABLE* efiST);
uint8_t acpi_checksum(void* data, size_t len);

#endif // _ACPI_H_INCLUDED
