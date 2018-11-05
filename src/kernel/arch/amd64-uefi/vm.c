#include "vm.h"
#include "mm.h"
#include "string.h"
#include "fbconsole.h"

#define BASE_TO_TABLE(x) ((vm_table_t*)((uint64_t)x << 12))

#define PML4_INDEX(x) (x >> 39) & 0x1FF
#define PDP_INDEX(x)  (x >> 30) & 0x1FF
#define PD_INDEX(x)   (x >> 21) & 0x1FF
#define PT_INDEX(x)   (x >> 12) & 0x1FF

extern void load_cr3(ptr_t cr3);

void init_vm() {
    vm_table_t* kernel_context = vm_context_new();
    VM_KERNEL_CONTEXT = kernel_context;

    fbconsole_write("Kernel PML4 located at: 0x%x\n", kernel_context);
}

vm_table_t* vm_context_new() {
    vm_table_t* context = (vm_table_t*)mm_alloc_kernel_pages(1);
    memset((void*)context, 0, 4096);

    return context;
}

void vm_context_activate(vm_table_t* context) {
    load_cr3((ptr_t)context);
}

void vm_context_map(vm_table_t* context, ptr_t virtual, ptr_t physical) {
    vm_table_entry_t* pml4_entry = &context->entries[PML4_INDEX(virtual)];

    if(!pml4_entry->present) {
        ptr_t pdp = (ptr_t)mm_alloc_kernel_pages(1);
        memset((void*)pdp, 0, 4096);

        pml4_entry->next_base = pdp >> 12;
        pml4_entry->present   = 1;
        pml4_entry->writeable = 1;
    }

    vm_table_t*       pdp       = BASE_TO_TABLE(pml4_entry->next_base);
    vm_table_entry_t* pdp_entry = &pdp->entries[PDP_INDEX(virtual)];

    if(!pdp_entry->present) {
        ptr_t pd = (ptr_t)mm_alloc_kernel_pages(1);
        memset((void*)pd, 0, 4096);

        pdp_entry->next_base = pd >> 12;
        pdp_entry->present   = 1;
        pdp_entry->writeable = 1;
    }

    vm_table_t*       pd       = BASE_TO_TABLE(pdp_entry->next_base);
    vm_table_entry_t* pd_entry = &pd->entries[PD_INDEX(virtual)];

    if(!pd_entry->present) {
        ptr_t pt = (ptr_t)mm_alloc_kernel_pages(1);
        memset((void*)pt, 0, 4096);

        pd_entry->next_base = pt >> 12;
        pd_entry->present   = 1;
        pd_entry->writeable = 1;
    }

    vm_table_t*       pt       = BASE_TO_TABLE(pd_entry->next_base);
    vm_table_entry_t* pt_entry = &pt->entries[PT_INDEX(virtual)];

    pt_entry->next_base = physical >> 12;
    pt_entry->present   = 1;
    pt_entry->writeable = 1;
}

ptr_t vm_context_get_free_address(vm_table_t *table, bool kernel) {
    int indices[4] = {kernel ? 256 : 0, 0, 0, 0};

    while(1) {
        vm_table_t* pdp = BASE_TO_TABLE(table->entries[indices[0]].next_base);
        vm_table_t* pd  = BASE_TO_TABLE(pdp  ->entries[indices[1]].next_base);
        vm_table_t* pt  = BASE_TO_TABLE(pd   ->entries[indices[2]].next_base);

        if(!vm_table_check_present(table, indices[0]) ||
           !vm_table_check_present(pdp,   indices[1]) ||
           !vm_table_check_present(pd,    indices[2]) ||
           !vm_table_check_present(pt,    indices[3])
        ) {
            break;
        }
        else {
            ++indices[3];

            if(indices[3] == 512) {
                indices[3] = 0;
                ++indices[2];

                if(indices[2] == 512) {
                    indices[2] = 0;
                    ++indices[1];

                    if(indices[1] == 512) {
                        indices[1] = 0;
                        ++indices[0];

                        if(indices[0] == (kernel ? 512 : 256)) {
                            return 0;
                        }
                    }
                }
            }
        }
    }

    ptr_t address = indices[0] > 255 ? 0xFFFF000000000000 : 0;
    address      |= ((uint64_t)indices[0] << 39);
    address      |= ((uint64_t)indices[1] << 30);
    address      |= ((uint64_t)indices[2] << 21);
    address      |= ((uint64_t)indices[3] << 12);

    return address;
}


bool vm_table_check_present(vm_table_t* table, int index) {
    return table->entries[index].present;
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
