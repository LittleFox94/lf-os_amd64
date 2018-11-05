#include "sc.h"
#include "msr.h"
#include "fbconsole.h"
#include "string.h"
#include "mm.h"

#define GDT_ACCESSED   0x01
#define GDT_RW         0x02
#define GDT_CONFORMING 0x04
#define GDT_EXECUTE    0x08
#define GDT_SYSTEM     0x10
#define GDT_RING1      0x20
#define GDT_RING2      0x40
#define GDT_RING3      0x60
#define GDT_PRESENT    0x80

typedef struct {
    uint16_t baseLow;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  flags;
    uint16_t baseMid;
    uint32_t baseHigh;
    uint32_t _reserved;
}__attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t   limitLow;
    uint16_t   baseLow;
    uint8_t    baseMid;
    uint8_t    type;
    uint8_t    size;
    uint8_t    baseHight;
}__attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
}__attribute__((packed)) table_pointer;

idt_entry_t _idt[256];
gdt_entry_t _gdt[5];

void _setup_idt();
void _setup_gdt();
extern void _syscall_handler();
extern void reload_cs();

void init_sc() {
    _setup_idt();
    _setup_gdt();

    uint64_t efer = read_msr(0xC0000080);
    efer |= 1;
    write_msr(0xC0000080, efer);

    write_msr(0xC0000081, 0x00000000);
    write_msr(0xC0000082, (ptr_t)_syscall_handler);
}

void _setup_idt() {
    memset((uint8_t*)_idt, 0, sizeof(_idt));

    table_pointer idtp = {
        .limit = sizeof(_idt) - 1,
        .base  = (ptr_t)_idt,
    };
    asm("lidt %0"::"m"(idtp));
}

void _setup_gdt() {
    memset((uint8_t*)_gdt, 0, sizeof(_gdt));

    _gdt[1].type = GDT_SYSTEM | GDT_PRESENT | GDT_RW | GDT_EXECUTE;
    _gdt[1].size = 0xa0;

    _gdt[2].type = GDT_SYSTEM | GDT_PRESENT | GDT_RW;
    _gdt[2].size = 0xa0;

    _gdt[3].type = GDT_SYSTEM | GDT_PRESENT | GDT_RW | GDT_EXECUTE | GDT_RING3;
    _gdt[3].size = 0xa0;

    _gdt[4].type = GDT_SYSTEM | GDT_PRESENT | GDT_RW | GDT_EXECUTE | GDT_RING3;
    _gdt[4].size = 0xa0;

    table_pointer gdtp = {
        .limit = sizeof(_gdt) -1,
        .base  = mm_get_mapping((ptr_t)_gdt),
    };
    asm("lgdt %0"::"m"(gdtp));

    reload_cs();
}

void syscall_handler() {
    fbconsole_write("[SC] syscall received!\n");
}
