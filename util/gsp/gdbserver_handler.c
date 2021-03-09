#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>

#include "gdbserver_handler.h"

const char* _unix_path(const char* address) {
    if(address[0] == '/')                     return address;
    if(strstr(address, "unix://") == address) return address + 7;

    fprintf(stderr, "Unable to get unix socket path from address '%s'\n", address);
    return NULL;
}

int _open_unix_socket(const char* address) {
    int ret = -1;

    struct sockaddr_un pa;
    pa.sun_family = AF_UNIX;
    strncpy(pa.sun_path, _unix_path(address), sizeof(pa.sun_path)  -1);

    if(pa.sun_path == NULL) {
        return -1;
    }

    if((ret = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Error opening socket: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    if(connect(ret, (struct sockaddr*)&pa, sizeof(pa)) == -1) {
        fprintf(stderr, "Error connecting to gdbserver: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    return ret;
}

char* _inet_host(const char* address) {
    bool ipv6 = address[0] == '[';

    char* end  = strchr(address, ipv6 ? ']' : ':');

    if(end == NULL && ipv6) {
        return NULL;
    } else if(end == NULL && !ipv6) {
        char* res = malloc(strlen(address) + 1);
        strcpy(res, address);

        return res;
    }

    size_t len = (end - address);

    char* res = malloc(len + 1);
    memset(res, 0, len + 1);
    strncpy(res, address, len);

    return res;
}

char* _inet_port(const char* address) {
    char* res  = strchr(address, ':');
    return res ? res + 1 : "1234";
}

int _open_inet6_socket(const char* address) {
    int ret = -1;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family     = AF_INET6;
    hints.ai_socktype   = SOCK_STREAM;
    hints.ai_flags      = 0;
    hints.ai_protocol   = 0;

    struct addrinfo* result;
    int err;

    char* host = _inet_host(address);
    char* port = _inet_port(address);

    if((err = getaddrinfo(host, port, &hints, &result)) != 0) {
        fprintf(stderr, "Error looking up address: %d (%s)\n", err, gai_strerror(err));
        return -1;
    }

    free(host);

    for(struct addrinfo* r = result; r; r = r->ai_next) {
        if((ret = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) == -1) {
            continue;
        }

        if(connect(ret, r->ai_addr, r->ai_addrlen) != -1) {
            break; /* success */
        }

        close(ret);
        ret = -1;
    }

    if(ret == -1) {
        fprintf(stderr, "Cannot connect to tcp socket, no address found\n");
        return -1;
    }

    freeaddrinfo(result);

    return ret;
}

int _open_stdio_socket(const char* address) {
    int sockets[2];

    if(socketpair(AF_LOCAL, SOCK_STREAM, 0, sockets) != 0) {
        fprintf(stderr, "Unable to create socket pair: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    if(fork()) {
        close(sockets[1]);
        return sockets[0];
    }
    else {
        close(sockets[0]);

        dup2(sockets[1], STDIN_FILENO);
        dup2(sockets[1], STDOUT_FILENO);

        char* program = malloc(strlen(address));
        sprintf(program, "%s", address+6);
        fprintf(stderr, "\"%s\"\n", program);

        if(execlp("/bin/sh", "/bin/sh", "-c", program, (char*)0) != 0) {
            fprintf(stderr, "Error starting profilee: %d (%s)\n", errno, strerror(errno));
        }

        exit(-1);
    }
}

int _open_socket(const char* address) {
    if(address[0] == '/' || strstr(address, "unix://") == address) {
        return _open_unix_socket(address);
    }
    else if(strstr(address, "stdio:") == address) {
        return _open_stdio_socket(address);
    }
    else {
        return _open_inet6_socket(address);
    }
}

int gdbserver_handler_init(const char* address, GDBServerHandler* handler) {
    memset(handler, 0, sizeof(GDBServerHandler));

    if(strlen(address) == 0) {
        fprintf(stderr, "No gdbserver address given\n");
        return -1;
    }

    handler->socket = _open_socket(address);

    if(handler->socket == -1) {
        return -1;
    }

    return 0;
}

static inline void _send_packet(GDBServerHandler* handler, const char* packet, size_t len) {
    uint8_t checksum = 0;

    for(size_t i = 0; i < len; ++i) {
        checksum += packet[i];
    }

    char* full_packet = malloc(len + 5);
    full_packet[0] = '$';
    memcpy(full_packet + 1, packet, len);
    full_packet[len + 1] = '#';
    snprintf(full_packet + len + 2, 3, "%02x", checksum);

    write(handler->socket, full_packet, len + 4);

    char buffer;
    do {
        read(handler->socket, &buffer, 1);
    } while(buffer != '+' && buffer != '-');

    if(buffer == '-') {
        fprintf(stderr, "Error reply to packet received");
    }
}

void gdbserver_handler_continue(GDBServerHandler* handler) {
    handler->paused = false;
    _send_packet(handler, "c", 1);
}

void gdbserver_handler_break(GDBServerHandler* handler, uint64_t addr) {
    char buffer[64];
    size_t len = snprintf(buffer, 64, "Z1,%lx,0", addr);

    _send_packet(handler, buffer, len);
}

bool gdbserver_handler_paused(GDBServerHandler* handler) {
    return handler->paused;
}

void gdbserver_handler_step(GDBServerHandler* handler) {
    handler->paused = false;
    _send_packet(handler, "s", 1);
}

void gdbserver_handler_stop(GDBServerHandler* handler) {
    handler->paused = false;
    _send_packet(handler, "k", 1);
}

void gdbserver_handle_packet(GDBServerHandler* handler) {
    if(handler->packet[0] == 'T') {
        handler->paused = true;
        _send_packet(handler, "g ", 2);
        handler->waiting_for_regs = 1;
    }
    else if(handler->waiting_for_regs && handler->packet_len >= 17 * 16) {
        char* str = malloc(17);
        memset(str, 0, 17);
        memcpy(str, handler->packet + (16 * 16), 16);

        // XXX: this seems not very elegant, but this part works.
        uint64_t res;
        sscanf(str, "%lx", &res);

        res = (((res & 0xF000000000000000) >> 60) <<  4) |
              (((res & 0x0F00000000000000) >> 56) <<  0) |
              (((res & 0x00F0000000000000) >> 52) << 12) |
              (((res & 0x000F000000000000) >> 48) <<  8) |
              (((res & 0x0000F00000000000) >> 44) << 20) |
              (((res & 0x00000F0000000000) >> 40) << 16) |
              (((res & 0x000000F000000000) >> 36) << 28) |
              (((res & 0x0000000F00000000) >> 32) << 24) |
              (((res & 0x00000000F0000000) >> 28) << 36) |
              (((res & 0x000000000F000000) >> 24) << 32) |
              (((res & 0x0000000000F00000) >> 20) << 44) |
              (((res & 0x00000000000F0000) >> 16) << 40) |
              (((res & 0x000000000000F000) >> 12) << 52) |
              (((res & 0x0000000000000F00) >>  8) << 48) |
              (((res & 0x00000000000000F0) >>  4) << 60) |
              (((res & 0x000000000000000F) >>  0) << 56);

        handler->address          = res;
        handler->waiting_for_regs = false;
    }
    else {
        char* str = malloc(handler->packet_len + 1);
        memset(str, 0, handler->packet_len + 1);
        strncpy(str, handler->packet, handler->packet_len + 1);
        fprintf(stderr, "Unknown packet received: \"%s\"\n", str);
        free(str);
    }

    handler->packet_ready = false;
    free(handler->packet);
    handler->packet = NULL;
}

void _recv_packet(GDBServerHandler* handler) {
    if(handler->packet_ready) {
        return;
    }

    char buffer[512];
    ssize_t r = 0;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(handler->socket, &rfds);
    select(handler->socket + 1, &rfds, NULL, NULL, NULL);

    if((r = recv(handler->socket, buffer, 512, MSG_DONTWAIT)) == 0) {
        fprintf(stderr, "Cannot read from socket anymore, exiting\n");
        exit(-1);
    }
    else {
        for(ssize_t i = 0; i < r; ++i) {
            char c = buffer[i];

            if(!handler->packet_started && c == '$') {
                handler->packet_started = true;
                handler->packet_len     = 0;
                continue;
            }

            if(handler->packet_started && c == '#') {
                handler->packet_checksum     = 0;
                handler->packet_checksum_ctr = 2;
                continue;
            }

            if(handler->packet_started && handler->packet_checksum_ctr > 0) {
                --handler->packet_checksum_ctr;

                char buffer[2] = { c, 0 };
                int decoded = 0;
                sscanf(buffer, "%x", &decoded);

                if(handler->packet_checksum_ctr == 1) {
                    handler->packet_checksum = decoded << 4;
                }

                if(handler->packet_checksum_ctr == 0) {
                    handler->packet_checksum |= decoded;

                    uint8_t calculated_checksum = 0;
                    for(size_t i = 0; i < handler->packet_len; ++i) {
                        calculated_checksum += handler->packet[i];
                    }

                    if(handler->packet_checksum != calculated_checksum) {
                        fprintf(stderr, "Invalid checksum: calculated %02x, got %02x\n", calculated_checksum, handler->packet_checksum);
                    }
                    else {
                        handler->packet_ready = true;
                        gdbserver_handle_packet(handler);
                    }

                    handler->packet_started = false;
                }

                continue;
            }

            ++handler->packet_len;
            handler->packet = handler->packet ? realloc(handler->packet, handler->packet_len + 1) : malloc(1);
            handler->packet[handler->packet_len - 1] = c;
        }
    }
}

void gdbserver_handler_loop(GDBServerHandler* handler) {
    _recv_packet(handler);
}
