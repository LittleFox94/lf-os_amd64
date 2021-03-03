#ifndef _TPA_H_INCLUDED
#define _TPA_H_INCLUDED

#include <allocator.h>
#include <stdint.h>

typedef struct tpa tpa_t;

/**
 * Allocate and initialize a new Thin Provisioned Array
 *
 * \param alloc Allocator function to use
 * \param dealloc Deallocator function to use
 * \param entry_size Size of each entry
 * \param page_size Size of each data page
 * \param tpa Pointer to a memory region to use as tpa, 0 to alloc internally
 * \returns A new TPA to use
 */
tpa_t* tpa_new(allocator_t* alloc, deallocator_t* dealloc, uint64_t entry_size, uint64_t page_size, tpa_t* tpa);

/**
 * Deallocate every data in use by the given TPA And the TPA itself
 *
 * \param tpa The TPA to destroy
 */
void tpa_delete(tpa_t* tpa);

/**
 * Retrieve a given element from the TPA or NULL if the element is not present
 *
 * \param tpa The TPA
 * \param idx The index of the element to retrieve
 * \returns pointer to the first byte of the entry
 */
void* tpa_get(tpa_t* tpa, size_t idx);

/**
 * Set new data at the given index
 *
 * \remarks This function might have to allocate a new page and could take some time to complete
 * \param tpa The TPA
 * \param idx Where to place the new entry
 * \param data Pointer to the first byte of the data to store
 */
void tpa_set(tpa_t* tpa, size_t idx, void* data);

/**
 * Returns the allocated size of the TPA in bytes
 *
 * \remarks Has to search through the TPA and might be slow
 * \returns The size of all pages combined of this TPA in bytes
 */
size_t tpa_size(tpa_t* tpa);

/**
 * Retrieve the highest index currently possible without new allocation
 *
 * \remarks Has to search through the TPA and might be slow
 * \param tpa The TPA
 * \returns The index after the last entry in the last allocated page or -1
 */
ssize_t tpa_length(tpa_t* tpa);

/**
 * Counts the number of existing entries in the TPA
 *
 * \remarks Has to search through the TPA and might be slow
 * \param tpa The TPA
 * \returns The number of entries currently set in the TPA
 */
size_t tpa_entries(tpa_t* tpa);

/**
 * Returns the next non-empty element after cur
 *
 * \remarks Has to iterate through the pages to get the page to the cur idx
 * \param tpa The TPA
 * \param cur The current index
 * \returns The next non-empty element index after cur. Special case: returns 0 when nothing found, you
 *          are expected to check if the returned value is larger than cur to check if something was found.
 */
size_t tpa_next(tpa_t* tpa, size_t cur);

#endif
