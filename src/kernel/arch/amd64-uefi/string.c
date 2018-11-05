#include "string.h"
#include "stdbool.h"
#include "stdint.h"

size_t strlen(char* str) {
    size_t i = 0;

    while(*++str) {
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

void* memcpy(void* dest, void* source, size_t size) {
    uint8_t* dst_8 = (uint8_t*)dest;
    uint8_t* src_8 = (uint8_t*)source;

    for(int i = 0; i < size; ++i) {
        dst_8[i] = src_8[i];
    }

    return dest;
}

int sputs(char* buffer, int buffer_size, char* string, int length) {
    int i;
    for(i = 0; i < length && i < buffer_size; i++) {
        buffer[i] = string[i];
    }

    return i;
}

int sputui(char* buffer, int buffer_size, uint64_t number, int base) {
    char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    int count = 0;

    if(number < 0) {
        buffer[0] = '-';
        buffer++;

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

int sputi(char* buffer, int buffer_size, int64_t number, int base) {
    char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

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
    char *prefixes       = " KMGT";
    static const int div = 1024;

    int prefix = 0;

    while(number / div > 1 && prefix < strlen(prefixes)) {
        number /= div;
        prefix++;
    }

    buffer[0] = '~';

    int len = sputi(buffer + 1, buffer_size, number, 10) + 1;
    
    if(prefix && len < buffer_size) {
        buffer[len++] = prefixes[prefix];
        buffer[len++] = 'i';
    }

    buffer[len++] = 'B';

    return len;
}

int kvsnprintf(char* buffer, int buffer_size, const char* format, va_list args) {
    int i            = 0;
    char c           = 0;
    bool placeholder = false;

    while((c = *format++) && i < buffer_size) {
        if(c == '%') {
            placeholder = true;
            continue;
        }

        if(placeholder) {
            switch(c) {
                case 'c':
                    buffer[i++] = va_arg(args, int);
                    break;
                case 'u':
                    i += sputui(buffer + i, buffer_size - i, va_arg(args, uint64_t), 10);
                    break;
                case 'd':
                    i += sputi(buffer + i, buffer_size - i, va_arg(args, int), 10);
                    break;
                case 'x':
                    i += sputui(buffer + i, buffer_size - i, va_arg(args, uint64_t), 16);
                    break;
                case 'B':
                    i += sputbytes(buffer + i, buffer_size - i, va_arg(args, long));
                    break;
            }

            placeholder = false;
        }
        else {
            buffer[i++] = c;
        }
    }

    buffer[i] = 0;

    return i;
}
