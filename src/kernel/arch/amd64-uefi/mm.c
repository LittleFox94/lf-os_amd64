#include "arch.h"
#include "mm.h"

ptr_t mm_get_pml4_address() {
    ptr_t pml4_address = 0;
    asm volatile("mov %%cr3, %0":"=r"(pml4_address));
    return pml4_address;
}

short mm_get_pml4_index(ptr_t address) {
    return (address >> 39) & 0x1FF;
}

short mm_get_pdp_index(ptr_t address) {
    return (address >> 30) & 0x1FF;
}

short mm_get_pd_index(ptr_t address) {
    return (address >> 21) & 0x1FF;
}

short mm_get_pt_index(ptr_t address) {
    return (address >> 12) & 0x1FF;
}

ptr_t mm_get_mapping(ptr_t address) {
    ptr_t pml4_index = mm_get_pml4_index(address);
    ptr_t pdp_index  = mm_get_pdp_index(address);
    ptr_t pd_index   = mm_get_pd_index(address);
    ptr_t pt_index   = mm_get_pt_index(address);

    uint64_t* pml4 = (uint64_t*)mm_get_pml4_address();

    // check if pml4_entry is present
    if(pml4[pml4_index] & 1) {
        uint64_t* pdp = (uint64_t*)(pml4[pml4_index] & 0xFFFFFFFFFF000);
        
        // check if pdp_entry is present
        if(pdp[pdp_index] & 1) {
            // check if last level
            if(pdp[pdp_index] & 0x80) {
                return (pdp[pdp_index] & 0xFFFFF80000000) | (address & 0x3FFFFFFF);
            }
            else {
                uint64_t* pd = (uint64_t*)(pdp[pdp_index] & 0xFFFFFFFFFF000);

                // check if pd_entry is present
                if(pd[pd_index] & 1) {
                    // check if last level
                    if(pd[pd_index] & 0x80) {
                        return (pd[pd_index] & 0xFFFFFFFE00000) | (address & 0x1FFFFF);
                    }
                    else {
                        uint64_t* pt = (uint64_t*)(pd[pd_index] & 0xFFFFFFFFFF000);

                        // check if pt_entry is present
                        if(pt[pt_index] & 1) {
                            return pt[pt_index] & 0xFFFFFFFFFF000;
                        }
                    }
                }
            }
        }
    }
    
    // nope
    return 0;
}
