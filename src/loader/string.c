#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <efi.h>

static bool uart_out = false;
static EFI_SYSTEM_TABLE* st;
EFI_BOOT_SERVICES* BS;

static void outb(uint16_t port, uint8_t data) {
    asm volatile("outb %0, %1"::"a"(data), "d"(port));
}

static uint8_t inb(uint16_t port) {
    uint8_t data = 0;
    asm volatile("inb %1, %0":"=a"(data):"d"(port));
    return data;
}

static int is_transmit_empty() {
   return inb(0x3F8 + 5) & 0x20;
}

static void uart_write(char* msg, size_t len) {
    for(size_t i = 0; i < len; ++i) {
        if(msg[i] == '\n') {
            while(!is_transmit_empty()) { }
            outb(0x3F8, '\r');
        }

        while(!is_transmit_empty()) { }
        outb(0x3F8, msg[i]);
    }
}

static void conwrite(CHAR16* s) {
    st->ConOut->OutputString(st->ConOut, s);

    size_t len = wcslen(s);
    char* msg  = malloc(len + 1);
    len = wcstombs(msg, s, len);

    if(len > 1 && len != (size_t)-1) {
        if(uart_out) {
            uart_write(msg, len - 1);
        }
    }

    free(msg);
}

void* memset(void* s, int c, size_t n) {
    for(size_t i = 0; i < n; ++i) {
        *((char*)s + i) = c;
    }

    return s;
}

void* memcpy(void* dest, const void* src, size_t n) {
    for(size_t i = 0; i < n; ++i) {
        *((char*)dest + i) = *((char*)src + i);
    }

    return dest;
}

int memcmp(const void* a, const void* b, size_t n) {
    for(size_t i = 0; i < n; ++i) {
        int diff = ((const char*)a)[i] - ((const char*)b)[i];

        if(diff) {
            return diff;
        }
    }

    return 0;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while(s[len++]);
    return len - 1;
}

size_t strcpy(char* d, const char* s) {
    size_t len = 0;
    while(s[len]) {
        d[len] = s[len];
        ++len;
    }
    return len;
}

size_t wcslen(const CHAR16* s) {
    size_t len = 0;
    while(s[len++]);
    return len - 1;
}

int wcscpy(CHAR16* d, const CHAR16* s) {
    size_t len = 0;
    while(s[len]) {
        d[len] = s[len];
        ++len;
    }
    return len;
}

int wcscmp(const CHAR16* s1, const CHAR16* s2) {
    size_t i = 0;
    while(*(s1 + i) && *(s1 + i) == *(s2 + i)) {
        ++i;
    }

    return *(s2 + i) - *(s1 + i);
}

CHAR16 towlower(CHAR16 c) {
    if(c >= 'a' && c <= 'z') {
        return c - ('a' - 'A');
    }

    return c;
}

int wcscasecmp(const CHAR16* s1, const CHAR16* s2) {
    size_t i = 0;
    while(s1[i] && s2[i]) {
        CHAR16 a = s1[i];
        CHAR16 b = s2[i];

        ++i;

        if(
            (a != b) &&
            (towlower(a) != towlower(b))
        ) {
            break;
        }
    }

    return *(s2 + i) - *(s1 + i);
}


size_t wcstombs(char* dest, const CHAR16* src, size_t n) {
    size_t len = wcslen(src);
    size_t i;
    for(i = 0; i < len && i < n; ++i) {
        CHAR16 c = src[i];

        if(c > 127) {
            return (size_t)-1;
        }

        if(dest) {
            dest[i] = c;
        }
    }

    if(i < n - 1) {
        dest[i++] = 0;
    }

    return i;
}

void* malloc(size_t size) {
    void* mem;
    if(BS->AllocatePool(EfiLoaderData, size + sizeof(size_t), &mem) == EFI_SUCCESS) {
        *(size_t*)mem = size;
        return mem + sizeof(size_t);
    }
    else {
        return 0;
    }
}

void  free(void* ptr) {
    BS->FreePool(ptr - sizeof(size_t));
}

void* realloc(void* ptr, size_t size) {
    if(!ptr) {
        return malloc(size);
    }

    size_t orig = *(size_t*)(ptr - sizeof(size_t));
    void* new = malloc(size);
    memcpy(new, ptr, orig);
    free(ptr);
    return new;
}

size_t wprintui(unsigned long long int i, unsigned char base) {
    const CHAR16* chars = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    const size_t buffer_size = 65;
    CHAR16 buffer[buffer_size];
    buffer[buffer_size - 1] = 0;

    size_t len = 0;

    do {
        buffer[buffer_size - 1 -++len] = chars[i % base];
        i /= base;
    } while(i);

    conwrite(buffer + (buffer_size - len) - 1);

    return len;
}

size_t wprinti(long long int i, unsigned char base) {
    int neg = i < 0;

    if(neg) {
        i *= -1;
        conwrite(L"-");
    }

    return wprintui(i, base) + (neg ? 1 : 0);
}

int wprintf(const CHAR16* fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    CHAR16 c;
    int placeholder  = 0;
    int long_counter = 0;
    size_t i   = 0;
    size_t len = 0;
    while((c = *(fmt + i))) {
        ++i;

        if(!placeholder) {
            if(c == '%') {
                placeholder = 1;
            }
            else if(c == '\n') {
                conwrite(L"\r\n");
                len += 2;
            }
            else {
                len += 1;
                CHAR16 buffer[2];
                buffer[0] = c;
                buffer[1]  =0;
                conwrite(buffer);
            }
        }
        else {
            switch(c) {
                case '%':
                    conwrite(L"%");
                    ++len;
                    break;
                case 'b':
                    CHAR16* b = __builtin_va_arg(args, int) ? L"true" : L"false";
                    conwrite(b);
                    len += wcslen(b);
                    break;
                case 's':
                    CHAR16* s = __builtin_va_arg(args, CHAR16*);
                    conwrite(s);
                    len += wcslen(s);
                    break;
                case 'u':
                    unsigned long long int u;

                    if(long_counter == 2) {
                        u = __builtin_va_arg(args, unsigned long long int);
                    }
                    else {
                        u = __builtin_va_arg(args, unsigned int);
                    }

                    len += wprintui(u, 10);
                    break;
                case 'd':
                    long long int d;

                    if(long_counter == 2) {
                        d = __builtin_va_arg(args, long long int);
                    }
                    else {
                        d = __builtin_va_arg(args, int);
                    }

                    len += wprinti(d, 10);
                    break;
                case 'x':
                    long long int x;

                    if(long_counter == 2) {
                        x = __builtin_va_arg(args, long long int);
                    }
                    else {
                        x = __builtin_va_arg(args, int);
                    }

                    len += wprintui(x, 16);
                    break;
                case 'l':
                    ++long_counter;
                    continue;
            }

            placeholder = 0;
        }
    }

    __builtin_va_end(args);
    return len;
}

void init_stdlib(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
    st = system_table;
    BS = st->BootServices;
    st->ConOut->ClearScreen(st->ConOut);

    if(wcscmp(st->FirmwareVendor, L"EDK II") != 0) {
        uart_out = true;

        // stolen from osdev
        outb(0x3F8 + 1, 0x00);    // Disable all interrupts
        outb(0x3F8 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
        outb(0x3F8 + 0, 0x0c);    // Set divisor to 12 (lo byte) 9600 baud
        outb(0x3F8 + 1, 0x00);    //                   (hi byte)
        outb(0x3F8 + 3, 0x03);    // 8 bits, no parity, one stop bit

        wprintf(L"UART output enabled\n");
    }
}
