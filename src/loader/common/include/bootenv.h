#pragma once

/**
 * \file bootenv.h
 * \brief specifies the interface a `bootenv` has to implement.
 *
 * Boot environment implementations are allowed to depend on all the functions defined in `platform.h`, which
 * defines the interface platforms have to implement. This includes functions for memory allocation for example.
 */

#include <stdbool.h>
#include <stdlib.h>

#include <loader.h>

/**
 * bootenv_init initializes the internal state of a boot environment implementation.
 *
 * It has to be called by the platform part of the bootloader before any other function defined in this file
 * is called, but after initialization is done, ensuring all functions defined in `platform.h` are already
 * working as expected.
 *
 * \returns pointer to internal state of the boot environment implementation. Do not attempt to do anything
 *          with this value, only pass it to the other functions defined in this file.
 */
void* bootenv_init();

/**
 * bootenv_boot finalizes preparing the boot environment and then starts the OS. This function will only return on
 * errors.
 *
 * \param bootenv is the pointer to internal state of the bootenv as returned by bootenv_init().
 * \returns only on errors.
 */
void bootenv_boot(void* bootenv);

/**
 * add_file is used by the platform part to add a file loaded into memory
 * into the boot environment for LF OS.
 *
 * \param bootenv is the pointer to internal state of the bootenv as returned by bootenv_init().
 * \param path is the full path on the filesystem to the loaded file.
 * \param contents is a pointer to a continous chunk of memory with the loaded contents of the file.
 * \param size specifies the size of the loaded file, also specifying the size of the memory chunk contents
 *        points to.
 * \returns `true` when everything went as planned, `false` on errors.
 */
bool add_file(void* bootenv, char const* path, void const* contents, size_t size);

/**
 * add_memory_region adds a region of memory to the boot environment for LF OS.
 *
 * The collection of all these memory regions is commonly called a memory map, giving the OS information
 * about the amount of physical memory present in the system, parts of that physical memory in use by hard- or
 * firmware. You should not add memory regions in use by the bootloader or by firmware only while bootservices
 * are running - only memory still in use after the OS is running should be added with this function.
 *
 * \param bootenv is the pointer to internal state of the bootenv as returned by bootenv_init().
 * \param region describes the memory region you want to add to the memory map.
 * \returns `true` when everything went as planned, `false` on errors.
 */
bool add_memory_region(void* bootenv, struct MemoryRegion const* region);

/**
 * add_framebuffers adds information about an initialized and ready-to-use framebuffer to the boot environment for LF OS.
 *
 * A framebuffer is defined as a memory area of size `scanline * height_px` where `scanline` is defined as
 * `max(width_px * pixel_bytes, stride_bytes)`. Any `stride_bytes` that is lower than `width_px * pixel_bytes` is to be
 * ignored by the OS.
 *
 * Previous versions of the LF OS boot environment specified `stride` in pixels instead of bytes, which probably does not
 * fit every use case, though this wasn't changed because of some observed breakage.
 *
 * \remarks Only a single framebuffer can be passed via the boot environment (specifically the `LoaderStruct`).
 *
 * \todo There is currently no way to specify where the individual color channels are stored in the area of a single pixel,
 *       LF OS assumes it is `BGRX` for 4 bytes per pixel.
 * \todo Currently LF OS does not support anything else than 4 bytes per pixel.
 *
 * \param bootenv is the pointer to internal state of the bootenv as returned by bootenv_init().
 * \param fb is a pointer to the start of the memory region of the framebuffer. Most of the time this is some sort of memory
 *        mapped I/O. The OS is responsible to configure useful memory access modes for this area (for example enabling write
 *        combine).
 * \param width_px specifies the width of the framebuffer in pixels, so for a framebuffer `1920x1080 32bpp` this contains `1920`.
 * \param height_px specifies the height of the framebuffer in pixels, so for a framebuffer `1920x1080 32bpp` this contains `1080`.
 * \param stride_bytes specifies the amount of bytes per scanline of the framebuffer. The `stride` is a hardware defined value, often
 *        something like `width_px * pixel_bytes` rounded to the next power of two or something, so for a framebuffer
 *        `1920x1080 32bpp` could be `2048`.
 * \param pixel_bytes specifies the amount of bytes used for a single pixel, so for a framebuffer `1920x1080 32bpp` this contains `4`.
 * \returns `true` on success, `false` on any errors.
 */
bool add_framebuffer(void* bootenv, void* fb, uint16_t width_px, uint16_t height_px, uint16_t stride_bytes, uint8_t pixel_bytes);
