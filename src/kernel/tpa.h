#ifndef _TPA_H_INCLUDED
#define _TPA_H_INCLUDED

#include <allocator.h>
#include <stdint.h>

typedef struct tpa tpa_t;

/**
 * Allocate and initialize a new Thin Provisioned Array
 *
 * \param allocator Allocator function to use
 * \param entry_size Size of each entry
 * \returns A new TPA to use
 */
tpa_t* tpa_new(allocator_t* allocator, uint8_t entry_size);

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
 * \returns The index of the last entry in the last allocated page or -1
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

#endif
