#include "fbconsole.h"
#include "font_sun8x16.c"
#include "mm.h"

#include "string.h"
#include "stdarg.h"

struct fbconsole_data fbconsole;

void __set_mtrr_wc(uint64_t fb, uint64_t len) {
    fb = mm_get_mapping(fb);

    uint64_t fb_lo = fb  & 0x0FFFFF000;
    uint64_t fb_hi = (fb & 0x700000000) >> 32;

    asm volatile("wrmsr" :: "a"(fb_lo | 1),     "c"(0x202), "d"(fb_hi));
    asm volatile("wrmsr" :: "a"(0xF8000800),    "c"(0x203), "d"(0x7));
    asm volatile("wrmsr" :: "a"(0x800 | 0x04), "c"(0x2FF), "d"(0));
}

void fbconsole_init(int width, int height, uint8_t* fb) {
    fbconsole.width  = width;
    fbconsole.height = height;
    fbconsole.fb     = fb;

    fbconsole.cols        = width / 8;
    fbconsole.rows        = height / 16;
    fbconsole.current_col = 0;
    fbconsole.current_row = 0;

    fbconsole.foreground_r = 127;
    fbconsole.foreground_g = 127;
    fbconsole.foreground_b = 127;
    fbconsole.background_r = 0;
    fbconsole.background_g = 0;
    fbconsole.background_b = 0;

    __set_mtrr_wc((uint64_t)fb, width * height * 4);

    fbconsole_clear(0, 0, 0);
}

void fbconsole_clear(int r, int g, int b) {
    for(int y = 0; y < fbconsole.height; y++) {
        for(int x = 0; x < fbconsole.width; x++) {
            fbconsole_setpixel(x, y, r, g, b);
        }
    }
}

void fbconsole_setpixel(int x, int y, int r, int g, int b) {
    int index = ((y * fbconsole.width) + x) * 4;
    fbconsole.fb[index    ] = r;
    fbconsole.fb[index + 1] = g;
    fbconsole.fb[index + 2] = b;
    fbconsole.fb[index + 3] = 0;
}

void fbconsole_draw_char(int start_x, int start_y, char c) {
    for(int y = 0; y < 16 && y + start_y < fbconsole.height; y++) {
        for(int x = 0; x < 8 && x + start_x < fbconsole.width; x++) {
            if(fontdata_sun8x16[(c * 16) + y] & (0x80 >> x)) {
                fbconsole_setpixel(start_x + x, start_y + y, fbconsole.foreground_r, fbconsole.foreground_g, fbconsole.foreground_b);
            }
            else {
                fbconsole_setpixel(start_x + x, start_y + y, fbconsole.background_r, fbconsole.background_g, fbconsole.background_b);
            }
        }
    }
}

void fbconsole_scroll(unsigned int scroll_amount) {
    unsigned int row_start = scroll_amount * 16;
    unsigned int begin     = row_start * fbconsole.width * 4;
    unsigned int end       = fbconsole.width * fbconsole.height * 4;

    memcpy(fbconsole.fb, fbconsole.fb + begin, end - begin);

    for(unsigned int i = end - begin; i < end; i++) {
        fbconsole.fb[i] = 0;
    }
}

void fbconsole_next_line() {
    fbconsole.current_col = 0;
    fbconsole.current_row++;

    if(fbconsole.current_row == fbconsole.rows) {
        fbconsole_scroll(1);
        fbconsole.current_row--;
    }
}

int fbconsole_write(char* string, ...) {
    va_list args;
    char buffer[512];

    va_start(args, string);
    int count = kvsnprintf(buffer, 512, string, args);
    va_end(args);

    int i = 0;
    while(count-- && i < 512) {
        char c = buffer[i++];

        if(c == '\n') {
            fbconsole_next_line();
            continue;
        }
        else if(c == 0) {
            break;
        }

        fbconsole_draw_char(fbconsole.current_col++ * 8, fbconsole.current_row * 16, c);

        if(fbconsole.current_col == fbconsole.cols) {
            fbconsole_next_line();
        }
    }

    return i;
}
