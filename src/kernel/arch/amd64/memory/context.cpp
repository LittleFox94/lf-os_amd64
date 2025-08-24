#include <memory/context.h>
#include <mm.h>
#include <vm.h>
#include <log.h>
#include <panic.h>

#include <cstdint>
#include <cstring>
#include <variant>

static const std::uint64_t DirectMappingStart = 0xFFFF800000000000;

template<std::size_t AddressBits, class NextLevel, std::size_t CleanupStart, std::size_t CleanupEnd>
class PageTableBase {
    private:
        template<class T, std::size_t Offset>
        struct _Entry : public vm_table_entry {
            T& operator*() {
                return *(reinterpret_cast<T*>(reinterpret_cast<std::uint64_t>(this) + Offset));
            }

            T* operator->() {
                return *this;
            }
        };

    public:
        struct VirtualEntry;
        struct PhysicalEntry : public _Entry< VirtualEntry,  DirectMappingStart> { };
        struct VirtualEntry  : public _Entry<PhysicalEntry, -DirectMappingStart> {
            void operator=(const vm_table_entry& e) {
                static_assert(sizeof(*this) == sizeof(e));
                std::memcpy(static_cast<void*>(this), static_cast<const void*>(&e), sizeof(e));
            }

            typedef std::variant<NextLevel*, void*> next_t;
            next_t operator()() {
                auto virtual_base = vm_table_entry::next_base << 12;

                if(!vm_table_entry::present) {
                    return next_t(static_cast<void*>(nullptr));
                }

                if(vm_table_entry::huge) {
                    return next_t(reinterpret_cast<void*>(virtual_base));
                }
                else {
                    return next_t(reinterpret_cast<NextLevel*>(virtual_base + DirectMappingStart));
                }
            }
        };

    protected:
        VirtualEntry entries[512];

        static constexpr size_t vaddr_index(uint64_t vaddr) {
            std::size_t shifted_vaddr = vaddr >> AddressBits;
            return shifted_vaddr & 0x1FF;
        }

        template<typename _NextLevel>
        void cleanup_nested_tables() {
            //logd("PageTableBase", "Cleaning nested tables from %u to %u (PageTableBase<%u>)", CleanupStart, CleanupEnd, AddressBits);

            for(size_t i = CleanupStart; i <= CleanupEnd; ++i) {
                auto next_v = entries[i]();
                auto next = std::get_if<_NextLevel*>(&next_v);
                if(next != nullptr) {
                    delete *next;
                }
            }
        }

        template<>
        void cleanup_nested_tables<void>() { }

        void map_direct(uint64_t vaddr, uint64_t paddr, MemoryAccess) {
            std::size_t idx = vaddr_index(vaddr);
            entries[idx].present   = true;
            entries[idx].userspace = true;
            entries[idx].huge      = false;
            entries[idx].global    = false;
            entries[idx].next_base = paddr >> 12;
            entries[idx].nx        = 0; // access != MemoryAccess::Execute;
            entries[idx].writeable = 1; // access == MemoryAccess::Write;
        }

    public:
        void* operator new(std::size_t n) {
            n /= 4096;

            void* ret = mm_alloc_pages(n);
            //logd("PageTableBase", "Allocated new PageTableBase<%u> instances: %u at 0x%x", AddressBits, n, ret);
            return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(ret) + DirectMappingStart);
        }

        void operator delete(void* ptr, std::size_t n) {
            n /= 4096;

            uint64_t addr = reinterpret_cast<uint64_t>(ptr) - DirectMappingStart;
            //logd("PageTableBase", "Freeing PageTableBase<%u> instances: %u at 0x%x", AddressBits, n, addr);

            mm_mark_physical_pages(addr, n, MM_FREE);
        }

        PageTableBase() {
            std::memset(reinterpret_cast<void*>(entries), 0, sizeof(entries));
        }

        ~PageTableBase() {
            cleanup_nested_tables<NextLevel>();
        }

        VirtualEntry& operator[](std::size_t index) {
            return entries[index];
        }

        void map(uint64_t vaddr, uint64_t paddr, MemoryAccess access) {
            std::size_t idx = vaddr_index(vaddr);
            auto next_v = entries[idx]();
            auto next = std::get_if<NextLevel*>(&next_v);
            NextLevel* next_level;

            if(next != 0) {
                next_level = *next;
            }
            else {
                //logd("PageTableBase::map", "Allocating new NextLevel (AddressBits %u) for index %u, current entry 0x%16x", AddressBits, idx, *reinterpret_cast<uint64_t*>(&entries[idx]));
                next_level = new NextLevel();

                entries[idx].present   = true;
                entries[idx].userspace = true;
                entries[idx].huge      = false;
                entries[idx].global    = false;
                entries[idx].nx        = false;
                entries[idx].writeable = true;
                entries[idx].next_base = (reinterpret_cast<uint64_t>(next_level) - DirectMappingStart) >> 12;
            }

            next_level->map(vaddr, paddr, access);
        }
};

class PT   : public PageTableBase<12, void, 0, 0>   {
    public:
        void map(uint64_t vaddr, uint64_t paddr, MemoryAccess access) {
            map_direct(vaddr, paddr, access);
        }
};
class PD   : public PageTableBase<21, PT,  0, 511>  { };
class PDP  : public PageTableBase<30, PD,  0, 511>  { };
class PML4 : public PageTableBase<39, PDP, 0, 255>  {
    public:
        PML4() {
            for(size_t i = 256; i < 512; ++i) {
                auto physicalEntry = &(*this)[i];
                *physicalEntry = VM_KERNEL_CONTEXT->entries[i];
            }
        }

        ~PML4() {
            uint64_t addr = reinterpret_cast<uint64_t>(this) - DirectMappingStart;
            uint64_t current_cr3 = 0;
            asm volatile("mov %%cr3, %0":"=r"(current_cr3));
            if(addr == current_cr3) {
                //logd("PageTableBase", "Handling deletion of the currently active context by activating VM_KERNEL_CONTEXT instead");
                vm_context_activate(VM_KERNEL_CONTEXT);
            }
        }
};

static_assert(sizeof(PT) == 4096);
static_assert(sizeof(PD) == 4096);
static_assert(sizeof(PDP) == 4096);
static_assert(sizeof(PML4) == 4096);

void* MemoryContext::arch_context() {
    if (_arch_cache != 0) {
        PML4* old = reinterpret_cast<PML4*>(_arch_cache);
        delete old;
    }

    auto pml4 = new PML4();
    _arch_cache = reinterpret_cast<void*>(pml4);

    for(auto& mapped_object : _objects) {
        auto mappings = mapped_object.object->getMappings();
        for(auto& mapping : mappings) {
            uint64_t vaddr = mapped_object.vaddr + mapping.vaddr_start;

            for(size_t i = 0; i < mapping.length; i += 4096) {
                pml4->map(vaddr + i, mapping.paddr_start + i, mapping.usage);
            }
        }
    }

    return pml4;
}
