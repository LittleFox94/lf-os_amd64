#include <mm.h>
#include <vm.h>
#include <log.h>
#include <panic.h>
#include <allocator/typed.h>

#include <forward_list>
#include <optional>

struct mm_page_list_entry {
    const uint64_t         start  = 0;
    const uint64_t         count  = 0;
    const mm_page_status_t status = MM_INVALID;

    uint64_t end() const {
        return start + (count * 4096);
    }

    bool intersects(const mm_page_list_entry& other) const {
        return (
            (start <= other.start && end() > other.start)  ||
            (start >= other.start && end() <= other.end()) ||

            (other.start <= start && other.end() > start)  ||
            (other.start >= start && other.end() <= end())
        );
    }

    std::optional<mm_page_list_entry> part_before(const mm_page_list_entry& other) const {
        if(!intersects(other)) {
            return {};
        }

        if(start >= other.start) {
            return {};
        }

        auto new_end = other.start;
        if(new_end == start) {
            return {};
        }

        return mm_page_list_entry{
            start,
            (new_end - start) / 4096,
            status,
        };
    }

    std::optional<mm_page_list_entry> part_after(const mm_page_list_entry& other) const {
        if(!intersects(other)) {
            return {};
        }

        if(end() <= other.end()) {
            return {};
        }

        auto new_start = other.end();
        auto new_count = (end() - new_start) / 4096;

        if(new_count == 0) {
            return {};
        }

        return mm_page_list_entry{
            new_start,
            new_count,
            status,
        };
    }

    bool operator==(const mm_page_list_entry& other) const {
        return start == other.start && count == other.count && status == other.status;
    }
};

typedef std::forward_list<mm_page_list_entry, TypedAllocator<mm_page_list_entry>> page_list_t;
//typedef std::forward_list<mm_page_list_entry> page_list_t;
page_list_t mm_physical_page_list;

static void mm_check_integrity() {
    auto prev = mm_physical_page_list.before_begin();

    for(auto it = mm_physical_page_list.begin(); it != mm_physical_page_list.end(); ++it) {
        if (it->count == 0) {
            panic_message("MM: count == 0 region detected!");
        }

        if (prev != mm_physical_page_list.before_begin() && prev->start > it->start) {
            panic_message("MM: unordered bullshit detected!");
        }
    }
}

void* mm_alloc_pages(uint64_t count) {
    mm_check_integrity();

    for(auto it = mm_physical_page_list.begin(); it != mm_physical_page_list.end(); ++it) {
        if(it->status == MM_FREE && it->count >= count) {
            auto start = it->start;
            mm_mark_physical_pages(start, count, MM_ALLOCATED);
            mm_check_integrity();
            return reinterpret_cast<void*>(start);
        }
    }

    mm_check_integrity();
    panic_message("Out of memory in mm_alloc_pages");
}

void mm_mark_physical_pages(uint64_t start, uint64_t count, mm_page_status_t status) {
    mm_check_integrity();

    mm_page_list_entry mark { start, count, status };

    auto prev = mm_physical_page_list.before_begin();
    auto insertion_point = prev;
    bool inserted = false;

    for(auto it = mm_physical_page_list.begin(); it != mm_physical_page_list.end(); ++it) {
        if(it->start > mark.start && insertion_point == mm_physical_page_list.before_begin()) {
            insertion_point = prev;
        }

        if(it->intersects(mark)) {
            auto before = it->part_before(mark);
            auto after = it->part_after(mark);

            mm_physical_page_list.erase_after(prev);

            if(before) {
                if(inserted) {
                    loge("mm", "Something weird is going on!");
                }

                prev = mm_physical_page_list.insert_after(prev, before.value());
                mm_check_integrity();
            }

            if(!inserted) {
                prev = mm_physical_page_list.insert_after(prev, mark);
                inserted = true;
                mm_check_integrity();
            }

            if(after) {
                prev = mm_physical_page_list.insert_after(prev, after.value());
                mm_check_integrity();
            }

            it = prev;
            continue;
        }

        prev = it;
    }

    if(!inserted) {
        if(insertion_point == mm_physical_page_list.before_begin()) {
            insertion_point = prev;
        }

        mm_physical_page_list.insert_after(insertion_point, mark);
        mm_check_integrity();
    }

    auto changed = true;

    while(changed) {
        changed = false;
        auto prev_prev = mm_physical_page_list.before_begin();
        prev = mm_physical_page_list.begin();

        for(auto it = prev; it != mm_physical_page_list.end(); ++it) {
            if(it == prev) {
                // would be great if we could just "it + 1" but no, not a thing ...
                continue;
            }

            if(prev->status == it->status && prev->end() == it->start) {
                mm_page_list_entry merged{ prev->start, prev->count + it->count, prev->status };
                mm_physical_page_list.erase_after(prev);
                mm_physical_page_list.erase_after(prev_prev);
                prev = it = mm_physical_page_list.insert_after(prev_prev, merged);
                mm_check_integrity();
                changed = true;
            }
            else {
                prev_prev = prev;
                prev = it;
            }
        }
    }
}

uint64_t mm_highest_address(void) {
    uint64_t res = 0;

    for(auto it = mm_physical_page_list.begin(); it != mm_physical_page_list.end(); ++it) {
        uint64_t candidate = it->start + (it->count * 4096);
        if(candidate > res) {
            res = candidate;
        }
    }

    return res;
}

void mm_print_regions(mm_page_status_t filter_status) {
    logd("mm", "===== Memory regions dump start =====");
    for(auto it = mm_physical_page_list.begin(); it != mm_physical_page_list.end(); ++it) {
        if(filter_status == MM_INVALID || it->status == filter_status) {
            const char* status;
            switch(it->status) {
                case MM_FREE:
                    status = "free";
                    break;
                case MM_ALLOCATED:
                    status = "allocated";
                    break;
                case MM_RESERVED:
                    status = "reserved";
                    break;
                case MM_UEFI_MAPPING_REQUIRED:
                    status = "uefi-mapping-required";
                    break;

                case MM_UNKNOWN:
                default:
                    status = "unknown";
                    break;
            }

            logd("mm", "%u %s (%u) pages starting at 0x%x", it->count, status, static_cast<uint64_t>(it->status), it->start);
        }
    }
    logd("mm", "===== Memory regions dump end =====");
}

void mm_print_physical_free_regions(void) {
    mm_print_regions(MM_FREE);
}

