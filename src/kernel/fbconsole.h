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

void fbconsole_init(int width, int height, uint8_t* fb);
struct fbconsole_data* fbconsole_instance();

void fbconsole_clear(int r, int g, int b);
void fbconsole_setpixel(int x, int y, int r, int g, int b);
int  fbconsole_write(char* string, ...);
void fbconsole_blt(uint8_t* image, uint16_t width, uint16_t height, uint16_t x, uint16_t y);

#endif
