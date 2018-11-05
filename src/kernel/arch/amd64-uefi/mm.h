#ifndef _MM_H_INCLUDED
#define _MM_H_INCLUDED

#include "arch.h"

ptr_t mm_get_pml4_address();

/**
 * return index of address in PML4 (level 4) structure
 *
 * \param address Adress to get pml4-index for
 * \returns index in pml4 for given address
 */
short mm_get_pml4_index(ptr_t address);
    
/**
 * return index of address in page directory pointer (level 3) structure
 *
 * \param address Adress to get pdp-index for
 * \returns index in pdp for given address
 */
short mm_get_pdp_index(ptr_t address);

/**
 * return index of address in page directory (level 2) structure
 *
 * \param address Adress to get pd-index for
 * \returns index in pd for given address
 */
short mm_get_pd_index(ptr_t address);

/**
 * return index of address in page table (level 1) structure
 *
 * \param address Adress to get pt-index for
 * \returns index in pt for given address
 */
short mm_get_pt_index(ptr_t address);

/**
 * return the mapped physical address of the given virtual address
 *
 * \param address Address to retrieve physical address for
 * \returns physical address mapped to the given virtual address
 */
ptr_t mm_get_mapping(ptr_t address);

#endif
