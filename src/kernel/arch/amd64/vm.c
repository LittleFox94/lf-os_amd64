#include "vm.h"
#include "mm.h"
#include "string.h"
#include "fbconsole.h"
#include "slab.h"
#include "bluescreen.h"

static bool vm_direct_mapping_initialized = false;

#define BASE_TO_TABLE(x) ((vm_table_t*)((uint64_t)(vm_direct_mapping_initialized ? ALLOCATOR_REGION_DIRECT_MAPPING.start : 0) + ((uint64_t)x << 12)))

#define PML4_INDEX(x) (((x) >> 39) & 0x1FF)
#define PDP_INDEX(x)  (((x) >> 30) & 0x1FF)
#define PD_INDEX(x)   (((x) >> 21) & 0x1FF)
#define PT_INDEX(x)   (((x) >> 12) & 0x1FF)

extern void load_cr3(ptr_t cr3);

void vm_setup_direct_mapping_init(vm_table_t* context) {
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
    fbconsole_write(" (%B", physicalEndAddress);

    if(ALLOCATOR_REGION_DIRECT_MAPPING.start + physicalEndAddress > ALLOCATOR_REGION_DIRECT_MAPPING.end) {
        fbconsole_write(" -> %B", ALLOCATOR_REGION_DIRECT_MAPPING.end - ALLOCATOR_REGION_DIRECT_MAPPING.start);
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

    fbconsole_write(", %d @ %B)", numPages, pageSize);

    SlabHeader* scratchpad_allocator = (SlabHeader*)ALLOCATOR_REGION_SCRATCHPAD.start;

    vm_table_t* pdp;
    int16_t last_pml4_idx = -1;

    vm_table_t* pd; // only used for 1MiB pages
    int16_t last_pdp_idx = -1;

    for(ptr_t i = 0; i <= physicalEndAddress;) {
        ptr_t vi = ALLOCATOR_REGION_DIRECT_MAPPING.start + i;

        int16_t pml4_idx = PML4_INDEX(vi);
        int16_t pdp_idx  = PDP_INDEX(vi);

        if(pml4_idx != last_pml4_idx) {
            pdp = (vm_table_t*)slab_alloc(scratchpad_allocator);
            memset((void*)pdp, 0, 0x1000);

            context->entries[pml4_idx] = (vm_table_entry_t){
                .present   = 1,
                .writeable = 1,
                .userspace = 0,
                .next_base = vm_context_get_physical_for_virtual(context, (ptr_t)pdp) >> 12,
            };

            last_pml4_idx = pml4_idx;
        }

        if(pageSize == 1*GiB) {
            pdp->entries[pdp_idx] = (vm_table_entry_t){
                .huge      = 1,
                .present   = 1,
                .writeable = 1,
                .userspace = 0,
                .next_base = i >> 12,
            };

            i += pageSize;
        } else {
            if(pdp_idx != last_pdp_idx) {
                pd = (vm_table_t*)slab_alloc(scratchpad_allocator);
                memset((void*)pd, 0, 0x1000);

                pdp->entries[pdp_idx] = (vm_table_entry_t){
                    .present   = 1,
                    .writeable = 1,
                    .userspace = 0,
                    .next_base = vm_context_get_physical_for_virtual(context, (ptr_t)pd) >> 12,
                };

                last_pdp_idx = pdp_idx;
            }

            int16_t pd_idx = PD_INDEX(vi);
            pd->entries[pd_idx] = (vm_table_entry_t){
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
}

void init_vm() {
    vm_setup_direct_mapping_init(VM_KERNEL_CONTEXT);

    vm_table_t* new_kernel_context = (vm_table_t*)vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_KERNEL_HEAP, 1);
    memcpy(new_kernel_context, VM_KERNEL_CONTEXT, 4*KiB);

    VM_KERNEL_CONTEXT = new_kernel_context;
}

void cleanup_boot_vm() {
    size_t ret = 0;

    // XXX: check higher half mappings if this page is mapped elsewhere
    //      mark physical page as free if not
    //      -> search for ret += foo lines

    for(uint16_t pml4_idx = 0; pml4_idx < 256; ++pml4_idx) {
        if(VM_KERNEL_CONTEXT->entries[pml4_idx].present) {
            vm_table_t* pdp = BASE_TO_TABLE(VM_KERNEL_CONTEXT->entries[pml4_idx].next_base);

            for(uint16_t pdp_idx = 0; pdp_idx < 512; ++pdp_idx) {
                if(pdp->entries[pdp_idx].huge) {
                    ret += 1*GiB;
                }
                else if(pdp->entries[pdp_idx].present) {
                    vm_table_t* pd = BASE_TO_TABLE(pdp->entries[pdp_idx].next_base);

                    for(uint16_t pd_idx = 0; pd_idx < 512; ++pd_idx) {
                        if(pd->entries[pd_idx].huge) {
                            ret += 2*MiB;
                        }
                        else if(pd->entries[pd_idx].present) {
                            vm_table_t* pt = BASE_TO_TABLE(pd->entries[pd_idx].next_base);

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

    fbconsole_write(" -> %B freed", ret);
}

void vm_setup_direct_mapping(vm_table_t* context) {
    uint16_t pml4_idx_start = PML4_INDEX(ALLOCATOR_REGION_DIRECT_MAPPING.start);
    uint16_t pml4_idx_end   = PML4_INDEX(ALLOCATOR_REGION_DIRECT_MAPPING.end);

    for(uint16_t i = pml4_idx_start; i <= pml4_idx_end; ++i) {
        context->entries[i] = VM_KERNEL_CONTEXT->entries[i];
    }
}

vm_table_t* vm_context_new() {
    vm_table_t* context = (vm_table_t*)vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_KERNEL_HEAP, 1);
    memcpy((void*)context, VM_KERNEL_CONTEXT, 4096);

    return context;
}

void vm_context_activate(vm_table_t* context) {
    load_cr3(vm_context_get_physical_for_virtual(VM_KERNEL_CONTEXT, (ptr_t)context));
}

void vm_context_map(vm_table_t* context, ptr_t virtual, ptr_t physical) {
    vm_table_entry_t* pml4_entry = &context->entries[PML4_INDEX(virtual)];

    if(!pml4_entry->present) {
        ptr_t pdp = (ptr_t)(ALLOCATOR_REGION_DIRECT_MAPPING.start + mm_alloc_pages(1));
        memset((void*)pdp, 0, 4096);

        pml4_entry->next_base = (pdp - ALLOCATOR_REGION_DIRECT_MAPPING.start) >> 12;
        pml4_entry->present   = 1;
        pml4_entry->writeable = 1;
        pml4_entry->userspace = 1;
    }

    vm_table_t*       pdp       = BASE_TO_TABLE(pml4_entry->next_base);
    vm_table_entry_t* pdp_entry = &pdp->entries[PDP_INDEX(virtual)];

    if(!pdp_entry->present) {
        ptr_t pd = (ptr_t)(ALLOCATOR_REGION_DIRECT_MAPPING.start + mm_alloc_pages(1));
        memset((void*)pd, 0, 4096);

        pdp_entry->next_base = (pd - ALLOCATOR_REGION_DIRECT_MAPPING.start) >> 12;
        pdp_entry->present   = 1;
        pdp_entry->writeable = 1;
        pdp_entry->userspace = 1;
    }

    vm_table_t*       pd       = BASE_TO_TABLE(pdp_entry->next_base);
    vm_table_entry_t* pd_entry = &pd->entries[PD_INDEX(virtual)];

    if(!pd_entry->present) {
        ptr_t pt = (ptr_t)(ALLOCATOR_REGION_DIRECT_MAPPING.start + mm_alloc_pages(1));
        memset((void*)pt, 0, 4096);

        pd_entry->next_base = (pt - ALLOCATOR_REGION_DIRECT_MAPPING.start) >> 12;
        pd_entry->present   = 1;
        pd_entry->writeable = 1;
        pd_entry->userspace = 1;
    }

    vm_table_t*       pt       = BASE_TO_TABLE(pd_entry->next_base);
    vm_table_entry_t* pt_entry = &pt->entries[PT_INDEX(virtual)];

    pt_entry->next_base = physical >> 12;
    pt_entry->present   = 1;
    pt_entry->writeable = 1;
    pt_entry->userspace = 1;
}

int vm_table_get_free_index1(vm_table_t *table) {
    return vm_table_get_free_index3(table, 0, 512);
}

int vm_table_get_free_index3(vm_table_t *table, int start, int end) {
    for(int i = start; i < end; i++) {
        if(!table->entries[i].present) {
            return i;
        }
    }

    return -1;
}

ptr_t vm_context_get_physical_for_virtual(vm_table_t* context, ptr_t virtual) {
    uint16_t pml4_index = PML4_INDEX(virtual);
    uint16_t pdp_index  = PDP_INDEX(virtual);
    uint16_t pd_index   = PD_INDEX(virtual);
    uint16_t pt_index   = PT_INDEX(virtual);

    if(context->entries[pml4_index].present) {
        vm_table_t* pdp = BASE_TO_TABLE(context->entries[pml4_index].next_base);

        if(pdp->entries[pdp_index].present) {
            // check if last level
            if(pdp->entries[pdp_index].huge) {
                return (pdp->entries[pdp_index].next_base << 30) | (virtual & 0x3FFFFFFF);
            }
            else {
                vm_table_t* pd = BASE_TO_TABLE(pdp->entries[pdp_index].next_base);

                if(pd->entries[pd_index].present) {
                    // check if last level
                    if(pd->entries[pd_index].huge) {
                        return (pd->entries[pd_index].next_base << 21) | (virtual & 0x1FFFFF);
                    }
                    else {
                        vm_table_t* pt = BASE_TO_TABLE(pd->entries[pd_index].next_base);

                        if(pt->entries[pt_index].present) {
                            return (pt->entries[pt_index].next_base << 12) | (virtual & 0xFFF);
                        }
                    }
                }
            }
        }
    }

    // nope
    return 0;
}

bool vm_context_page_present(vm_table_t* context, uint16_t pml4_index, uint16_t pdp_index, uint16_t pd_index, uint16_t pt_index) {
    if(context->entries[pml4_index].present) {
        vm_table_t* pdp = BASE_TO_TABLE(context->entries[pml4_index].next_base);

        if(pdp->entries[pdp_index].present) {
            // check if last level
            if(pdp->entries[pdp_index].huge) {
                return true;
            }
            else {
                vm_table_t* pd = BASE_TO_TABLE(pdp->entries[pdp_index].next_base);

                if(pd->entries[pd_index].present) {
                    // check if last level
                    if(pd->entries[pd_index].huge) {
                        return true;
                    }
                    else {
                        vm_table_t* pt = BASE_TO_TABLE(pd->entries[pd_index].next_base);

                        if(pt->entries[pt_index].present) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

ptr_t vm_context_alloc_pages(vm_table_t* context, region_t region, size_t num) {
    ptr_t current = region.start;
    ptr_t found   = 0;

    while(!found && current <= region.end) {
        if(vm_context_page_present(context, PML4_INDEX(current), PDP_INDEX(current), PD_INDEX(current), PT_INDEX(current))) {
            current += 4096;
            continue;
        }

        bool abortThisCandidate = false;
        for(size_t i = 0; i < num; ++i) {
            if(vm_context_page_present(context, PML4_INDEX(current), PDP_INDEX(current), PD_INDEX(current), PT_INDEX(current))) {
                current += 4096;
                abortThisCandidate = true;
                break;
            }
        }

        if(abortThisCandidate) {
            continue;
        }

        // if we came to this point, enough space is available starting at $current
        found = current;
    }

    // we do not need continuous physical memory, so allocate each page on it's own
    for(size_t i = 0; i < num; ++i) {
        ptr_t physical = (ptr_t)mm_alloc_pages(1);
        vm_context_map(context, found + (i * 4096), physical);
    }

    return found;
}

void vm_copy_page(vm_table_t* dst_ctx, ptr_t dst, vm_table_t* src_ctx, ptr_t src) {
    // XXX: make some copy-on-write here
    // XXX: incompatible with non-4k pages!

    if(vm_context_page_present(src_ctx, PML4_INDEX(src), PDP_INDEX(src), PD_INDEX(src), PT_INDEX(src))) {
        ptr_t src_phys = vm_context_get_physical_for_virtual(src_ctx, src);
        ptr_t dst_phys;

        if(!vm_context_page_present(dst_ctx, PML4_INDEX(dst), PDP_INDEX(dst), PD_INDEX(dst), PT_INDEX(dst))) {
            dst_phys = (ptr_t)mm_alloc_pages(1);
            vm_context_map(dst_ctx, dst, dst_phys);
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
