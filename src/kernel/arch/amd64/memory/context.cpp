#include <memory/context.h>
#include <vm.h>

void* MemoryContext::arch_context() {
    return VM_KERNEL_CONTEXT;
}
