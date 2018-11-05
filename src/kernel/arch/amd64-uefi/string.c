#include "string.h"
#include "stdbool.h"
#include "stdint.h"

void* memcpy(void* dest, void* source, size_t size) {
    if((ptr_t)dest % 8 == 0 && (ptr_t)source % 8 == 0) {
        uint64_t* dst_64 = (uint64_t*)dest;
        uint64_t* src_64 = (uint64_t*)source;

        while(size > 64) {
            *dst_64++ = *src_64++;
            *dst_64++ = *src_64++;
            *dst_64++ = *src_64++;
            *dst_64++ = *src_64++;
            *dst_64++ = *src_64++;
            *dst_64++ = *src_64++;
            *dst_64++ = *src_64++;
            *dst_64++ = *src_64++;

            size -= 64;
        }

        while(size) {
            *dst_64++ = *src_64++;
            size -= 8;
        }
    }
    else {
        uint8_t* dst_8 = (uint8_t*)dest;
        uint8_t* src_8 = (uint8_t*)source;

        while(size) {
            *dst_8++ = *src_8++;
            size--;
        }
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
                    i += sputi(buffer + i, buffer_size - i, va_arg(args, long), 16);
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
