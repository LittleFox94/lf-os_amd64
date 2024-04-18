#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include <sys/message_passing.h>
#include <sys/syscalls.h>
#include <sys/io.h>
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

    VTermPos    cursor_pos;
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

char translate_scancode(int set, uint16_t scancode) {
    static bool shift = false;
    static bool caps  = false;

    if(set == 0) {
        switch(scancode) {
            case 0x2A:
            case 0x36:
                shift = true;
                return 0;
            case 0xAA:
            case 0xB6:
                shift = false;
                return 0;

            case 0xBA:
                caps = !caps;
                return 0;
        }
    }

    char sc2char[][2] = {
        { 0,    0, },
        { 0x1b, 0, },
        { '1', '!', },
        { '2', '@', },
        { '3', '#', },
        { '4', '$', },
        { '5', '%', },
        { '6', '^', },
        { '7', '&', },
        { '8', '*', },
        { '9', '(', },
        { '0', ')', },
        { '-', '_', },
        { '=', '+', },
        { '\b', '\b', },
        { '\t', '\t', },
        { 'q', 'Q', },
        { 'w', 'W', },
        { 'e', 'E', },
        { 'r', 'R', },
        { 't', 'T', },
        { 'y', 'Y', },
        { 'u', 'U', },
        { 'i', 'I', },
        { 'o', 'O', },
        { 'p', 'P', },
        { '[', 0, },
        { ']', 0, },
        { '\n', '\n', },
        { 0, 0, },
        { 'a', 'A', },
        { 's', 'S', },
        { 'd', 'D', },
        { 'f', 'F', },
        { 'g', 'G', },
        { 'h', 'H', },
        { 'j', 'J', },
        { 'k', 'K', },
        { 'l', 'L', },
        { ';', ':', },
        { '\'', '"', },
        { '`', '~', },
        { 0, 0, },
        { '\\', '|', },
        { 'z', 'Z', },
        { 'x', 'X', },
        { 'c', 'C', },
        { 'v', 'V', },
        { 'b', 'B', },
        { 'n', 'N', },
        { 'm', 'M', },
        { ',', '<', },
        { '.', '>', },
        { '/', '?', },
        { 0, 0, },
        { '*', 0, },
        { 0, 0, },
        { ' ', ' ', },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { '7', 0, },
        { '8', 0, },
        { '9', 0, },
        { '-', 0, },
        { '4', 0, },
        { '5', 0, },
        { '6', 0, },
        { '+', 0, },
        { '1', 0, },
        { '2', 0, },
        { '3', 0, },
        { '0', 0, },
        { '.', 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
        { 0, 0, },
    };

    if(set != 0 || scancode >= sizeof(sc2char)) {
        return -1;
    }

    char ret = sc2char[scancode][shift || caps ? 1 : 0];
    if(!ret) {
        return -1;
    }

    return ret;
}

char kbd_handle_interrupt() {
    static int     e0_code  = 0;
    static int     e1_code  = 0;
    static uint16_t e1_prev = 0;

    uint8_t scancode = inb(0x60);
    bool break_code  = false;

    if((scancode & 0x80) &&
        (e1_code || (scancode != 0xE1)) &&
        (e0_code || (scancode != 0xE0))) {
        break_code = true;
    }

    char translated = 0;
    if(e0_code) {
        e0_code = 0;
        if(scancode == 0x2A || scancode == 0x36) {
            return 0;
        }

        translated = translate_scancode(1, scancode);
    } else if(e1_code == 2) {
        e1_prev |= (uint16_t)scancode << 8;
        translated = translate_scancode(2, e1_prev);
        e1_code = 0;
    } else if(e1_code == 1) {
        e1_prev = scancode;
        ++e1_code;
    } else if(scancode == 0xE0) {
        e0_code = 1;
    } else if(scancode == 0xE1) {
        e1_code = 1;
    } else {
        translated = translate_scancode(0, scancode);
    }

    if(translated && !break_code) {
        return translated;
    }

    return 0;
}

char read_single_char() {
    size_t size  = sizeof(struct Message) + sizeof(struct HardwareInterruptUserData);
    struct Message* msg = malloc(size);
    memset(msg, 0, size);
    msg->size = size;

    uint64_t error;

    while(true) {
        do {
            sc_do_ipc_mq_poll(0, true, msg, &error);
        } while(
            error == EAGAIN ||
            (error == 0 && msg->type != MT_HardwareInterrupt)
        );

        char ret = kbd_handle_interrupt();
        if(ret) {
            return ret;
        }
    }
}

lfos_term_state state;
int read_lfos(int file, char *ptr, int len) {
    size_t i = 0;
    while(i < len) {
        char c = read_single_char();
        if(c != -1) {
            ptr[i++] = c;
        }
    }

    errno = 0;
    return i;
}

int write_lfos(int file, char *ptr, int len) {
    vterm_input_write(state.vterm, ptr, len);
    return len;
}

void kbd_send_cmd(uint8_t cmd) {
    while((inb(0x64) & 0x02));
    outb(0x60, cmd);
}

void kbd_init() {
    uint64_t error;
    sc_do_hardware_ioperm(0x60, 1, true, &error);
    sc_do_hardware_ioperm(0x64, 1, true, &error);
    sc_do_hardware_interrupt_notify(1, true, true, &error);

    while(inb(0x64) & 0x01) {
        inb(0x60);
    }

    kbd_send_cmd(0xF4);
}

int klsh_main();

int main(int argc, char* argv[]) {
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

    // Setup some things to match what LF OS users would expect - like \n also
    // resetting the column without needing a CR - this is LF OS after all and
    // not CR LF OS.
    char* init = "\e[20h";
    vterm_input_write(state.vterm, init, strlen(init));

    return klsh_main();
}
