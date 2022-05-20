#include <vm.h>
#include <mm.h>
#include <string.h>
#include <log.h>
#include <slab.h>
#include <bluescreen.h>
#include <tpa.h>
#include <msr.h>

void load_cr3(ptr_t cr3);

static bool vm_direct_mapping_initialized = false;
static tpa_t* page_descriptors = 0;
struct vm_table* VM_KERNEL_CONTEXT;

struct page_descriptor {
    uint32_t flags: 30;
    uint8_t  size:   2; // 0 = 4KiB, 1 = 2MiB, 2 = 1GiB, 3 = panic
    uint32_t refcount;
};

//! A single entry in a paging table
struct vm_table_entry {
    unsigned int present      : 1;
    unsigned int writeable    : 1;
    unsigned int userspace    : 1;
    unsigned int pat0         : 1;
    unsigned int pat1         : 1;
    unsigned int accessed     : 1;
    unsigned int dirty        : 1;
    unsigned int huge         : 1;
    unsigned int global       : 1;
    unsigned int available    : 3;
    unsigned long next_base   : 40;
    unsigned int available2   : 11;
    unsigned int nx           : 1;
}__attribute__((packed));

//! A paging table, when this is a PML4 it may also be called context
struct vm_table {
    struct vm_table_entry entries[512];
}__attribute__((packed));

#define BASE_TO_PHYS(x)          ((void*)(x << 12))
#define BASE_TO_DIRECT_MAPPED(x) ((vm_direct_mapping_initialized ? ALLOCATOR_REGION_DIRECT_MAPPING.start : 0) + BASE_TO_PHYS(x))
#define BASE_TO_TABLE(x)         ((struct vm_table*)BASE_TO_DIRECT_MAPPED(x))

#define PML4_INDEX(x) (((x) >> 39) & 0x1FF)
#define PDP_INDEX(x)  (((x) >> 30) & 0x1FF)
#define PD_INDEX(x)   (((x) >> 21) & 0x1FF)
#define PT_INDEX(x)   (((x) >> 12) & 0x1FF)

void vm_setup_direct_mapping_init(struct vm_table* context) {
    // we have to implement a lot code from below in a simpler way since we cannot assume a lot of things assumed below:
    //  - we do not have the direct mapping of all physical memory yet, so we have to carefully work with virtual and physical addresses
    //  - we do not have malloc() yet
    //  - we do not have any exception handlers in place. Creating a page fault _now_ will reboot the system

    // to make things simple, we just don't call any other function here that relies on the virtual memory management
    // things allowed here:
    //  - fbconsole (debugging output)
    //  - mm        (physical memory management)
    //  - data structure definitions and macros

    ptr_t physicalEndAddress = mm_highest_address();

    if(ALLOCATOR_REGION_DIRECT_MAPPING.start + physicalEndAddress > ALLOCATOR_REGION_DIRECT_MAPPING.end) {
        logw("vm", "More physical memory than direct mapping region. Only %B will be usable", ALLOCATOR_REGION_DIRECT_MAPPING.end - ALLOCATOR_REGION_DIRECT_MAPPING.start);
        physicalEndAddress = ALLOCATOR_REGION_DIRECT_MAPPING.end - ALLOCATOR_REGION_DIRECT_MAPPING.start;
    }

    size_t numPages;
    size_t pageSize;

    if(physicalEndAddress > 1 * GiB) {
        numPages = (physicalEndAddress + (GiB - 1)) / (1 * GiB);
        pageSize = 1 * GiB;
    }
    else {
        numPages = (physicalEndAddress + (MiB - 1)) / (2 * MiB);
        pageSize = 2 * MiB;
    }

    logd("vm", "%B -> %d pages @ %B", physicalEndAddress, numPages, pageSize);

    SlabHeader* scratchpad_allocator = (SlabHeader*)ALLOCATOR_REGION_SCRATCHPAD.start;

    struct vm_table* pdp;
    int16_t last_pml4_idx = -1;

    struct vm_table* pd; // only used for 1MiB pages
    int16_t last_pdp_idx = -1;

    for(ptr_t i = 0; i <= physicalEndAddress;) {
        ptr_t vi = ALLOCATOR_REGION_DIRECT_MAPPING.start + i;

        int16_t pml4_idx = PML4_INDEX(vi);
        int16_t pdp_idx  = PDP_INDEX(vi);

        if(pml4_idx != last_pml4_idx) {
            pdp = (struct vm_table*)slab_alloc(scratchpad_allocator);
            memset((void*)pdp, 0, 0x1000);

            context->entries[pml4_idx] = (struct vm_table_entry){
                .present   = 1,
                .writeable = 1,
                .userspace = 0,
                .next_base = vm_context_get_physical_for_virtual(context, (ptr_t)pdp) >> 12,
            };

            last_pml4_idx = pml4_idx;
        }

        if(pageSize == 1*GiB) {
            pdp->entries[pdp_idx] = (struct vm_table_entry){
                .huge      = 1,
                .present   = 1,
                .writeable = 1,
                .userspace = 0,
                .next_base = i >> 12,
            };

            i += pageSize;
        } else {
            if(pdp_idx != last_pdp_idx) {
                pd = (struct vm_table*)slab_alloc(scratchpad_allocator);
                memset((void*)pd, 0, 0x1000);

                pdp->entries[pdp_idx] = (struct vm_table_entry){
                    .present   = 1,
                    .writeable = 1,
                    .userspace = 0,
                    .next_base = vm_context_get_physical_for_virtual(context, (ptr_t)pd) >> 12,
                };

                last_pdp_idx = pdp_idx;
            }

            int16_t pd_idx = PD_INDEX(vi);
            pd->entries[pd_idx] = (struct vm_table_entry){
                .huge      = 1,
                .present   = 1,
                .writeable = 1,
                .userspace = 0,
                .next_base = i >> 12,
            };

            i += pageSize;
        }
    }

    vm_direct_mapping_initialized = true;
    page_descriptors = (tpa_t*)slab_alloc(scratchpad_allocator);
    memset(page_descriptors, 0, 4*KiB);
}

struct page_descriptor* vm_page_descriptor(ptr_t physical) {
    struct page_descriptor* page = tpa_get(page_descriptors, physical >> 12);

    if(!page) {
        struct page_descriptor new_page = {
            .size     = PageSize4KiB,
            .flags    = PageUsageKernel,
            .refcount = 0,
        };

        tpa_set(page_descriptors, physical >> 12, &new_page);
        page = tpa_get(page_descriptors, physical >> 12);

        if(page == 0) {
            panic_message("page error");
        }
    }

    return page;
}

void vm_set_page_descriptor(ptr_t physical, uint32_t flags, uint8_t size) {
    struct page_descriptor* page = vm_page_descriptor(physical);

    page->flags = flags;
    page->size  = size;

    if(size > 3) {
        panic_message("Invalid page size!");
    }
}

void vm_ref_inc(ptr_t physical) {
    struct page_descriptor* page = vm_page_descriptor(physical);
    ++page->refcount;
}

void vm_ref_dec(ptr_t physical) {
    struct page_descriptor* page = vm_page_descriptor(physical);
    --page->refcount;

    if(!page->refcount) {
        tpa_set(page_descriptors, physical >> 12, 0);
    }
}

void* vm_page_alloc(uint32_t flags, uint8_t size) {
    void* ret;
    switch(size) {
        case PageSize4KiB:
            ret = mm_alloc_pages(1);
            break;
        case PageSize2MiB:
            ret = mm_alloc_pages(2*MiB / 4*KiB);
            break;
        case PageSize1GiB:
            ret = mm_alloc_pages(1*GiB / 4*KiB);
            break;
    }

    // XXX this can recursively call vm_page_alloc
    // \todo describe kernel pages
    // vm_set_page_descriptor((ptr_t)ret, flags, size);

    return ret;
}

void init_vm() {
    vm_setup_direct_mapping_init(VM_KERNEL_CONTEXT);
    logd("vm", "direct mapping set up");

    // initializing page descriptors
    page_descriptors = tpa_new(&kernel_alloc, sizeof(struct page_descriptor), 4080, page_descriptors); // vm_alloc needs 16 bytes
    logd("vm", "page descriptor structure initialized");

    struct vm_table* new_kernel_context = (struct vm_table*)vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_KERNEL_HEAP, 1);
    memcpy(new_kernel_context, VM_KERNEL_CONTEXT, 4*KiB);

    VM_KERNEL_CONTEXT = new_kernel_context;
    logd("vm", "bootstrapped kernel context");

    // allocate all PDPs for the kernel
    // since we copy the kernel context for new processes, every process inherits those PDPs and
    // we don't have to sync every kernel mapping to every process manually
    for(int i = 256; i < 512; ++i) {
        if(!VM_KERNEL_CONTEXT->entries[i].present) {
            void* page = vm_page_alloc(PageUsageKernel|PageUsagePagingStructure, PageSize4KiB);
            memset(page + ALLOCATOR_REGION_DIRECT_MAPPING.start, 0, 4*KiB);
            VM_KERNEL_CONTEXT->entries[i].next_base = (ptr_t)page >> 12;
            VM_KERNEL_CONTEXT->entries[i].present   = 1;
            VM_KERNEL_CONTEXT->entries[i].huge      = 0;
            VM_KERNEL_CONTEXT->entries[i].global    = 1;
        }
    }

    // set up PAT table, especially setting PAT 7 to write combine and PAT 6 to uncachable
    uint64_t pat = read_msr(0x0277);
    pat &= ~(0xFFULL << 56);
    pat |= (0x01ULL << 56);
    pat &= ~(0xFFULL << 48);
    write_msr(0x0277, pat);

    logd("vm", "reserved kernel PML4 entries");
    logw("vm", "Skipping lots of code because not completed");
    return;

    uint16_t pml4_idx = 0;
    uint16_t pdp_idx  = 0;
    uint16_t pd_idx   = 0;
    uint16_t pt_idx   = 0;

    while(pml4_idx < PML4_INDEX(ALLOCATOR_REGION_DIRECT_MAPPING.start)) {
        if(pt_idx == 512) {
            pt_idx = 0;
            ++pd_idx;
        }

        if(pd_idx == 512) {
            pd_idx = pt_idx = 0;
            ++pdp_idx;
        }

        if(pdp_idx == 512) {
            pdp_idx = pd_idx = pt_idx = 0;
            ++pml4_idx;
            continue;
        }

        if(!VM_KERNEL_CONTEXT->entries[pml4_idx].present) {
            ++pml4_idx;
            pdp_idx = pd_idx = pt_idx = 0;
            continue;
        }

        struct vm_table* pdp = BASE_TO_TABLE(VM_KERNEL_CONTEXT->entries[pml4_idx].next_base);

        if(!pdp->entries[pdp_idx].present || pdp->entries[pdp_idx].huge) {
            if(pdp->entries[pdp_idx].huge) {
                vm_set_page_descriptor(pdp->entries[pdp_idx].next_base << 12, PageUsageKernel, PageSize1GiB);
                vm_ref_inc(pdp->entries[pdp_idx].next_base << 12);
            }

            ++pdp_idx;
            pd_idx = pt_idx = 0;
            continue;
        }

        struct vm_table* pd = BASE_TO_TABLE(pdp->entries[pdp_idx].next_base);

        if(!pd->entries[pd_idx].present || pd->entries[pd_idx].huge) {
            if(pd->entries[pd_idx].huge) {
                vm_set_page_descriptor(pd->entries[pd_idx].next_base << 12, PageUsageKernel, PageSize2MiB);
                vm_ref_inc(pd->entries[pd_idx].next_base << 12);
            }

            ++pd_idx;
            pt_idx = 0;
            continue;
        }

        struct vm_table* pt = BASE_TO_TABLE(pd->entries[pd_idx].next_base);

        if(pt->entries[pt_idx].present) {
            vm_set_page_descriptor(pt->entries[pt_idx].next_base << 12, PageUsageKernel, PageSize4KiB);
            vm_ref_inc(pt->entries[pt_idx].next_base << 12);
        }

        ++pt_idx;
    }

    logd("vm", "set up descriptors for early pages");
}

void cleanup_boot_vm() {
    size_t ret = 0;

    // XXX: check higher half mappings if this page is mapped elsewhere
    //      mark physical page as free if not
    //      -> search for ret += foo lines

    for(uint16_t pml4_idx = 0; pml4_idx < 256; ++pml4_idx) {
        if(VM_KERNEL_CONTEXT->entries[pml4_idx].present) {
            struct vm_table* pdp = BASE_TO_TABLE(VM_KERNEL_CONTEXT->entries[pml4_idx].next_base);

            for(uint16_t pdp_idx = 0; pdp_idx < 512; ++pdp_idx) {
                if(pdp->entries[pdp_idx].huge) {
                    ret += 1*GiB;
                }
                else if(pdp->entries[pdp_idx].present) {
                    struct vm_table* pd = BASE_TO_TABLE(pdp->entries[pdp_idx].next_base);

                    for(uint16_t pd_idx = 0; pd_idx < 512; ++pd_idx) {
                        if(pd->entries[pd_idx].huge) {
                            ret += 2*MiB;
                        }
                        else if(pd->entries[pd_idx].present) {
                            struct vm_table* pt = BASE_TO_TABLE(pd->entries[pd_idx].next_base);

                            for(uint16_t pt_idx = 0; pt_idx < 512; ++pt_idx) {
                                if(pt->entries[pt_idx].present) {
                                    pt->entries[pt_idx].present = 0;
                                    ret += 4*KiB;
                                }
                            }
                        }

                        pd->entries[pd_idx].present = 0;
                    }
                }

                pdp->entries[pdp_idx].present = 0;
            }
        }

        VM_KERNEL_CONTEXT->entries[pml4_idx].present = 0;
    }

    logi("vm", "Cleaned %B", ret);
}

struct vm_table* vm_context_new() {
    struct vm_table* context = (struct vm_table*)vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_KERNEL_HEAP, 1);
    memcpy((void*)context, VM_KERNEL_CONTEXT, 4096);

    return context;
}

void vm_context_activate(struct vm_table* context) {
    load_cr3(vm_context_get_physical_for_virtual(VM_KERNEL_CONTEXT, (ptr_t)context));
}

static void vm_ensure_table(struct vm_table* table, uint16_t index) {
    struct vm_table_entry* entry = &table->entries[index];

    if(!entry->present) {
        ptr_t nt = (ptr_t)(ALLOCATOR_REGION_DIRECT_MAPPING.start + vm_page_alloc(PageUsageKernel|PageUsagePagingStructure, PageSize4KiB));
        memset((void*)nt, 0, 4096);

        entry->next_base = (nt - ALLOCATOR_REGION_DIRECT_MAPPING.start) >> 12;
        entry->present   = 1;
        entry->writeable = 1;
        entry->userspace = 1;
    }
}

void vm_context_map(struct vm_table* pml4, ptr_t virtual, ptr_t physical, uint8_t pat) {
    vm_ensure_table(pml4, PML4_INDEX(virtual));

    struct vm_table* pdp = BASE_TO_TABLE(pml4->entries[PML4_INDEX(virtual)].next_base);
    vm_ensure_table(pdp, PDP_INDEX(virtual));

    struct vm_table* pd = BASE_TO_TABLE(pdp->entries[PDP_INDEX(virtual)].next_base);
    vm_ensure_table(pd, PD_INDEX(virtual));

    struct vm_table* pt = BASE_TO_TABLE(pd->entries[PD_INDEX(virtual)].next_base);

    pt->entries[PT_INDEX(virtual)].next_base = physical >> 12;
    pt->entries[PT_INDEX(virtual)].present   = 1;
    pt->entries[PT_INDEX(virtual)].writeable = 1;
    pt->entries[PT_INDEX(virtual)].userspace = 1;

    pt->entries[PT_INDEX(virtual)].pat0 = !!(pat & 1);
    pt->entries[PT_INDEX(virtual)].pat1 = !!(pat & 2);
    pt->entries[PT_INDEX(virtual)].huge = !!(pat & 4); // huge bit is pat2 bit in PT
}

void vm_context_unmap(struct vm_table* context, ptr_t virtual) {
    struct vm_table_entry* pml4_entry = &context->entries[PML4_INDEX(virtual)];

    if(!pml4_entry->present) {
        return;
    }

    struct vm_table*       pdp= BASE_TO_TABLE(pml4_entry->next_base);
    struct vm_table_entry* pdp_entry = &pdp->entries[PDP_INDEX(virtual)];

    if(!pdp_entry->present) {
        return;
    }

    struct vm_table*       pd = BASE_TO_TABLE(pdp_entry->next_base);
    struct vm_table_entry* pd_entry = &pd->entries[PD_INDEX(virtual)];

    if(!pd_entry->present) {
        return;
    }

    struct vm_table*       pt = BASE_TO_TABLE(pd_entry->next_base);
    struct vm_table_entry* pt_entry = &pt->entries[PT_INDEX(virtual)];

    if(!pt_entry->present) {
        return;
    }

    pt_entry->next_base = 0;
    pt_entry->present   = 0;
    pt_entry->writeable = 0;
    pt_entry->userspace = 0;
}

int vm_table_get_free_index1(struct vm_table *table) {
    return vm_table_get_free_index3(table, 0, 512);
}

int vm_table_get_free_index3(struct vm_table *table, int start, int end) {
    for(int i = start; i < end; i++) {
        if(!table->entries[i].present) {
            return i;
        }
    }

    return -1;
}

ptr_t vm_context_get_physical_for_virtual(struct vm_table* context, ptr_t virtual) {
    if(virtual >= ALLOCATOR_REGION_DIRECT_MAPPING.start && virtual <= ALLOCATOR_REGION_DIRECT_MAPPING.end) {
        return virtual - ALLOCATOR_REGION_DIRECT_MAPPING.start;
    }

    if(!context->entries[PML4_INDEX(virtual)].present)
        return 0;

    struct vm_table* pdp = BASE_TO_TABLE(context->entries[PML4_INDEX(virtual)].next_base);

    if(!pdp->entries[PDP_INDEX(virtual)].present)
        return 0;
    else if(pdp->entries[PDP_INDEX(virtual)].huge)
        return (pdp->entries[PDP_INDEX(virtual)].next_base << 30) | (virtual & 0x3FFFFFFF);

    struct vm_table* pd = BASE_TO_TABLE(pdp->entries[PDP_INDEX(virtual)].next_base);

    if(!pd->entries[PD_INDEX(virtual)].present)
        return 0;
    else if(pd->entries[PD_INDEX(virtual)].huge)
        return (pd->entries[PD_INDEX(virtual)].next_base << 21) | (virtual & 0x1FFFFF);

    struct vm_table* pt = BASE_TO_TABLE(pd->entries[PD_INDEX(virtual)].next_base);

    if(!pt->entries[PT_INDEX(virtual)].present)
        return 0;
    else
        return (pt->entries[PT_INDEX(virtual)].next_base << 12) | (virtual & 0xFFF);
}

bool vm_context_page_present(struct vm_table* context, ptr_t virtual) {
    if(!context->entries[PML4_INDEX(virtual)].present)
        return false;

    struct vm_table* pdp = BASE_TO_TABLE(context->entries[PML4_INDEX(virtual)].next_base);

    if(!pdp->entries[PDP_INDEX(virtual)].present)
        return false;
    else if(pdp->entries[PDP_INDEX(virtual)].huge)
        return true;

    struct vm_table* pd = BASE_TO_TABLE(pdp->entries[PDP_INDEX(virtual)].next_base);

    if(!pd->entries[PD_INDEX(virtual)].present)
        return false;
    else if(pd->entries[PD_INDEX(virtual)].huge)
        return true;

    struct vm_table* pt = BASE_TO_TABLE(pd->entries[PD_INDEX(virtual)].next_base);

    if(!pt->entries[PT_INDEX(virtual)].present)
        return false;
    else
        return true;
}

ptr_t vm_context_find_free(struct vm_table* context, region_t region, size_t num) {
    ptr_t current = region.start;

    while(current <= region.end) {
        if(vm_context_page_present(context, current)) {
            current += 4096;
            continue;
        }

        bool abortThisCandidate = false;
        for(size_t i = 0; i < num; ++i) {
            if(vm_context_page_present(context, current + (i * 4096))) {
                current += 4096;
                abortThisCandidate = true;
                break;
            }
        }

        if(abortThisCandidate) {
            continue;
        }

        // if we came to this point, enough space is available starting at $current
        return current;
    }

    return 0;
}

ptr_t vm_context_alloc_pages(struct vm_table* context, region_t region, size_t num) {
    ptr_t vdest = vm_context_find_free(context, region, num);

    if(!vdest) return 0;

    // we do not need continuous physical memory, so allocate each page on it's own
    for(size_t i = 0; i < num; ++i) {
        ptr_t physical = (ptr_t)mm_alloc_pages(1);
        vm_context_map(context, vdest + (i * 4096), physical, 0);
    }

    return vdest;
}

void vm_copy_page(struct vm_table* dst_ctx, ptr_t dst, struct vm_table* src_ctx, ptr_t src) {
    // XXX: make some copy-on-write here
    // XXX: incompatible with non-4k pages!

    if(vm_context_page_present(src_ctx, src)) {
        ptr_t src_phys = vm_context_get_physical_for_virtual(src_ctx, src);
        ptr_t dst_phys;

        if(!vm_context_page_present(dst_ctx, dst)) {
            dst_phys = (ptr_t)mm_alloc_pages(1);
            vm_context_map(dst_ctx, dst, dst_phys, 0); // TODO: get PAT from source page
        } else {
            dst_phys = vm_context_get_physical_for_virtual(dst_ctx, dst);
        }

        if(src_phys != dst_phys) {
            memcpy((void*)(dst_phys + ALLOCATOR_REGION_DIRECT_MAPPING.start), (void*)(src_phys + ALLOCATOR_REGION_DIRECT_MAPPING.start), 0x1000);
        } else {
            panic_message("vm_copy_page with same src and dest physical mapping!");
        }
    }
}

void vm_copy_range(struct vm_table* dst, struct vm_table* src, ptr_t addr, size_t size) {
    uint16_t  pml4_l = 0,     pdp_l = 0,      pd_l = 0;
    struct vm_table      *src_pdp   = 0, *src_pd   = 0, *src_pt   = 0;
    struct vm_table      *dst_pdp   = 0, *dst_pd   = 0, *dst_pt   = 0;

    for(ptr_t i = addr; i < addr + size; ) {
        uint16_t pml4_i = PML4_INDEX(i);
        uint16_t pdp_i  = PDP_INDEX(i);
        uint16_t pd_i   = PD_INDEX(i);
        uint16_t pt_i   = PT_INDEX(i);

        if(!src_pdp || pml4_i != pml4_l) {
            vm_ensure_table(dst, pml4_i);
            src_pdp = BASE_TO_TABLE(src->entries[pml4_i].next_base);
            dst_pdp = BASE_TO_TABLE(dst->entries[pml4_i].next_base);
            pml4_l = pml4_i;
        }

        if(!src_pdp->entries[pdp_i].present) {
            i += 1*GiB;
            continue;
        }

        // 1 GiB pages
        if(src_pdp->entries[pdp_i].huge) {
            if(dst_pdp->entries[pdp_i].present) {
                // we would have to free downwards
                panic_message("vm_copy_range/pdp: huge src and dst present is not yet implemented!");
            }

            if((i % (1*GiB)) != 0) {
                panic_message("vm_copy_range/pdp: unaligned huge page address!");
            }

            dst_pdp->entries[pdp_i]           = src_pdp->entries[pdp_i];
            dst_pdp->entries[pdp_i].next_base = (uint64_t)(mm_alloc_pages(1*GiB/4*KiB)) >> 12;
            memcpy(BASE_TO_DIRECT_MAPPED(dst_pdp->entries[pdp_i].next_base), BASE_TO_DIRECT_MAPPED(src_pdp->entries[pdp_i].next_base), 1*GiB);

            i += 1*GiB;
            continue;
        }

        if(!src_pd || pdp_i != pdp_l) {
            vm_ensure_table(dst_pdp, pdp_i);
            src_pd = BASE_TO_TABLE(src_pdp->entries[pdp_i].next_base);
            dst_pd = BASE_TO_TABLE(dst_pdp->entries[pdp_i].next_base);
            pdp_l = pdp_i;
        }

        if(!src_pd->entries[pd_i].present) {
            i += 2*MiB;
            continue;
        }

        // 2 MiB pages
        if(src_pd->entries[pd_i].huge) {
            if(dst_pd->entries[pd_i].present) {
                // we would have to free downwards
                panic_message("vm_copy_range/pd: huge src and dst present is not yet implemented!");
            }

            if((i % (2*MiB)) != 0) {
                panic_message("vm_copy_range/pd: unaligned huge page address!");
            }

            dst_pd->entries[pd_i]           = src_pd->entries[pd_i];
            dst_pd->entries[pd_i].next_base = (uint64_t)(mm_alloc_pages(2*MiB/4*KiB)) >> 12;
            memcpy(BASE_TO_DIRECT_MAPPED(dst_pd->entries[pd_i].next_base), BASE_TO_DIRECT_MAPPED(src_pd->entries[pd_i].next_base), 2*MiB);

            i += 2*MiB;
            continue;
        }

        if(!src_pt || pd_i != pd_l) {
            vm_ensure_table(dst_pd, pd_i);
            src_pt = BASE_TO_TABLE(src_pd->entries[pd_i].next_base);
            dst_pt = BASE_TO_TABLE(dst_pd->entries[pd_i].next_base);
            pd_l = pd_i;
        }

        if(src_pt->entries[pt_i].present) {
            // 4 KiB pages
            if(dst_pt->entries[pt_i].present) {
                // TODO: ref counter for physical pages, mark as free
                logw("vm", "vm_copy_range: unmapping page without marking it as free: %x", dst_pt->entries[pt_i].next_base << 12);
            }

            if((i % (4*KiB)) != 0) {
                panic_message("vm_copy_range/pt: unaligned page address!");
            }

            dst_pt->entries[pt_i]           = src_pt->entries[pt_i];
            dst_pt->entries[pt_i].next_base = (uint64_t)mm_alloc_pages(1) >> 12;
            memcpy(BASE_TO_DIRECT_MAPPED(dst_pt->entries[pt_i].next_base), BASE_TO_DIRECT_MAPPED(src_pt->entries[pt_i].next_base), 4*KiB);
        }

        i += 4*KiB;
    }
}

void* vm_alloc(size_t size) {
    size_t pages = (size + 4095 /* for rounding */ + 16 /* overhead */) / 4096;

    void* ptr                    = (uint64_t*)vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_KERNEL_HEAP, pages);
    *(uint64_t*)ptr              = size;
    *(uint64_t*)(ptr + size + 8) = ~size;

    return (void*)ptr + 8;
}

void vm_free(void* ptr) {
    //! \todo clear and unmap pages
    uint64_t size       = *(uint64_t*)(ptr - 8);
    uint64_t validation = *(uint64_t*)(ptr + size);

    if(size != ~validation) {
        panic_message("VM corruption detected!");
    }

    uint64_t pages = (size + 4095) / 4096;

    for(uint64_t i = 0; i < pages; ++i) {
        ptr_t vir = ((ptr_t)ptr - 8) + (i * 0x1000);
        ptr_t phy = vm_context_get_physical_for_virtual(VM_KERNEL_CONTEXT, vir);
        vm_context_unmap(VM_KERNEL_CONTEXT, vir);
        mm_mark_physical_pages(phy, 1, MM_FREE);
    }
}

static void* kernel_alloc_fn(allocator_t* alloc, size_t size) {
    alloc->tag += size;
    return vm_alloc(size);
}

static void kernel_dealloc_fn(allocator_t* alloc, void* ptr) {
    size_t size = *((size_t*)ptr-1);
    alloc->tag -= size;
    vm_free(ptr);
}

allocator_t kernel_alloc = (allocator_t){
    .alloc   = kernel_alloc_fn,
    .dealloc = kernel_dealloc_fn,
    .tag     = 0,
};

struct vm_table* vm_current_context() {
    struct vm_table* current;
    asm("mov %%cr3, %0":"=r"(current));

    if(vm_direct_mapping_initialized) {
        return (void*)current + ALLOCATOR_REGION_DIRECT_MAPPING.start;
    }
    else {
        return current;
    }
}

ptr_t vm_map_hardware(ptr_t hw, size_t len) {
    struct vm_table* context = vm_current_context();

    size_t pages = (len + 4095) / 4096;
    ptr_t  dest  = vm_context_find_free(context, ALLOCATOR_REGION_USER_HARDWARE, pages);

    for(size_t page = 0; page < pages; ++page) {
        vm_context_map(context, dest + (page * 4096), hw + (page * 4096), 0x07);
    }

    return dest;
}
