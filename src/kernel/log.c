#include <config.h>
#include <stdint.h>
#include <string.h>
#include <panic.h>
#include <fbconsole.h>
#include <log.h>
#include <vm.h>
#include <scheduler.h>
#include <efi.h>
#include <io.h>

const int logging_page_size = 4080;

struct logging_page {
    struct logging_page* prev;
    struct logging_page* next;
    uint16_t current_end;

    char messages[logging_page_size - 18];
};

// this is our logging page to use before memory management is up
struct logging_page log_initial_page = {
    .prev        = 0,
    .next        = 0,
    .current_end = 0,
};

struct logging_page* log_first      = &log_initial_page;
struct logging_page* log_last       = &log_initial_page;
uint64_t             log_page_count = 1;
uint64_t             log_count      = 0;

void log_append_page(void) {
    struct logging_page* new = (struct logging_page*)vm_alloc(logging_page_size);
    memset((void*)new, 0, logging_page_size);
    new->prev = log_last;
    log_last->next = new;
    log_last = new;

    ++log_page_count;

    while(log_page_count * logging_page_size > LOG_MAX_BUFFER) {
        struct logging_page* page = log_first;

        page->next->prev = 0;
        log_first = page->next;
        --log_page_count;

        for(int i = 0; i < page->current_end; ++i) {
            if(!page->messages[i]) {
                --log_count;
            }
        }

        if(page != &log_initial_page) {
            vm_free(page);
        }
    }
}

void log_append(char level, char* component, char* message) {
    size_t len = strlen(component) + strlen(message) + 4; // 4: level, two tabs and terminator

    if(len > sizeof(log_first->messages) - log_last->current_end) {
        log_append_page();
    }

    uint64_t msg_start = log_last->current_end;

    ksnprintf(log_last->messages + log_last->current_end, sizeof(log_last->messages) - log_last->current_end, "%c\t%s\t%s", (int)level, component, message);
    log_last->current_end += len;
    uint64_t msg_end = log_last->current_end;

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

        fbconsole_write("\x1b[38;5;%dm[%c] %s: %s\n", color_code, level, component, message);
    }

    if(LOG_COM0) {
        for(size_t i = msg_start; i < msg_end-1; ++i) {
            while((inb(0x3F8 + 5) & 0x20) == 0) {}
            outb(0x3F8, log_last->messages[i]);
        }
        while((inb(0x3F8 + 5) & 0x20) == 0) {}
        outb(0x3F8, '\r');
        while((inb(0x3F8 + 5) & 0x20) == 0) {}
        outb(0x3F8, '\n');
    }

    if(LOG_EFI) {
        efi_append_log(log_last->messages + msg_start);
    }

    ++log_count;
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

void sc_handle_debug_print(char* message) {
    if(message[strlen(message)-1] == '\n') {
        message[strlen(message)-1] = 0;
    }

    char buffer[20];
    ksnprintf(buffer, sizeof(buffer), "process %d", scheduler_current_process);

    logd(buffer, "%s", message);
}
