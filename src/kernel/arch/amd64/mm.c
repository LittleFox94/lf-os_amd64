#include "arch.h"
#include "mm.h"
#include "vm.h"
#include "fbconsole.h"
#include "bluescreen.h"

#define PRESENT_BIT 1

typedef struct {
    ptr_t               start;
    uint64_t            count;
    void*               next;
    mm_page_status_t    status;
} mm_page_list_entry_t;

mm_page_list_entry_t* mm_physical_page_list = 0;

void mm_del_page_list_entry(mm_page_list_entry_t* entry) {
    entry->start  = 0;
    entry->count  = 0;
    entry->status = MM_UNKNOWN;
}

void* mm_alloc_pages(uint64_t count) {
    mm_page_list_entry_t* current = mm_physical_page_list;

    while(current) {
        if(current->status == MM_FREE && current->count >= count) {
            current->count -= count;
            void* ret = (void*)current->start;

            if(current->count > 2) {
                current->start += 4096 * count;
            }
            else {
                mm_del_page_list_entry(current);
            }

            return ret;
        }

        current = current->next;
    }

    panic_message("Out of memory in mm_alloc_pages");
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

    mm_page_list_entry_t* new = (mm_page_list_entry_t*)vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_KERNEL_HEAP, 1);

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
    mm_page_list_entry_t* page_list = (mm_page_list_entry_t*)usable_page;
    mm_physical_page_list = page_list;

    for(int i = 0; i < 4096 / sizeof(mm_page_list_entry_t); i++) {
        page_list[i].start  = 0;
        page_list[i].count  = 0;
        page_list[i].status = MM_UNKNOWN;
        page_list[i].next   = &page_list[i + 1];
    }

    page_list[(4096 / sizeof(mm_page_list_entry_t)) - 1].next = 0;
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
        panic_message("mm not bootstrapped with free page first!");
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

ptr_t mm_highest_address() {
    ptr_t res = 0;
    mm_page_list_entry_t* current = mm_physical_page_list;

    while(current) {
        if(current->start + current->count > res) {
            res = current->start + (current->count * 4096);
        }

        current = current->next;
    }

    return res;
}
