#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include <sys/syscalls.h>

#include "main.h"
#include "keyboard.h"

#include "font.h"
extern struct font terminal_font;

#define FONT_WIDTH  (terminal_font.bbox.width)
#define FONT_HEIGHT (terminal_font.bbox.height)

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

    bool reverseBits = cell->attrs.reverse;
    if(state->cursor_pos.row == row && state->cursor_pos.col == col) {
        reverseBits = !reverseBits;
    }

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

            if(reverseBits) {
                pixel_lit = !pixel_lit;
            }

            *pixel = pixel_lit ? foreground : background;

            if(y == FONT_HEIGHT - 1 && cell->attrs.underline) {
                *pixel = foreground;
            }
        }
    }
}

static void vterm_render_cell(lfos_term_state *state, int row, int col) {
    VTermScreenCell cell;
    vterm_screen_get_cell(state->vtermScreen, (VTermPos){ row, col }, &cell);
    render_cell(state, row, col, &cell);
}

static int vterm_damage(VTermRect rect, void* user) {
    lfos_term_state *state = (lfos_term_state*)user;

    for(int row = rect.start_row; row < rect.end_row; ++row) {
        for(int col = rect.start_col; col < rect.end_col; ++col) {
            vterm_render_cell(state, row, col);
        }
    }

    return 1;
}

static int vterm_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user) {
    lfos_term_state *state = (lfos_term_state*)user;
    state->cursor_pos = pos;

    vterm_render_cell(state, pos.row, pos.col);
    vterm_render_cell(state, oldpos.row, oldpos.col);
    return 1;
}

static void vterm_output_callback(const char *s, size_t len, void *user) {
    lfos_term_state *state = (lfos_term_state*)user;
    char* resized_input_buffer = malloc(state->input_buffer_len + len);
    memcpy(resized_input_buffer, state->input_buffer_current, state->input_buffer_len);
    memcpy(resized_input_buffer + state->input_buffer_len, s, len);

    if(state->input_buffer) {
        free(state->input_buffer);
    }

    state->input_buffer = state->input_buffer_current
                        = resized_input_buffer;
    state->input_buffer_len += len;
}

lfos_term_state state;
int read_lfos(int file, char *ptr, int len) {
    while(true) {
        kbd_read(&state);

        if(state.input_buffer_len) {
            size_t to_read = len;
            if(to_read > state.input_buffer_len) {
                to_read = state.input_buffer_len;
            }

            memcpy(ptr, state.input_buffer_current, to_read);
            state.input_buffer_current += to_read;
            state.input_buffer_len     -= to_read;

            return to_read;
        }
    }
}

int write_lfos(int file, char *ptr, int len) {
    vterm_input_write(state.vterm, ptr, len);
    return len;
}


int klsh_main();

int main(int argc, char* argv[]) {
    bzero(&state, sizeof(state));

    kbd_init();
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
    screenCallbacks.movecursor = vterm_movecursor;
    vterm_screen_set_callbacks(state.vtermScreen, &screenCallbacks, (void*)&state);

    vterm_output_set_callback(state.vterm, vterm_output_callback, (void*)&state);

    // Setup some things to match what LF OS users would expect - like \n also
    // resetting the column without needing a CR - this is LF OS after all and
    // not CR LF OS.
    char* init = "\e[20h";
    vterm_input_write(state.vterm, init, strlen(init));

    extern unsigned char bootlogo_ans[];
    extern unsigned int bootlogo_ans_len;
    printf("%*s\e[0m\n", bootlogo_ans_len, bootlogo_ans);

    return klsh_main();
}
