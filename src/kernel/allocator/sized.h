#ifndef _ALLOCATOR_SIZED_H_INCLUDED
#define _ALLOCATOR_SIZED_H_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <forward_list>
#include <bitset>

#include "page.h"

class SizedAllocatorBase {
    protected:
        template<size_t S>
        struct Helpers {
            struct slot_t {
                uint8_t v[S];
            }__attribute__((packed));

            template<size_t PageSize>
            class page_t {
                private:
                    struct header_t {
                        size_t max_continuous_free;
                    };

                    using bitmap_t = std::bitset<PageSize / S>;

                public:
                    static const size_t max_entries = (
                        PageSize
                      - sizeof(header_t)
                      - sizeof(bitmap_t)
                      - 8 // pages are stored in a std::forward_list, which needs another 8 bytes per entry
                    ) / S;

                private:
                    header_t _header;
                    bitmap_t _bitmap;
                    slot_t   _data[max_entries];

                public:
                    page_t() {
                        _header.max_continuous_free = max_entries;
                    }

                    size_t num_free() const {
                        return max_entries - _bitmap.count();
                    }

                    bool empty() const {
                        return _bitmap.none();
                    }

                    slot_t* allocate(size_t n) {
                        if(_header.max_continuous_free < n) {
                            return 0;
                        }

                        for(size_t candidate = 0; candidate < max_entries - n; ++candidate) {
                            bool success = true;
                            for(size_t i = 0; i < n; ++i) {
                                if(_bitmap.test(candidate + i)) {
                                    success = false;
                                    candidate += i;
                                    break;
                                }
                            }

                            if(success) {
                                for(size_t i = 0; i < n; ++i) {
                                    _bitmap.set(candidate + i);
                                }

                                update_header();
                                return &_data[candidate];
                            }
                        }

                        // can we get here, with the check if we have enough continues
                        // entries in this page on entry of this method?
                        return 0;
                    }

                    bool deallocate(slot_t* p, size_t n) {
                        uint64_t base_addr = reinterpret_cast<uint64_t>(_data);
                        uint64_t addr      = reinterpret_cast<uint64_t>(p);

                        if(addr < base_addr || addr >= base_addr + (max_entries * S)) {
                            return false;
                        }

                        if((addr - base_addr) % S != 0) {
                            logf("SizedAllocator", "Unaligned deallocate requested! tried to deallocate 0x%x, page data based at 0x%x and slot size %d", addr, base_addr, S);
                            return false;
                        }

                        for(size_t index = 0; index < max_entries; ++index) {
                            if(&_data[index] == p) {
                                for(size_t i = 0; i < n; ++i) {
                                    _bitmap.reset(index + i);
                                }
                            }
                        }


                        update_header();
                        return true;
                    }

                private:
                    void update_header() {
                        size_t max_free = 0;
                        size_t current_count = 0;
                        for(size_t i = 0; i < max_entries; ++i) {
                            if(_bitmap.test(i)) {
                                if(current_count > max_free) {
                                    max_free = current_count;
                                }
                                current_count = 0;
                            } else {
                                ++current_count;
                            }
                        }

                        if(current_count > max_free) {
                            max_free = current_count;
                        }
                        _header.max_continuous_free = max_free;
                    }
            };
        };
};

template<
    size_t S,
    size_t PageSize = (
        S > 1 * MiB ? 1 * GiB
      : S > 1 * KiB ? 2 * MiB
      :               4 * KiB
    ),
    class PageAllocator = ::PageAllocator
>
class SizedAllocator : protected SizedAllocatorBase {
    using page_t     = Helpers<S>::template page_t<PageSize>;
    using slot_t     = Helpers<S>::slot_t;
    using list_t     = std::forward_list<page_t, typename PageAllocator::template allocator<page_t, PageSize>>;

    static list_t pages;

    public:
        using value_type = slot_t*;

        template<class T>
        T* allocate(size_t n) {
            if(n > page_t::max_entries) {
                return 0; // can't allocate more than what fits in a single page
            }

            auto last_page = pages.begin();
            if (last_page == pages.end()) {
                return reinterpret_cast<T*>(pages.emplace_front()->allocate(n));
            }

            for(auto it = pages.begin(); it != pages.end(); ++it) {
                slot_t* ret = it->allocate(n);
                if(ret) {
                    return reinterpret_cast<T*>(ret);
                }

                last_page = it;
            }

            auto new_page = pages.emplace_after(last_page);
            return reinterpret_cast<T*>(new_page->allocate(n));
        }

        template<class T>
        void deallocate(T* p, size_t n) {
            auto previous = pages.end();

            for(auto it = pages.begin(); it != pages.end(); ++it) {
                if(it->deallocate(reinterpret_cast<slot_t*>(p), n)) {
                    if(it->empty()) {
                        if(previous != pages.end()) {
                            pages.erase_after(previous);
                        } else {
                            pages.pop_front();
                        }
                    }
                    return;
                }

                previous = it;
            }

            loge("SizedAllocator", "Cannot deallocate allocation of %d entries at 0x%x: page not found!", n, p);
        }
};

template<
    size_t S,
    size_t PageSize,
    class PageAllocator
>
SizedAllocator<S, PageSize, PageAllocator>::list_t SizedAllocator<S, PageSize, PageAllocator>::pages;

#endif
