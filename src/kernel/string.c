#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

int strcmp(const char* a, const char* b) {
    while(*a && *b && *a == *b) { a++; b++; }

    return *a - *b;
}

char tolower(const char c) {
    if(c >= 'A' && c <= 'Z') {
        return c - ('A' - 'a');
    }

    return c;
}

int strcasecmp(const char* a, const char* b) {
    while(
        *a && *b &&
        tolower(*a) == tolower(*b)
    ) { a++; b++; }

    return tolower(*a) - tolower(*b);
}

size_t strlen(const char* str) {
    if(!str) return 0;

    size_t i = 0;

    while(*(str + i)) {
        ++i;
    }

    return i;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for(i = 0; i < n && src[i]; ++i) {
        dest[i] = src[i];
    }
    for(; i < n; ++i) {
        dest[i] = 0;
    }

    return dest;
}

size_t wcslen(const wchar_t* str) {
    if(!str) return 0;

    size_t i = 0;

    while(*(str + i)) {
        ++i;
    }

    return i;
}

void memset32(uint32_t* dest, uint32_t c, size_t size) {
    for(size_t i = 0; i < size; ++i) {
        dest[i] = c;
    }
}

void* memset(void* dest, int c, size_t size) {
    for(size_t i = 0; i < size; ++i) {
        *((uint8_t*)dest + i) = c;
    }

    return dest;
}

void* memcpy(void* dest, void const* source, size_t size) {
    uint8_t* dst_8 = (uint8_t*)dest;
    uint8_t* src_8 = (uint8_t*)source;

    for(size_t i = 0; i < size; ++i) {
        dst_8[i] = src_8[i];
    }

    return dest;
}

int memcmp(const void* a, const void* b, size_t n) {
    const uint8_t* _a = (const uint8_t*)a;
    const uint8_t* _b = (const uint8_t*)b;

    size_t i = 0;
    for(i = 0; i < n && _a[i] == _b[i]; ++i) {
    }

    if(i == n) {
        return 0;
    }

    return _a[i] - _b[i];
}

void* memmove(void* dest, const void* src, size_t n) {
    if(src > dest) {
        for(size_t i = 0; i < n; ++i) {
            ((uint8_t*)dest)[i]  = ((uint8_t*)src)[i];
        }
    }
    else {
        for(size_t i = n - 1; i >= 0; --i) {
            ((uint8_t*)dest)[i]  = ((uint8_t*)src)[i];
        }
    }

    return dest;
}

int sputs(char* buffer, int buffer_size, char* string, int length) {
    int i;

    for(i = 0; i < length && i < buffer_size; ++i) {
        buffer[i] = string[i];
    }

    return i;
}

int sputls(char* buffer, int buffer_size, wchar_t* string, int length) {
    int i;

    for(i = 0; i < length && i < buffer_size; ++i) {
        buffer[i] = (char)string[i]; // XXX: we only support ASCII code points in wchar_t
    }

    return i;
}

int sputui(char* buffer, int buffer_size, uint64_t number, int base) {
    const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    int count = 0;

    const int size = 64;
    int i          = size - 1;
    char num_buffer[size];

    do {
        char c          = chars[number % base];
        num_buffer[i--] = c;
        number         /= base;
        count++;
    } while(number && i >= 0 && count < buffer_size);

    sputs(buffer, buffer_size, num_buffer + (size - count), count);
    return count;
}

int sputi(char* buffer, int buffer_size, int64_t number, int base) {
    const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    int count = 0;

    if(number < 0) {
        *buffer++ = '-';
        buffer_size--;
        count++;
        number *= -1;
    }

    const int size = 64;
    int i          = size - 1;
    char num_buffer[size];

    do {
        char c          = chars[number % base];
        num_buffer[i--] = c;
        number         /= base;
        count++;
    } while(number && i >= 0 && count < buffer_size);

    sputs(buffer, buffer_size, num_buffer + (size - count), count);
    return count;
}

int sputbytes(char* buffer, int buffer_size, int64_t number) {
    const char *prefixes = " KMGT";
    static const int div = 1024;

    int prefix = 0;
    int exact  = 1;

    while(number / div >= 1 && prefix < strlen(prefixes)) {
        if((number / div) * div != number) {
            exact = 0;
        }

        number /= div;
        prefix++;
    }

    int len = 0;

    if(!exact) {
        buffer[len++] = '~';
    }

    len += sputi(buffer + len, buffer_size, number, 10);

    if(prefix && len < buffer_size) {
        buffer[len++] = prefixes[prefix];
        buffer[len++] = 'i';
    }

    buffer[len++] = 'B';

    return len;
}

int kvsnprintf(char* buffer, int buffer_size, const char* format, va_list args) {
    int i                = 0;
    char c               = 0;
    bool placeholder     = false;
    char placeholderChar = ' ';
    int minLength        = 0;
    char lengthModifier  = 0; // store length in bytes

    while((c = *format++) && i < buffer_size) {
        if(c == '%') {
            placeholder     = true;
            placeholderChar = ' ';
            minLength       = 0;
            lengthModifier  = 0;
            continue;
        }

        if(placeholder) {
            if((minLength == 0 && c == '0') || c == ' ') {
                placeholderChar = c;
                continue;
            }
            else if(c >= '0' && c <= '9') {
                minLength *= 10;
                minLength += c - '0';
            }
            else if(c == 'l') {
                lengthModifier = 16;
            }
            else {
                char argBuffer[256];
                int  length;

                memset((void*)argBuffer, 0, 256);

                switch(c) {
                    case 'c':
                        argBuffer[0] = va_arg(args, int);
                        length = 1;
                        break;
                    case 'u':
                        length = sputui(argBuffer, 256, va_arg(args, uint64_t), 10);
                        break;
                    case 'd':
                        length = sputi(argBuffer, 256, va_arg(args, int), 10);
                        break;
                    case 'x':
                        length = sputui(argBuffer, 256, va_arg(args, uint64_t), 16);
                        break;
                    case 'B':
                        length = sputbytes(argBuffer, 256, va_arg(args, long));
                        break;
                    case 's':
                        switch(lengthModifier) {
                            case 0:
                                {
                                    char* sarg = va_arg(args, char*);
                                    length = sputs(argBuffer, 256, sarg, strlen(sarg));
                                }
                                break;
                            case 16:
                                {
                                    wchar_t* lsarg = va_arg(args, wchar_t*);
                                    length = sputls(argBuffer, 256, lsarg, wcslen(lsarg));
                                }
                                break;
                        }
                        break;
                    default:
                        length = 1;
                        argBuffer[0] = c;
                        break;
                }

                while(length < minLength && i < buffer_size) {
                    buffer[i++] = placeholderChar;
                    --minLength;
                }

                i += sputs(buffer + i, buffer_size - i, argBuffer, length);

                placeholder = false;
            }
        }
        else {
            buffer[i++] = c;
        }
    }

    buffer[i] = 0;

    return i;
}

int ksnprintf(char* buffer, int buffer_size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int count = kvsnprintf(buffer, buffer_size, format, args);
    va_end(args);

    return count;
}
