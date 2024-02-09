#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/syscalls.h>
#include <vterm.h>

#include "font.h"
extern struct font terminal_font;

#define FONT_WIDTH  (terminal_font.bbox.width)
#define FONT_HEIGHT (terminal_font.bbox.height)

typedef struct {
    uint32_t *fb;
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    uint16_t bpp;
} lfos_framebuffer;

typedef struct {
    lfos_framebuffer fb;

    VTerm       *vterm;
    VTermScreen *vtermScreen;
} lfos_term_state;

static void lfos_framebuffer_init(lfos_framebuffer *this) {
    sc_do_hardware_framebuffer(&this->fb, &this->width, &this->height, &this->stride, &this->bpp);
}

static uint32_t get_color(lfos_term_state *state, VTermColor *color) {
    if(!VTERM_COLOR_IS_RGB(color)) {
        vterm_screen_convert_color_to_rgb(state->vtermScreen, color);
    }

    return ((uint32_t)color->rgb.red   << 16) |
           ((uint32_t)color->rgb.green << 8) |
           ((uint32_t)color->rgb.blue  << 0);
}

static struct glyph *find_glyph(uint32_t codepoint) {
    if(!codepoint) {
        codepoint = ' ';
    }

    for(size_t i = 0; i < terminal_font.num_glyphs; ++i) {
        struct glyph *glyph = &terminal_font.glyphs[i];
        if(glyph->encoding == codepoint) {
            return glyph;
        }
    }

    printf("No glyph found for codepoint %u\n", codepoint);
    return find_glyph('?');
}

static void render_cell(lfos_term_state *state, int row, int col, VTermScreenCell *cell) {
    uint16_t start_x = col * FONT_WIDTH;
    uint16_t start_y = row * FONT_HEIGHT;

    uint32_t foreground = get_color(state, &cell->fg);
    uint32_t background = get_color(state, &cell->bg);

    struct glyph *glyph = find_glyph(cell->chars[0]);

    for(int y = 0; y < FONT_HEIGHT && y + start_y < state->fb.height; y++) {
        for(int x = 0; x < FONT_WIDTH && x + start_x < state->fb.width; x++) {
            uint32_t* pixel = state->fb.fb + ((y + start_y) * state->fb.stride) + (x + start_x);

            uint64_t leftmost = 1ULL << 63;
            if(FONT_WIDTH <= 8) {
                leftmost = 1 << 7;
            } else if(FONT_WIDTH <= 16) {
                leftmost = 1 << 15;
            } else if(FONT_WIDTH <= 32) {
                leftmost = 1 << 31;
            }

            bool pixel_lit = glyph->bitmap[y] & (leftmost >> x);

            if(cell->attrs.reverse) {
                pixel_lit = !pixel_lit;
            }

            *pixel = pixel_lit ? foreground : background;

            if(y == FONT_HEIGHT - 1 && cell->attrs.underline) {
                *pixel = foreground;
            }
        }
    }
}

static int vterm_damage(VTermRect rect, void* user) {
    lfos_term_state *state = (lfos_term_state*)user;

    for(int row = rect.start_row; row < rect.end_row; ++row) {
        for(int col = rect.start_col; col < rect.end_col; ++col) {
            VTermScreenCell cell;
            vterm_screen_get_cell(state->vtermScreen, (VTermPos){ row, col }, &cell);
            render_cell(state, row, col, &cell);
        }
    }

    return 1;
}

int main(int argc, char* argv[]) {
    lfos_term_state state;

    lfos_framebuffer_init(&state.fb);

    int rows = state.fb.height / FONT_HEIGHT;
    int cols = state.fb.width  / FONT_WIDTH;

    state.vterm = vterm_new(rows, cols);
    vterm_set_utf8(state.vterm, true);

    state.vtermScreen = vterm_obtain_screen(state.vterm);
    vterm_screen_reset(state.vtermScreen, 1);

    VTermScreenCallbacks screenCallbacks;
    bzero(&screenCallbacks, sizeof(screenCallbacks));
    screenCallbacks.damage = vterm_damage;
    vterm_screen_set_callbacks(state.vtermScreen, &screenCallbacks, (void*)&state);

    // Setup some things to match what LF OS users would expect - like \n also
    // resetting the column without needing a CR - this is LF OS after all and
    // not CR LF OS.
    char* init = "\e[20h";
    vterm_input_write(state.vterm, init, strlen(init));

    #include "data.inc"
    vterm_input_write(state.vterm, (char*)_home_littlefox_foo, _home_littlefox_foo_len);

    while(1) {
    //    char* data = "\e[7mHello\e[27m \e[4mworld!\n";
    //    vterm_input_write(state.vterm, data, strlen(data));
    //    sleep(1);
    }

    return 0;
}
