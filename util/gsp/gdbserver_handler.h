#ifndef _GDBSERVER_HANDLER_INCLUDED
#define _GDBSERVER_HANDLER_INCLUDED

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int socket;
    bool paused;

    char*   packet;
    size_t  packet_len;
    uint8_t packet_checksum;
    size_t  packet_checksum_ctr;
    bool    packet_started;
    bool    packet_ready;

    bool waiting_for_regs;
    uint64_t address;
} GDBServerHandler;

int gdbserver_handler_init(const char* address, GDBServerHandler* handler);
void gdbserver_handler_continue(GDBServerHandler* handler);
void gdbserver_handler_break(GDBServerHandler* handler, uint64_t addr);
void gdbserver_handler_step(GDBServerHandler* handler);
bool gdbserver_handler_paused(GDBServerHandler* handler);

void gdbserver_handler_loop(GDBServerHandler* handler);

#endif
