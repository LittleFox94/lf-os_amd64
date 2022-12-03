#ifndef _LOADER_H_INCLUDED
#define _LOADER_H_INCLUDED

#include <stdint.h>

static const uint64_t LFOS_LOADER_SIGNATURE = 0x17a15174545c8b4f;

// basic "what is this memory for" attributes
//! memory is free to be used
#define MEMORY_REGION_USABLE        (1ULL << 0)
//! memory is in use by the firmware
#define MEMORY_REGION_FIRMWARE      (1ULL << 1)
//! memory holds code
#define MEMORY_REGION_CODE          (1ULL << 2)
//! memory supports being mapped read-only
#define MEMORY_REGION_RO            (1ULL << 3)

// "what kinda memory is this"
//! memory is non-volatile
#define MEMORY_REGION_NV            (1ULL << 4)
//! memory is more reliable than other memory in the system
#define MEMORY_REGION_MORE_RELIABLE (1ULL << 5)

// cacheability flags
//! supports uncachable
#define MEMORY_REGION_UC    (1ULL << 16)
//! supports write-combine
#define MEMORY_REGION_WC    (1ULL << 17)
//! supports write-through
#define MEMORY_REGION_WT    (1ULL << 18)
//! supports write-back
#define MEMORY_REGION_WB    (1ULL << 19)
//! supports uncacheable, exported and "fetch and add" semaphore mechanism
#define MEMORY_REGION_UCE   (1ULL << 20)

//! Supports write-protected
#define MEMORY_REGION_WP    (1ULL << 21)
//! Supports read-protected
#define MEMORY_REGION_RP    (1ULL << 22)
//! Supports execute-protected
#define MEMORY_REGION_XP    (1ULL << 23)

//! Main interface between loader and kernel
struct LoaderStruct {
    //! Signature. always 0x17a15174545c8b4f
    uint64_t signature;

    /** Size of this structure, allowing us to add fields at the end for new features.
     */
    uint16_t size;

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
