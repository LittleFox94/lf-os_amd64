#ifndef _FBCONSOLE_H_INCLUDED
#define _FBCONSOLE_H_INCLUDED

#include <stdint.h>

struct fbconsole_data {
    int      width;
    int      height;
    uint8_t* fb;

    int cols, rows, current_col, current_row;
    int foreground_r, foreground_g, foreground_b;
    int background_r, background_g, background_b;

    int palette[16][3];
};

extern struct fbconsole_data fbconsole;

void fbconsole_init(int width, int height, uint8_t* fb);
void fbconsole_clear(int r, int g, int b);
void fbconsole_setpixel(int x, int y, int r, int g, int b);
int  fbconsole_write(char* string, ...);

#endif
