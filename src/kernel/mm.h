#ifndef _MM_H_INCLUDED
#define _MM_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

#define KiB (1024)
#define MiB (1024ULL * KiB)
#define GiB (1024ULL * MiB)
#define TiB (1024ULL * GiB)

typedef enum {
    MM_UNKNOWN,
    MM_FREE,
    MM_RESERVED,
    MM_UEFI_MAPPING_REQUIRED,
} mm_page_status_t;

typedef enum {
    MM_UNCACHED,
    MM_WRITETHROUGH,
    MM_WRITEBACK,
} mm_caching_mode_t;

void* mm_alloc_pages(uint64_t count);

/**
 * set the caching mode for the pages starting at start until start + len
 *
 * \param mode caching mode to set
 * \param start start address
 * \param len number of bytes
 */
void mm_set_caching_mode(mm_caching_mode_t mode, uint64_t start, size_t len);

/**
 * Mark physical pages with information from loader memory map
 *
 * \param start Start of the physical memory region to mark
 * \param count Number of pages
 * \param status Page status of the given memory region
 */
void mm_mark_physical_pages(uint64_t start, uint64_t count, mm_page_status_t status);

/**
 * Make page writeable
 *
 * \param address Address to modify
 * \param writeable Shall the page be writeable or readonly?
 */
void mm_make_writeable(uint64_t address, int writeable);

void mm_print_physical_free_regions(void);

void mm_bootstrap(uint64_t usable_page);

uint64_t mm_highest_address(void);

#endif
