#ifndef _TPA_H_INCLUDED
#define _TPA_H_INCLUDED

#include <allocator.h>
#include <stdint.h>
#include <string.h>

template<typename T> class TPA {
    friend class TpaTest;

    public:
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
        static TPA<T>* create(allocator_t* alloc, uint64_t page_size, TPA* tpa) {
            if(!tpa) {
                tpa = (TPA<T>*)alloc->alloc(alloc, sizeof(TPA<T>));
            }

            tpa->allocator   = alloc;
            tpa->entry_size  = sizeof(T);
            tpa->page_size   = page_size;
            tpa->first       = 0;

            return tpa;
        }

        /**
         * Deallocate every data in use by the given TPA and the TPA itself
         */
        void destroy() {
            tpa_page_header* current = this->first;
            while(current) {
                struct tpa_page_header* next = current->next;
                this->allocator->dealloc(this->allocator, current);
                current = next;
            }

            this->allocator->dealloc(this->allocator, this);
        }

        /**
         * Retrieve a given element from the TPA or NULL if the element is not present
         *
         * \param tpa The TPA
         * \param idx The index of the element to retrieve
         * \returns pointer to the first byte of the entry
         */
        T* get(size_t idx) {
            tpa_page_header* page = this->get_page_for_idx(idx);

            if(page) {
                if(this->entry_exists_in_page(page, idx - page->start_idx)) {
                    return (T*)((uint64_t)page + this->offset_in_page(idx - page->start_idx) + 8); // 8: skip marker
                }
            }

            return 0;
        }

        /**
         * Set new data at the given index
         *
         * \remarks This function might have to allocate a new page and could take some time to complete
         * \param idx Where to place the new entry
         * \param data Pointer to the first byte of the data to store
         */
        void set(size_t idx, T* data) {
            tpa_page_header* page = this->get_page_for_idx(idx);

            if(!data) {
                if(page) {
                    *(this->get_marker(page, idx - page->start_idx)) = 0;
                    this->clean_page(page);
                    return;
                }
            }
            else {
                if(!page) {
                    page = (tpa_page_header*)this->allocator->alloc(this->allocator, this->page_size);
                    memset(page, 0, this->page_size);
                    page->start_idx = (idx / this->entries_per_page()) * this->entries_per_page();

                    tpa_page_header* current = this->first;
                    while(current && current->next && current->next->start_idx < page->start_idx) {
                        current = current->next;
                    }

                    if(current && current->start_idx < page->start_idx) {
                        tpa_page_header* next = current->next;
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

                        this->first = page;

                    }
                }

                *(this->get_marker(page, idx - page->start_idx)) = 1;
                memcpy((void*)((uint64_t)page + this->offset_in_page(idx - page->start_idx) + 8), data, this->entry_size);
            }
        }

        /**
         * Returns the allocated size of the TPA in bytes
         *
         * \remarks Has to search through the TPA and might be slow
         * \returns The size of all pages combined of this TPA in bytes
         */
        size_t size() {
            size_t res = this->page_size; // for the header
            tpa_page_header* current = this->first;
            while(current) {
                res += this->page_size;
                current = current->next;
            }

            return res;
        }

        /**
         * Retrieve the highest index currently possible without new allocation
         *
         * \remarks Has to search through the TPA and might be slow
         * \returns The index after the last entry in the last allocated page or -1
         */
        ssize_t length() {
            if(!this->first) return -1;

            tpa_page_header* current = this->first;
            while(current->next) {
                current = current->next;
            }

            return current->start_idx + this->entries_per_page();
        }

        /**
         * Counts the number of existing entries in the TPA
         *
         * \remarks Has to search through the TPA and might be slow
         * \returns The number of entries currently set in the TPA
         */
        size_t entries() {
            size_t res = 0;

            tpa_page_header* current = this->first;
            while(current) {
                for(uint64_t i = 0; i < this->entries_per_page(); ++i) {
                    if(this->entry_exists_in_page(current, i)) {
                        ++res;
                    }
                }

                current = current->next;
            }

            return res;
        }

        /**
         * Returns the next non-empty element after cur
         *
         * \remarks Has to iterate through the pages to get the page to the cur idx
         * \param cur The current index
         * \returns The next non-empty element index after cur. Special case: returns 0 when nothing found, you
         *          are expected to check if the returned value is larger than cur to check if something was found.
         */
        size_t next(size_t cur) {
            tpa_page_header* page;

            // seek to first page where our next entry could be in
            for(page = this->first; page && page->start_idx + this->entries_per_page() < cur; page = page->next) { }

            while(page) {
                for (size_t page_idx = (cur - page->start_idx); page_idx < this->entries_per_page(); ++page_idx) {
                    if(*(this->get_marker(page, page_idx))) {
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

    private:
        //! Header of a tpa page. Data follows after this until &tpa_page_header+tpa->page_size
        struct tpa_page_header {
            //! Pointer to the next page
            struct tpa_page_header* next;
            struct tpa_page_header* prev;

            //! First index in this page
            uint64_t start_idx;
        };

        size_t entries_per_page() {
            return ((this->page_size - sizeof(struct tpa_page_header)) / (this->entry_size + 8));
        }

        uint64_t offset_in_page(uint64_t idx) {
            return sizeof(struct tpa_page_header) + ((this->entry_size + 8) * idx);
        }

        uint64_t* get_marker(struct tpa_page_header* page, uint64_t idx) {
            return (uint64_t*)((uint64_t)page + this->offset_in_page(idx));
        }

        bool entry_exists_in_page(struct tpa_page_header* page, uint64_t idx) {
            return *(this->get_marker(page, idx)) != 0;
        }

        tpa_page_header* get_page_for_idx(uint64_t idx) {
            tpa_page_header* current = this->first;
            while(current) {
                if(current->start_idx <= idx && current->start_idx + this->entries_per_page() > idx) {
                    return current;
                }

                if(current->start_idx > idx) {
                    break;
                }

                current = current->next;
            }

            return 0;
        }

        void clean_page(tpa_page_header* page) {
            for(uint64_t idx = 0; idx < this->entries_per_page(); ++idx) {
                // if an entry is in use we cannot delete the page
                if(this->entry_exists_in_page(page, idx)) {
                    return;
                }
            }

            if(page->prev) {
                page->prev->next = page->next;
            }
            else {
                this->first = page->next;
            }

            this->allocator->dealloc(this->allocator, page);
        }

        //! Allocator for new tpa pages
        allocator_t* allocator;

        //! Size of each entry
        uint64_t entry_size;

        //! Size of each data page
        uint64_t page_size;

        //! Pointer to the first data page
        struct tpa_page_header* first;

};

#endif
