#ifndef _LOADER_H_INCLUDED
#define _LOADER_H_INCLUDED

#include <stdint.h>

//! Main interface between loader and kernel
typedef struct {
    //! Signature. always 0x17a15174545c8b4f
    uint64_t signature;

    //! Size of this structure including appended structs. Should fit in an
    //  uint16_t most of the time, but this way it's more efficient and future
    //  proof (ramdisk anyone?)
    uint64_t size;

    //! Location of framebuffer as physical address
    ptr_t fb_location;

    //! Width of the framebuffer in pixels
    uint16_t fb_width;

    //! Height of the framebuffer in pixels
    uint16_t fb_height;

    //! Bytes per pixel for the framebuffer
    uint8_t fb_bpp;

    //! reserved location
    uint8_t  fb_reserved1;
    //! reserved location
    uint16_t fb_reserved2;

    //! Where the kernel was loaded. Just to make things future proof, but most
    //  likely 0xFFFF80000000 (i.e. the start of the higher half).
    uint64_t kernel_location;

    //! Size of the loaded kernel image. kernel_location + kernel_size gives
    //  the end of the kernel image, pad to a full page and you get the start
    //  of the scratchpad memory.
    uint64_t kernel_size;

    //! Number of loaded files
    uint64_t num_files;

    uint64_t num_mem_desc;
}__attribute__((packed)) LoaderStruct;

#endif
