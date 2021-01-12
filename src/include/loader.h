#ifndef _LOADER_H_INCLUDED
#define _LOADER_H_INCLUDED

#include <stdint.h>

static const uint64_t LFOS_LOADER_SIGNATURE = 0x17a15174545c8b4f;

#define MEMORY_REGION_USABLE        1ULL
#define MEMORY_REGION_FIRMWARE      2ULL
#define MEMORY_REGION_CODE          4ULL

//! Main interface between loader and kernel
struct LoaderStruct {
    //! Signature. always 0x17a15174545c8b4f
    uint64_t signature;

    //! Size of this structure including appended structs. Should fit in an
    //  uint16_t most of the time, but this way it's more efficient and future
    //  proof (ramdisk anyone?)
    uint64_t size;

    //! Location of framebuffer as physical address
    ptr_t fb_location;

    //! Width of the framebuffer in visible pixels
    uint16_t fb_width;

    //! Height of the framebuffer in pixels
    uint16_t fb_height;

    //! Bytes per pixel for the framebuffer
    uint8_t fb_bpp;

    //! reserved location
    uint8_t  fb_reserved1;

    //! Pixels per scanline (including invisible pixels)
    uint16_t fb_stride;

    //! Number of memory descriptors
    uint64_t num_mem_desc;

    //! Number of loaded files
    uint64_t num_files;

    //! Firmware info data, e.g. EFI_SYSTEM_TABLE on UEFI
    ptr_t firmware_info;
}__attribute__((packed));

//! Describes a single memory region
struct MemoryRegion {
    //! Start of the region as physical address
    ptr_t  start_address;

    //! Number of pages, where page size is 4096 bytes
    size_t num_pages;

    //! Flags for the memory region. See MEMORY_REGION_ defines
    uint64_t flags;
}__attribute__((packed));

struct FileDescriptor {
    //! Zero terminated string with the name of the file
    char name[256];

    //! Size of the loaded file
    uint64_t size;

    //! Offset where the file contents are located after LoaderStruct
    uint64_t offset;
}__attribute__((packed));

#endif
