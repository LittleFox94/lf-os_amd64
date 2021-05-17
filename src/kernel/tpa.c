/**
 * \file
 *
 * This file and tpa.h are the implementation of a thin provisioned array, an array with
 * unlimited size and holes for unused data.
 *
 * There is a small data overhead (8 byte per entry, could be 4 byte on 32bit platforms. aligned)
 * and quite some important runtime overhead (linked list to find the correct page, array access
 * in page).
 *
 * However, the implementation is simple and for many use cases this is a practical thing :)
 *
 * You can look at /t/kernel/tpa.c for some examples.
 *
 * License: MIT
 * Author:  Mara Sophie Grosch (littlefox@lf-net.org)
 */

#include <tpa.h>
#include <stdbool.h>
#include <string.h>

//! Header of a tpa page. Data follows after this until &tpa_page_header+tpa->page_size
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

    //! Size of each entry
    uint64_t entry_size;

    //! Size of each data page
    uint64_t page_size;

    //! Pointer to the first data page
    struct tpa_page_header* first;
};

tpa_t* tpa_new(allocator_t* alloc, uint64_t entry_size, uint64_t page_size, tpa_t* tpa) {
    if(!tpa) {
        tpa = alloc->alloc(alloc, sizeof(struct tpa));
    }

    tpa->allocator   = alloc;
    tpa->entry_size  = entry_size;
    tpa->page_size   = page_size;
    tpa->first       = 0;

    return tpa;
}

void tpa_delete(tpa_t* tpa) {
    struct tpa_page_header* current = tpa->first;
    while(current) {
        struct tpa_page_header* next = current->next;
        tpa->allocator->dealloc(tpa->allocator, current);
        current = next;
    }

    tpa->allocator->dealloc(tpa->allocator, tpa);
}

size_t tpa_size(tpa_t* tpa) {
    size_t res = tpa->page_size; // for the header
    struct tpa_page_header* current = tpa->first;
    while(current) {
        res += tpa->page_size;
        current = current->next;
    }

    return res;
}

size_t tpa_entries_per_page(tpa_t* tpa) {
    return ((tpa->page_size - sizeof(struct tpa_page_header)) / (tpa->entry_size + 8));
}

ssize_t tpa_length(tpa_t* tpa) {
    if(!tpa->first) return -1;

    struct tpa_page_header* current = tpa->first;
    while(current->next) {
        current = current->next;
    }

    return current->start_idx + tpa_entries_per_page(tpa);
}

uint64_t tpa_offset_in_page(tpa_t* tpa, uint64_t idx) {
    return sizeof(struct tpa_page_header) + ((tpa->entry_size + 8) * idx);
}

uint64_t* tpa_get_marker(tpa_t* tpa, struct tpa_page_header* page, uint64_t idx) {
    return (uint64_t*)((void*)page + tpa_offset_in_page(tpa, idx));
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

        current = current->next;
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

    tpa->allocator->dealloc(tpa->allocator, page);
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
            page = tpa->allocator->alloc(tpa->allocator, tpa->page_size);
            memset(page, 0, tpa->page_size);
            page->start_idx = (idx / tpa_entries_per_page(tpa)) * tpa_entries_per_page(tpa);

            struct tpa_page_header* current = tpa->first;
            while(current && current->next && current->next->start_idx < page->start_idx) {
                current = current->next;
            }

            if(current && current->start_idx < page->start_idx) {
                struct tpa_page_header* next = current->next;
                page->next = next;

                current->next = page;
                page->prev = current;

                if(next) {
                    next->prev = page;
                }
            }
            else {
                if(current && current->start_idx > page->start_idx) {
                    page->next = current;
                    page->prev = current->prev;
                    current->prev = page;
                }

                tpa->first = page;

            }
        }

        *(tpa_get_marker(tpa, page, idx - page->start_idx)) = 1;
        memcpy((void*)page + tpa_offset_in_page(tpa, idx - page->start_idx) + 8, data, tpa->entry_size);
    }
}

size_t tpa_next(tpa_t* tpa, size_t cur) {
    struct tpa_page_header* page;

    // seek to first page where our next entry could be in
    for(page = tpa->first; page && page->start_idx + tpa_entries_per_page(tpa) < cur; page = page->next) { }

    while(page) {
        for (size_t page_idx = (cur - page->start_idx); page_idx < tpa_entries_per_page(tpa); ++page_idx) {
            if(*(tpa_get_marker(tpa, page, page_idx))) {
                return page->start_idx + page_idx;
            }
        }

        page = page->next;

        if(page) {
            cur = page->start_idx;
        }
    }

    return 0;
}
