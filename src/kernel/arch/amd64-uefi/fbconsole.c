#include "fbconsole.h"
#include "font_sun8x16.c"

#include "string.h"
#include "stdarg.h"

struct fbconsole_data fbconsole;

void fbconsole_init(int width, int height, uint8_t* fb) {
    fbconsole.width  = width;
    fbconsole.height = height;
    fbconsole.fb     = fb;

    fbconsole.cols        = width / 8;
    fbconsole.rows        = height / 16;
    fbconsole.current_col = 0;
    fbconsole.current_row = 0;

    fbconsole.foreground_r = 255;
    fbconsole.foreground_g = 255;
    fbconsole.foreground_b = 255;
    fbconsole.background_r = 0;
    fbconsole.background_g = 0;
    fbconsole.background_b = 0;

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

void fbconsole_scroll(int scroll_amount) {
    
}

void fbconsole_next_line() {
    fbconsole.current_col = 0;
    fbconsole.current_row++;

    if(fbconsole.current_row == fbconsole.rows) {
        fbconsole_scroll(1);
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
