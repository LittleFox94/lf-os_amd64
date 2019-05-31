#include "fbconsole.h"
#include "mm.h"

#include "string.h"
#include "stdarg.h"

#include "config.h"
#include "font_acorn_8x8.c"

struct fbconsole_data fbconsole;

void __set_mtrr_wc(uint64_t fb, uint64_t len) {
    fb = mm_get_mapping(fb);

    uint64_t fb_lo = fb  & 0x0FFFFF000;
    uint64_t fb_hi = (fb & 0x700000000) >> 32;

    asm volatile("wrmsr" :: "a"(fb_lo | 1),    "c"(0x202), "d"(fb_hi));
    asm volatile("wrmsr" :: "a"(0xF8000800),   "c"(0x203), "d"(0x7));
    asm volatile("wrmsr" :: "a"(0x800 | 0x04), "c"(0x2FF), "d"(0));
}

void fbconsole_init(int width, int height, uint8_t* fb) {
    fbconsole.width  = width;
    fbconsole.height = height;
    fbconsole.fb     = fb;

    fbconsole.cols        = width  / (FONT_WIDTH + FONT_COL_SPACING);
    fbconsole.rows        = height / (FONT_HEIGHT + FONT_ROW_SPACING);
    fbconsole.current_col = 0;
    fbconsole.current_row = 0;

    fbconsole.foreground_r = 255;
    fbconsole.foreground_g = 255;
    fbconsole.foreground_b = 255;
    fbconsole.background_r = 0;
    fbconsole.background_g = 0;
    fbconsole.background_b = 0;

    memcpy(fbconsole.palette, (int[16][3]){
            {   0,   0,   0 },
            { 127,   0,   0 },
            {   0, 127,   0 },
            { 127, 127,   0 },
            {   0,   0, 127 },
            { 127,   0, 127 },
            {   0, 127, 127 },
            { 127, 127, 127 },
            {   0,   0,   0 },
            { 255,   0,   0 },
            {   0, 255,   0 },
            { 255, 255,   0 },
            {   0,   0, 255 },
            { 255,   0, 255 },
            {   0, 255, 255 },
            { 255, 255, 255 },
        }, sizeof(int) * 16 * 3);

    __set_mtrr_wc((uint64_t)fb, width * height * 4);

    fbconsole_clear(0, 0, 0);
}

void fbconsole_clear(int r, int g, int b) {
    memset32((uint32_t*)fbconsole.fb, (r << 16) | (g << 8) | (b << 0), fbconsole.width * fbconsole.height);
    fbconsole.current_row = 0;
    fbconsole.current_col = 0;
}

void fbconsole_setpixel(int x, int y, int r, int g, int b) {
    int index = ((y * fbconsole.width) + x) * 4;
    fbconsole.fb[index + 2] = r;
    fbconsole.fb[index + 1] = g;
    fbconsole.fb[index + 0] = b;
    fbconsole.fb[index + 3] = 0;
}

void fbconsole_draw_char(int start_x, int start_y, char c) {
    for(int x = 0; x < FONT_WIDTH && x + start_x < fbconsole.width; x++) {
        for(int y = 0; y < FONT_HEIGHT && y + start_y < fbconsole.height; y++) {
            if(FONT_NAME[(c * FONT_HEIGHT) + y] & (0x80 >> x)) {
                fbconsole_setpixel(start_x + x, start_y + y, fbconsole.foreground_r, fbconsole.foreground_g, fbconsole.foreground_b);
            }
            else {
                fbconsole_setpixel(start_x + x, start_y + y, fbconsole.background_r, fbconsole.background_g, fbconsole.background_b);
            }
        }
    }
}

void fbconsole_scroll(unsigned int scroll_amount) {
    unsigned int row_start = scroll_amount * FONT_HEIGHT;
    unsigned int begin     = row_start * fbconsole.width * 4;
    unsigned int end       = fbconsole.width * fbconsole.height * 4;

    memcpy(fbconsole.fb, fbconsole.fb + begin, end - begin);

    for(unsigned int i = end - begin; i < end; i++) {
        fbconsole.fb[i] = 0;
    }
}

void fbconsole_next_line() {
    fbconsole.current_col = 0;
    ++fbconsole.current_row;

    if(fbconsole.current_row == fbconsole.rows) {
        fbconsole_scroll(1);
        fbconsole.current_row--;
    }
}

int fbconsole_write(char* string, ...) {
    va_list args;
    char buffer[512];
    memset((uint8_t*)buffer, 0, 512);

    va_start(args, string);
    int count = kvsnprintf(buffer, 512, string, args);
    va_end(args);

    int i = 0;
    int inside_ansi_sequence = 0;
    int ansi_command = 0;
    int ansi_args[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int ansi_parse_step = 0;

    while(count-- && i < 512) {
        char c = buffer[i++];

        if(inside_ansi_sequence) {
            if(c == ';') {
                ++ansi_parse_step;
                continue;
            }
            else if(c == 'm') {
                inside_ansi_sequence = 0;

                switch(ansi_command) {
                    case 38:
                        if(ansi_args[0] == 5) {
                            fbconsole.foreground_r = fbconsole.palette[ansi_args[1]][0];
                            fbconsole.foreground_g = fbconsole.palette[ansi_args[1]][1];
                            fbconsole.foreground_b = fbconsole.palette[ansi_args[1]][2];
                        }
                        else if(ansi_args[0] == 2) {
                            fbconsole.foreground_r = ansi_args[1];
                            fbconsole.foreground_g = ansi_args[2];
                            fbconsole.foreground_b = ansi_args[3];
                        }
                        break;
                    case 48:
                        if(ansi_args[0] == 5) {
                            fbconsole.background_r = fbconsole.palette[ansi_args[1]][0];
                            fbconsole.background_g = fbconsole.palette[ansi_args[1]][1];
                            fbconsole.background_b = fbconsole.palette[ansi_args[1]][2];
                        }
                        else if(ansi_args[0] == 2) {
                            fbconsole.foreground_r = ansi_args[1];
                            fbconsole.foreground_g = ansi_args[2];
                            fbconsole.foreground_b = ansi_args[3];
                        }
                        break;
                }

                continue;
            }

            if(ansi_parse_step == 0) {
                ansi_command *= 10;
                ansi_command += c - '0';
            }
            else {
                ansi_args[ansi_parse_step-1] *= 10;
                ansi_args[ansi_parse_step-1] += c - '0';
            }

            continue;
        }

        if(c == '\n') {
            fbconsole_next_line();
            continue;
        }
        else if(c == 0x1B && buffer[i] == '[') {
            ++i;
            inside_ansi_sequence = 1;

            ansi_command    = 0;
            ansi_parse_step = 0;
            memset((void*)ansi_args, 0, sizeof(int) * 8);
            continue;
        }
        else if(c == 0) {
            break;
        }

        fbconsole_draw_char(fbconsole.current_col++ * (FONT_WIDTH + FONT_COL_SPACING), fbconsole.current_row * (FONT_HEIGHT + FONT_ROW_SPACING), c);

        if(fbconsole.current_col == fbconsole.cols) {
            fbconsole_next_line();
        }
    }

    return i;
}
