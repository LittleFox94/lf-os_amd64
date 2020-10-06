#include <stdint.h>
#include <efi.h>

static EFI_SYSTEM_TABLE* st;
EFI_BOOT_SERVICES* BS;

void init_stdlib(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
    st = system_table;
    BS = st->BootServices;
    st->ConOut->ClearScreen(st->ConOut);
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

size_t wcslen(const CHAR16* s) {
    size_t len = 0;
    while(s[len++]);
    return len;
}

int wcscmp(const CHAR16* s1, const CHAR16* s2) {
    size_t i = 0;
    while(*(s1 + i) && *(s1 + i) == *(s2 + i)) {
        ++i;
    }

    return *(s2 + i) - *(s1 + i);
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

    while(i) {
        buffer[buffer_size - 1 -++len] = chars[i % base];
        i /= base;
    }

    st->ConOut->OutputString(st->ConOut, buffer + (buffer_size - len) - 1);

    return len;
}

size_t wprinti(long long int i, unsigned char base) {
    int neg = i < 0;

    if(neg) {
        i *= -1;
        st->ConOut->OutputString(st->ConOut, L"-");
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
                st->ConOut->OutputString(st->ConOut, L"\r\n");
                len += 2;
            }
            else {
                len += 1;
                CHAR16 buffer[2];
                buffer[0] = c;
                buffer[1]  =0;
                st->ConOut->OutputString(st->ConOut, buffer);
            }
        }
        else {
            switch(c) {
                case '%':
                    st->ConOut->OutputString(st->ConOut, L"\r\n");
                    len += 2;
                    break;
                case 's':
                    CHAR16* s = __builtin_va_arg(args, CHAR16*);
                    st->ConOut->OutputString(st->ConOut, s);
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
