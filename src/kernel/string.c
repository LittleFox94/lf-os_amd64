#include "string.h"
#include "stdbool.h"
#include "stdint.h"

int strcmp(const char* a, const char* b) {
    while(*a && *b && *a == *b) { a++; b++; }

    return *a - *b;
}

size_t strlen(const char* str) {
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

void memset(uint8_t* dest, uint8_t c, size_t size) {
    for(size_t i = 0; i < size; ++i) {
        dest[i] = c;
    }
}

void* memcpy(void* dest, void const* source, size_t size) {
    uint8_t* dst_8 = (uint8_t*)dest;
    uint8_t* src_8 = (uint8_t*)source;

    for(size_t i = 0; i < size; ++i) {
        dst_8[i] = src_8[i];
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

    while((c = *format++) && i < buffer_size) {
        if(c == '%') {
            placeholder     = true;
            placeholderChar = ' ';
            minLength       = 0;
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
                        {
                            char* arg = va_arg(args, char*);
                            length = sputs(argBuffer, 256, arg, strlen(arg));
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
    int count = kvsnprintf(buffer, 512, format, args);
    va_end(args);

    return count;
}
