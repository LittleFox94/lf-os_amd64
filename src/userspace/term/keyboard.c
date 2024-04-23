#include <stdint.h>
#include <string.h>

#include <sys/syscalls.h>
#include <sys/io.h>
#include <errno.h>

#include "main.h"
#include <vterm.h>
#include <vterm_keycodes.h>

static void translate_and_feed_scancode(lfos_term_state* term, int set, uint16_t scancode) {
    static bool caps  = false;
    static int shift  = 0;
    static int alt    = 0;
    static int ctrl   = 0;

    if(set == 0) {
        switch(scancode) {
            case 0x2A:
            case 0x36:
                ++shift;
                return;
            case 0xAA:
            case 0xB6:
                --shift;
                return;

            case 0xBA:
                caps = !caps;
                return;

            case 0x38:
                ++alt;
                return;
            case 0xB8:
                --alt;
                return;

            case 0x1D:
                ++ctrl;
                return;
            case 0x9D:
                --ctrl;
                return;
        }
    } else if(set == 1) {
        switch(scancode) {
            case 0x38:
                ++alt;
                return;
            case 0xB8:
                --alt;
                return;

            case 0x1D:
                ++ctrl;
                return;
            case 0x9D:
                --ctrl;
                return;
        }
    }

    #define vterm_keycode(x)  ((uint16_t)(x) | 0x8000)
    #define unused_single(x)                   0xFFFF
    #define unused(x)                          unused_single(x), unused_single(x), unused_single(x)
    #define unmapped                           unused(not_mapped)
    #define unmapped_single                        unused_single(not_mapped)

    static const uint16_t set1[][3] = {
        { unmapped },
        { vterm_keycode(VTERM_KEY_ESCAPE), 0,       unmapped_single },
        { '1', '!',                                 unmapped_single },
        { '2', '@',                                 unmapped_single },
        { '3', '#',                                 unmapped_single },
        { '4', '$',                                 unmapped_single },
        { '5', '%',                                 unmapped_single },
        { '6', '^',                                 unmapped_single },
        { '7', '&',                                 unmapped_single },
        { '8', '*',                                 unmapped_single },
        { '9', '(',                                 unmapped_single },
        { '0', ')',                                 unmapped_single },
        { '-', '_',                                 unmapped_single },
        { '=', '+',                                 unmapped_single },
        { vterm_keycode(VTERM_KEY_BACKSPACE), 0,    unmapped_single },
        { vterm_keycode(VTERM_KEY_TAB), 0,          unmapped_single },

        { 'q', 'Q',                                 unused_single(previous_track) },
        { 'w', 'W',                                 unmapped_single },
        { 'e', 'E',                                 unmapped_single },
        { 'r', 'R',                                 unmapped_single },
        { 't', 'T',                                 unmapped_single },
        { 'y', 'Y',                                 unmapped_single },
        { 'u', 'U',                                 unmapped_single },
        { 'i', 'I',                                 unmapped_single },
        { 'o', 'O',                                 unmapped_single },
        { 'p', 'P',                                 unused_single(next_track) },
        { '[', '{',                                 unmapped_single },
        { ']', '}',                                 unmapped_single },
        { vterm_keycode(VTERM_KEY_ENTER), 0,        vterm_keycode(VTERM_KEY_KP_ENTER) },
        { unused(left_right_ctrl) },
        { 'a', 'A',                                 unmapped_single },
        { 's', 'S',                                 unmapped_single },

        { 'd', 'D',                                 unused_single(mute) },
        { 'f', 'F',                                 unused_single(calculator) },
        { 'g', 'G',                                 unused_single(play) },
        { 'h', 'H',                                 unmapped_single },
        { 'j', 'J',                                 unused_single(stop) },
        { 'k', 'K',                                 unmapped_single },
        { 'l', 'L',                                 unmapped_single },
        { ';', ':',                                 unmapped_single },
        { '\'', '"',                                unmapped_single },
        { '`', '~',                                 unmapped_single },
        { unused(left_shift) },
        { '\\', '|',                                unmapped_single },
        { 'z', 'Z',                                 unmapped_single },
        { 'x', 'X',                                 unmapped_single },
        { 'c', 'C',                                 unused_single(volume_down) },
        { 'v', 'V',                                 unmapped_single },

        { 'b', 'B',                                 unused_single(volume_up) },
        { 'n', 'N',                                 unmapped_single },
        { 'm', 'M',                                 unused_single(www_home) },
        { ',', '<',                                 unmapped_single },
        { '.', '>',                                 unmapped_single },
        { '/', '?',                                 vterm_keycode(VTERM_KEY_KP_DIVIDE) },
        { unused(right_shift) },
        { vterm_keycode(VTERM_KEY_KP_MULT), 0,      unmapped_single },
        { unused(left_right_alt) },
        { ' ', ' ',                                 unmapped_single },
        { unused(caps) },
        { vterm_keycode(VTERM_KEY_FUNCTION(1)), 0,  unmapped_single },
        { vterm_keycode(VTERM_KEY_FUNCTION(2)), 0,  unmapped_single },
        { vterm_keycode(VTERM_KEY_FUNCTION(3)), 0,  unmapped_single },
        { vterm_keycode(VTERM_KEY_FUNCTION(4)), 0,  unmapped_single },
        { vterm_keycode(VTERM_KEY_FUNCTION(5)), 0,  unmapped_single },

        { vterm_keycode(VTERM_KEY_FUNCTION(6)), 0,  unmapped_single },
        { vterm_keycode(VTERM_KEY_FUNCTION(7)), 0,  unmapped_single },
        { vterm_keycode(VTERM_KEY_FUNCTION(8)), 0,  unmapped_single },
        { vterm_keycode(VTERM_KEY_FUNCTION(9)), 0,  unmapped_single },
        { vterm_keycode(VTERM_KEY_FUNCTION(10)), 0, unmapped_single },
        { unused(num_lock) },
        { unused(scroll_lock) },
        { vterm_keycode(VTERM_KEY_KP_7), 0,         vterm_keycode(VTERM_KEY_HOME) },
        { vterm_keycode(VTERM_KEY_KP_8), 0,         vterm_keycode(VTERM_KEY_UP) },
        { vterm_keycode(VTERM_KEY_KP_9), 0,         vterm_keycode(VTERM_KEY_PAGEUP) },
        { vterm_keycode(VTERM_KEY_KP_MINUS), 0,     unmapped_single },
        { vterm_keycode(VTERM_KEY_KP_4), 0,         vterm_keycode(VTERM_KEY_LEFT) },
        { vterm_keycode(VTERM_KEY_KP_5), 0,         unmapped_single },
        { vterm_keycode(VTERM_KEY_KP_6), 0,         vterm_keycode(VTERM_KEY_RIGHT) },
        { vterm_keycode(VTERM_KEY_KP_PLUS), 0,      unmapped_single },
        { vterm_keycode(VTERM_KEY_KP_1), 0,         vterm_keycode(VTERM_KEY_END) },

        { vterm_keycode(VTERM_KEY_KP_2), 0,         vterm_keycode(VTERM_KEY_DOWN) },
        { vterm_keycode(VTERM_KEY_KP_3), 0,         vterm_keycode(VTERM_KEY_PAGEDOWN) },
        { vterm_keycode(VTERM_KEY_KP_0), 0,         vterm_keycode(VTERM_KEY_INS) },
        { vterm_keycode(VTERM_KEY_KP_PERIOD), 0,    vterm_keycode(VTERM_KEY_DEL) },
        { unmapped },
        { unmapped },
        { unmapped },
        { vterm_keycode(VTERM_KEY_FUNCTION(11)), 0, unmapped_single },
        { vterm_keycode(VTERM_KEY_FUNCTION(12)), 0, unmapped_single },
        { unmapped },
        { unmapped },
        { unmapped },
    };

    uint16_t res;

    if(set == 0 || set == 1) {
        if(scancode < (sizeof(set1) / sizeof(set1[0])) ) {
            uint8_t idx = 0;

            if(shift && set == 0) {
                idx = 1;
            }
            else if(set == 1) {
                idx = 2;
            }

            const uint16_t* key = set1[scancode];
            res = key[idx];

            if(!res) {
                res = key[0];
            }

            if(res == 0xFFFF) {
                return;
            }

            VTermModifier mod = VTERM_MOD_NONE;
            if(shift || caps) {
                mod |= VTERM_MOD_SHIFT;
            }

            if(alt) {
                mod |= VTERM_MOD_ALT;
            }

            if(ctrl) {
                mod |= VTERM_MOD_CTRL;
            }

            if(res & 0x8000) {
                vterm_keyboard_key(term->vterm, (VTermKey)(res & ~0x8000), mod);
            } else {
                vterm_keyboard_unichar(term->vterm, res, mod);
            }
        }
    }
}

static void kbd_handle_input(lfos_term_state* term) {
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

    if(e0_code) {
        e0_code = 0;
        if(scancode == 0x2A || scancode == 0x36) {
            return;
        }

        translate_and_feed_scancode(term, 1, scancode);
    } else if(e1_code == 2) {
        e1_prev |= (uint16_t)scancode << 8;
        translate_and_feed_scancode(term, 2, e1_prev);
        e1_code = 0;
    } else if(e1_code == 1) {
        e1_prev = scancode;
        ++e1_code;
    } else if(scancode == 0xE0) {
        e0_code = 1;
    } else if(scancode == 0xE1) {
        e1_code = 1;
    } else {
        translate_and_feed_scancode(term, 0, scancode);
    }
}

static void kbd_send_cmd(uint8_t cmd) {
    while((inb(0x64) & 0x02));
    outb(0x60, cmd);
}

static bool kbd_wait_interrupt() {
    size_t size  = sizeof(struct Message) + sizeof(struct HardwareInterruptUserData);
    struct Message* msg = malloc(size);
    memset(msg, 0, size);
    msg->size = size;

    uint64_t error;
    do {
        sc_do_ipc_mq_poll(0, false, msg, &error);
    } while(
        error == EAGAIN ||
        (error == 0 && msg->type != MT_HardwareInterrupt)
   );

    return error == 0 && msg->type == MT_HardwareInterrupt;
}

void kbd_read(lfos_term_state* term) {
    while(true) {
        if(kbd_wait_interrupt()) {
            kbd_handle_input(term);
        }
        else {
            return;
        }
    }
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
