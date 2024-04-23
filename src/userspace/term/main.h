#pragma once

#include <vterm.h>

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

    char *input_buffer;
    char *input_buffer_current;
    size_t input_buffer_len;
} lfos_term_state;
