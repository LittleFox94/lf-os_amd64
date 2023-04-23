#include <fbconsole.h>
#include <mm.h>
#include <vm.h>
#include <log.h>
#include <scheduler.h>

#include <string.h>
#include <stdarg.h>

#include <config.h>
#include "font_acorn_8x8.c"

bool fbconsole_active = false;

struct fbconsole_data {
    int      width;
    int      height;
    int      stride;
    uint8_t* fb;
    uint8_t* backbuffer;

    int cols, rows, current_col, current_row;
    int foreground_r, foreground_g, foreground_b;
    int background_r, background_g, background_b;

    int palette[16][3];
};

static struct fbconsole_data fbconsole;

void fbconsole_init(int width, int height, int stride, uint8_t* fb) {
    fbconsole.width      = width;
    fbconsole.height     = height;
    fbconsole.stride     = stride;
    fbconsole.fb         = fb;
    fbconsole.backbuffer = fb;

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
            { 170,   0,   0 },
            {   0, 170,   0 },
            { 170, 170,   0 },
            {   0,   0, 170 },
            { 170,   0, 170 },
            {   0, 170, 170 },
            { 170, 170, 170 },
            {  85,  85,  85 },
            { 255,  85,  85 },
            {  85, 255,  85 },
            { 255, 255,  85 },
            {  85,  85, 255 },
            { 255,  85, 255 },
            {  85, 255, 255 },
            { 255, 255, 255 },
        }, sizeof(int) * 16 * 3);

    fbconsole_active = true;

    if(!fb) {
        logi("fbconsole", "No framebuffer, will not actually render anything");
        return;
    }

    fbconsole_clear(fbconsole.background_r, fbconsole.background_g, fbconsole.background_b);

    logd("framebuffer", "framebuffer console @ 0x%x (0x%x) %dx%d (stride %d)",
        fb,
        (uint64_t)vm_context_get_physical_for_virtual(VM_KERNEL_CONTEXT, (ptr_t)fb),
        width, height, stride
    );
}

void fbconsole_init_backbuffer() {
    if(!fbconsole.fb) {
        return;
    }

    size_t backbuffer_size      = fbconsole.stride * fbconsole.height * 4;
    size_t backbuffer_num_pages = (backbuffer_size + 4095) / 4096;

    uint8_t* backbuffer = (uint8_t*)vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_KERNEL_HEAP, backbuffer_num_pages);

    memcpy(backbuffer, fbconsole.fb, fbconsole.stride * fbconsole.height * 4);
    fbconsole.backbuffer = backbuffer;

    for(ptr_t page = (ptr_t)fbconsole.fb; page < (ptr_t)(fbconsole.fb + (fbconsole.stride * fbconsole.height * 4)); page += 4096) {
        vm_context_map(VM_KERNEL_CONTEXT, page, vm_context_get_physical_for_virtual(VM_KERNEL_CONTEXT, page), 0x07);
    }
}

void fbconsole_clear(int r, int g, int b) {
    if(!fbconsole_active) {
        logd("fbconsole", "was in userspace control, fbconsole_clear gave it back to kernel");
        fbconsole_active = true;
    }

    if(!fbconsole.fb) {
        return;
    }

    memset32((uint32_t*)fbconsole.backbuffer, (r << 16) | (g << 8) | (b << 0), fbconsole.stride * fbconsole.height);

    if(fbconsole.backbuffer != fbconsole.fb) {
        memset32((uint32_t*)fbconsole.fb, (r << 16) | (g << 8) | (b << 0), fbconsole.stride * fbconsole.height);
    }

    fbconsole.current_row = 0;
    fbconsole.current_col = 0;
}

void fbconsole_blt(const uint8_t* image, uint16_t width, uint16_t height, int16_t tx, int16_t ty) {
    if(!fbconsole.fb) {
        return;
    }

    if(tx < 0) tx = fbconsole.width + tx;
    if(ty < 0) ty = fbconsole.height + ty;

    if(tx < 0) tx = 0;
    if(ty < 0) ty = 0;

    for(uint16_t y = 0; y < height; ++y) {
        size_t imgRow =  y       *           width  * 4;
        size_t fbRow  = (ty + y) * fbconsole.stride * 4;
        for(uint16_t x = 0; x < width && x + tx < fbconsole.width; ++x) {
            size_t imgCol = imgRow + (x * 4);
            size_t fbCol  = fbRow  + ((tx + x) * 4);

            fbconsole.backbuffer[fbCol + 2] = image[imgCol + 0];
            fbconsole.backbuffer[fbCol + 1] = image[imgCol + 1];
            fbconsole.backbuffer[fbCol + 0] = image[imgCol + 2];
            fbconsole.backbuffer[fbCol + 3] = 0;
        }

        if(fbconsole.fb != fbconsole.backbuffer) {
            memcpy(fbconsole.fb + fbRow + (tx * 4), fbconsole.backbuffer + fbRow + (tx * 4), width * 4);
        }
    }
}

void fbconsole_setpixel(const int x, const int y, const int r, const int g, const int b) {
    if(!fbconsole.fb) {
        return;
    }

    int index = ((y * fbconsole.stride) + x) * 4;
    fbconsole.backbuffer[index + 2] = r;
    fbconsole.backbuffer[index + 1] = g;
    fbconsole.backbuffer[index + 0] = b;
    fbconsole.backbuffer[index + 3] = 0;

    if(fbconsole.fb != fbconsole.backbuffer) {
        index /= 4;
        ((uint32_t*)fbconsole.fb)[index] = ((uint32_t*)fbconsole.backbuffer)[index];
    }
}

void fbconsole_draw_char(int start_x, int start_y, char c) {
    if(!fbconsole.fb) {
        return;
    }

    for(int y = 0; y < FONT_HEIGHT && y + start_y < fbconsole.height; y++) {
        for(int x = 0; x < FONT_WIDTH && x + start_x < fbconsole.width; x++) {
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
    if(!fbconsole.fb) {
        return;
    }

    size_t row_start = scroll_amount * (FONT_HEIGHT + FONT_ROW_SPACING);
    size_t begin     = row_start * fbconsole.stride * 4;
    size_t end       = fbconsole.stride * fbconsole.height * 4;

    memcpy(fbconsole.backbuffer, fbconsole.backbuffer + begin, end - begin);

    memset32(
        (uint32_t*)(fbconsole.backbuffer + (end - (row_start * 4 * fbconsole.stride))),
        (fbconsole.background_r << 16) | (fbconsole.background_g << 8) | (fbconsole.background_b << 0),
        row_start * fbconsole.stride
    );

    if(fbconsole.fb != fbconsole.backbuffer) {
        memcpy(fbconsole.fb, fbconsole.backbuffer, fbconsole.stride * fbconsole.height * 4);
    }
}

void fbconsole_next_line() {
    if(!fbconsole.fb) {
        return;
    }

    fbconsole.current_col = 0;
    ++fbconsole.current_row;

    if(fbconsole.current_row == fbconsole.rows) {
        fbconsole_scroll(1);
        fbconsole.current_row--;
    }
}

int fbconsole_write(char* string, ...) {
    if(!fbconsole.fb) {
        return 0;
    }

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
        else if(c == '\r') {
            fbconsole.current_col = 0;
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

void sc_handle_hardware_framebuffer(ptr_t *fb, uint16_t *width, uint16_t *height, uint16_t* stride, uint16_t* colorFormat) {
    if(fbconsole_active) {
        *width = fbconsole.width;
        *height = fbconsole.height;
        *stride = fbconsole.stride;
        *colorFormat = 0; // to be specified

        if(fbconsole.fb) {
            *fb = scheduler_map_hardware(vm_context_get_physical_for_virtual(VM_KERNEL_CONTEXT, (ptr_t)fbconsole.fb), *stride * *height * 4);
            fbconsole_clear(0, 0, 0);
            fbconsole_active = false;

            logd("fbconsole", "Gave up control of framebuffer, now process %d is in charge", scheduler_current_process);
        } else {
            *fb = 0;

            logd("fbconsole", "Would have gave up control of framebuffer to process %d, but we don't have one", scheduler_current_process);
        }

    }
}
