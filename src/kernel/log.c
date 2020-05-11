#include <stdint.h>
#include <string.h>
#include <bluescreen.h>
#include <fbconsole.h>
#include <log.h>
#include <vm.h>

const int logging_page_size = 4096;

struct logging_page {
    struct logging_page* prev;
    struct logging_page* next;
    uint16_t current_end;

    char messages[logging_page_size - 18];
};

struct logging_page log_initial_page = {
    .prev        = 0,
    .next        = 0,
    .current_end = 0,
};

struct logging_page* log_first = &log_initial_page;
struct logging_page* log_last  = &log_initial_page;

void log_append_page() {
    struct logging_page* new = (struct logging_page*)vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_SLAB_4K, 1);
    memset((void*)new, 0, logging_page_size);
    new->prev = log_last;
    log_last->next = new;
    log_last = new;
}

void log_append(char level, char* component, char* message) {
    size_t len = strlen(component) + strlen(message) + 4; // 4: level, two tabs and terminator

    if(len > sizeof(log_first->messages) - log_last->current_end) {
        log_append_page();
    }

    ksnprintf(log_last->messages + log_last->current_end, sizeof(log_last->messages) - log_last->current_end, "%c\t%s\t%s", level, component, message);
    log_last->current_end += len;

    if(fbconsole_active) {
        int color_code;

        switch(level) {
            case 'D':
                color_code = 8;
                break;
            case 'I':
                color_code = 15;
                break;
            case 'W':
                color_code = 3;
                break;
            case 'E':
                color_code = 1;
                break;
            case 'F':
                color_code = 5;
                break;
            default:
                color_code = 4;
                break;
        }

        fbconsole_write("\e[38;5;%dm[%c] %s: %s\n", color_code, level, component, message);
    }
}

void log(char level, char* component, char* fmt, ...) {
    va_list args;
    char buffer[512];
    memset((uint8_t*)buffer, 0, 512);

    va_start(args, fmt);
    kvsnprintf(buffer, 512, fmt, args);
    va_end(args);

    log_append(level, component, buffer);
}

int scheduler_current_process;
void sc_handle_debug_print(char* message) {
    if(message[strlen(message)-1] == '\n') {
        message[strlen(message)-1] = 0;
    }

    logd("userspace", "debug from %d: %s", scheduler_current_process, message);
}
