#include "arch.h"
#include "mm.h"
#include "vm.h"
#include "fbconsole.h"

#define PRESENT_BIT 1

typedef struct {
    ptr_t               start;
    uint64_t            count;
    void*               next;
    mm_page_status_t    status;
} mm_page_list_entry_t;

mm_page_list_entry_t* mm_physical_page_list = 0;

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

    if(pml4[pml4_index] & PRESENT_BIT) {
        uint64_t* pdp = (uint64_t*)(pml4[pml4_index] & 0xFFFFFFFFFF000);

        if(pdp[pdp_index] & PRESENT_BIT) {
            // check if last level
            if(pdp[pdp_index] & 0x80) {
                return (pdp[pdp_index] & 0xFFFFF80000000) | (address & 0x3FFFFFFF);
            }
            else {
                uint64_t* pd = (uint64_t*)(pdp[pdp_index] & 0xFFFFFFFFFF000);

                if(pd[pd_index] & PRESENT_BIT) {
                    // check if last level
                    if(pd[pd_index] & 0x80) {
                        return (pd[pd_index] & 0xFFFFFFFE00000) | (address & 0x1FFFFF);
                    }
                    else {
                        uint64_t* pt = (uint64_t*)(pd[pd_index] & 0xFFFFFFFFFF000);

                        if(pt[pt_index] & PRESENT_BIT) {
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

void mm_del_page_list_entry(mm_page_list_entry_t* entry) {
    entry->start  = 0;
    entry->count  = 0;
    entry->status = MM_UNKNOWN;
}

void* mm_alloc_kernel_pages(uint64_t count) {
    mm_page_list_entry_t* current = mm_physical_page_list;

    while(current) {
        if(current->status == MM_FREE && current->count >= count) {
            current->count -= count;
            void* ret = (void*)current->start;

            if(current->count) {
                current->start += 4096 * count;
            }
            else {
                mm_del_page_list_entry(current);
            }

            return ret;
        }

        current = current->next;
    }

    return 0; // TODO: again, this should crash as we ran out of memory.
}

mm_page_list_entry_t* mm_get_page_list_entry(mm_page_list_entry_t* start) {
    mm_page_list_entry_t* previous = 0;

    while(start) {
        if(start->status == MM_UNKNOWN && start->start == 0 && start->count == 0) {
            return start;
        }

        previous = start;
        start    = start->next;
    }

    mm_page_list_entry_t* new = mm_alloc_kernel_pages(1);

    for(int i = 0; i < 4096 / sizeof(mm_page_list_entry_t); i++) {
        new[i].start  = 0;
        new[i].count  = 0;
        new[i].status = MM_UNKNOWN;

        if(i) {
            new[i - 1].next = new + i;
        }
    }

    previous->next = new;
    return new;
}

void mm_bootstrap(ptr_t usable_page) {
    while(usable_page % 4096);

    // bootstrap memory management with the first page given to us
    mm_page_list_entry_t* page_list = mm_physical_page_list = (mm_page_list_entry_t*)usable_page;

    for(int i = 0; i < 4096 / sizeof(mm_page_list_entry_t); i++) {
        page_list[i].start  = 0;
        page_list[i].count  = 0;
        page_list[i].status = MM_UNKNOWN;
        page_list[i].next   = 0;
    }
}

void mm_mark_physical_pages(ptr_t start, uint64_t count, mm_page_status_t status) {
    if(!count) {
        return;
    }

    if(!mm_physical_page_list && status == MM_FREE) {
        mm_bootstrap(start);
        return;
    }
    else if(!mm_physical_page_list && status != MM_FREE) {
        while(1); // this should crash, but there is no crash handling yet, so just halt
    }

    mm_page_list_entry_t* current = mm_physical_page_list;
    while(current) {
        if(start >= current->start && start + (count * 4096) <= current->start + (current->count * 4096)) {
            if(current->status == status) {
                return;
            }
            else {
                mm_page_status_t old_status = current->status;

                ptr_t start_first     = current->start;
                uint64_t count_first  = (start - start_first) / 4096;

                if(count_first == 0) {
                    mm_del_page_list_entry(current);
                }
                else {
                    current->start = start_first;
                    current->count = count_first;
                }

                ptr_t start_second    = start + (count * 4096);
                uint64_t count_second = (current->start + (current->count * 4096)) - (start + (count * 4096));

                if(count_second) {
                    mm_page_list_entry_t* entry = mm_get_page_list_entry(mm_physical_page_list);
                    entry->start  = start_second;
                    entry->count  = count_second;
                    entry->status = old_status;
                }

                mm_page_list_entry_t* entry = mm_get_page_list_entry(mm_physical_page_list);
                entry->start  = start;
                entry->count  = count;
                entry->status = status;

                return;
            }
        }

        current = current->next;
    }

    mm_page_list_entry_t* new = mm_get_page_list_entry(mm_physical_page_list);
    new->start  = start;
    new->count  = count;
    new->status = status;
}

void mm_print_physical_free_regions() {
    mm_page_list_entry_t* current = mm_physical_page_list;

    while(current) {
        if(current->status == MM_FREE) {
            fbconsole_write(" --> %d free pages starting at 0x%x\n", current->count, current->start);
        }

        current = current->next;
    }
}

