#include <tpa.h>
#include <stdbool.h>
#include <string.h>

//! Header of a tpa page. Data follows after this until tpa_page_header+4096
struct tpa_page_header {
    //! Pointer to the next page
    struct tpa_page_header* next;
    struct tpa_page_header* prev;

    //! First index in this page
    uint64_t start_idx;
};

//! Header of a TPA
struct tpa {
    //! Allocator for new tpa pages
    allocator_t* allocator;

    //! Deallocator for tpa pages no longer in use
    deallocator_t* deallocator;

    //! Size of each entry
    uint8_t entry_size;

    //! Pointer to the first data page
    struct tpa_page_header* first;
};

tpa_t* tpa_new(allocator_t* alloc, deallocator_t* dealloc, uint8_t entry_size) {
    tpa_t* tpa = alloc(4096);
    tpa->allocator   = alloc;
    tpa->deallocator = dealloc;
    tpa->entry_size  = entry_size;
    tpa->first       = 0;

    return tpa;
}

void tpa_delete(tpa_t* tpa) {
    struct tpa_page_header* current = tpa->first;
    while(current) {
        struct tpa_page_header* next = current->next;
        tpa->deallocator(current);
        current = next;
    }

    tpa->deallocator(tpa);
}

size_t tpa_size(tpa_t* tpa) {
    size_t res = 4096; // for the header
    struct tpa_page_header* current = tpa->first;
    while(current) {
        res += 4096;
        current = current->next;
    }

    return res;
}

size_t tpa_entries_per_page(tpa_t* tpa) {
    return ((4096-sizeof(struct tpa_page_header)) / (tpa->entry_size + 8));
}

ssize_t tpa_length(tpa_t* tpa) {
    if(!tpa->first) return -1;

    struct tpa_page_header* current = tpa->first;
    while(current->next) {
        current = current->next;
    }

    return current->start_idx + tpa_entries_per_page(tpa);
}

uint16_t tpa_offset_in_page(tpa_t* tpa, uint64_t idx) {
    return sizeof(struct tpa_page_header) + ((tpa->entry_size + 8) * idx);
}

uint64_t* tpa_get_marker(tpa_t* tpa, struct tpa_page_header* page, uint64_t idx) {
    return (uint64_t*)((void*)page + tpa_offset_in_page(tpa, idx - page->start_idx));
}

bool tpa_entry_exists_in_page(tpa_t* tpa, struct tpa_page_header* page, uint64_t idx) {
    return *(tpa_get_marker(tpa, page, idx)) != 0;
}

size_t tpa_entries(tpa_t* tpa) {
    size_t res = 0;

    struct tpa_page_header* current = tpa->first;
    while(current) {
        for(uint64_t i = 0; i < tpa_entries_per_page(tpa); ++i) {
            if(tpa_entry_exists_in_page(tpa, current, i)) {
                ++res;
            }
        }

        current = current->next;
    }
    
    return res;
}

struct tpa_page_header* tpa_get_page_for_idx(tpa_t* tpa, uint64_t idx) {
    struct tpa_page_header* current = tpa->first;
    while(current) {
        if(current->start_idx <= idx && current->start_idx + tpa_entries_per_page(tpa) > idx) {
            return current;
        }

        if(current->start_idx > idx) {
            break;
        }
    }

    return 0;
}

void* tpa_get(tpa_t* tpa, uint64_t idx) {
    struct tpa_page_header* page = tpa_get_page_for_idx(tpa, idx);

    if(page) {
        if(tpa_entry_exists_in_page(tpa, page, idx - page->start_idx)) {
            return (void*)page + tpa_offset_in_page(tpa, idx - page->start_idx) + 8; // 8: skip marker
        }
    }

    return 0;
}

void tpa_clean_page(tpa_t* tpa, struct tpa_page_header* page) {
    for(uint64_t idx = 0; idx < tpa_entries_per_page(tpa); ++idx) {
        // if an entry is in use we cannot delete the page
        if(tpa_entry_exists_in_page(tpa, page, idx)) {
            return;
        }
    }

    if(page->prev) {
        page->prev->next = page->next;
    }
    else {
        tpa->first = page->next;
    }

    tpa->deallocator(page);
}

void tpa_set(tpa_t* tpa, uint64_t idx, void* data) {
    struct tpa_page_header* page = tpa_get_page_for_idx(tpa, idx);

    if(!data) {
        if(page) {
            *(tpa_get_marker(tpa, page, idx - page->start_idx)) = 0;
            tpa_clean_page(tpa, page);
            return;
        }
    }
    else {
        if(!page) {
            page = tpa->allocator(4096);
            page->start_idx = (idx / tpa_entries_per_page(tpa)) * tpa_entries_per_page(tpa);

            struct tpa_page_header* current = tpa->first;
            while(current && current->next && current->next->start_idx > page->start_idx) {
                current = current->next;
            }

            if(current) {
                struct tpa_page_header* next = current->next;
                current->next = page;
                page->next = next;
                page->prev = current;

                if(next) {
                    next->prev = page;
                }
            }
            else {
                tpa->first = page;
            }
        }

        *(tpa_get_marker(tpa, page, idx - page->start_idx)) = 1;
        memcpy((void*)page + tpa_offset_in_page(tpa, idx - page->start_idx) + 8, data, tpa->entry_size);
    }
}
