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

int _protocol(const char* address) {
    if(address[0] == '/')                     return AF_UNIX;
    if(address[0] == '[')                     return AF_INET6;
    if(strstr(address, "unix://") == address) return AF_UNIX;

    return AF_INET6;
}

const char* _unix_path(const char* address) {
    if(address[0] == '/')                     return address;
    if(strstr(address, "unix://") == address) return address + 7;

    fprintf(stderr, "Unable to get unix socket path from address '%s'\n", address);
    return NULL;
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

int gdbserver_handler_init(const char* address, GDBServerHandler* handler) {
    memset(handler, 0, sizeof(GDBServerHandler));

    if(strlen(address) == 0) {
        fprintf(stderr, "No gdbserver address given\n");
        return -1;
    }

    int protocol = _protocol(address);

    switch(protocol) {
        case AF_UNIX:
            {
                struct sockaddr_un pa;
                pa.sun_family = AF_UNIX;
                strncpy(pa.sun_path, _unix_path(address), sizeof(pa.sun_path)  -1);

                if(pa.sun_path == NULL) {
                    return -1;
                }

                if((handler->socket = socket(protocol, SOCK_STREAM, 0)) == -1) {
                    fprintf(stderr, "Error opening socket: %d (%s)\n", errno, strerror(errno));
                    return -1;
                }

                if(connect(handler->socket, (struct sockaddr*)&pa, sizeof(pa)) == -1) {
                    fprintf(stderr, "Error connecting to gdbserver: %d (%s)\n", errno, strerror(errno));
                    return -1;
                }

            }
            break;
        case AF_INET6:
            {
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
                    if((handler->socket = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) == -1) {
                        continue;
                    }

                    if(connect(handler->socket, r->ai_addr, r->ai_addrlen) != -1) {
                        break; /* success */
                    }

                    close(handler->socket);
                    handler->socket = -1;
                }

                if(handler->socket == -1) {
                    fprintf(stderr, "Cannot connect to tcp socket, no address found\n");
                    return -1;
                }

                freeaddrinfo(result);
            }
            break;
        default:
            fprintf(stderr, "I understood the address but it's not yet implemented\n");
            return -1;
            break;
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
        fprintf(stderr, "Error reply to packet received\n");
    }
}

void _recv_packet(GDBServerHandler* handler) {
    if(handler->packet_ready) {
        return;
    }

    char c;

    if(recv(handler->socket, &c, 1, MSG_DONTWAIT) <= 0) { 
        if(errno != EAGAIN && errno != EWOULDBLOCK) {
            fprintf(stderr, "Cannot read from socket anymore, exiting\n");
            exit(-1);
        } else {
            return;
        }
    }
    else {
        if(!handler->packet_started && c == '$') {
            handler->packet_started = true;
            handler->packet_len     = 0;
            return;
        }

        if(handler->packet_started && c == '#') {
            handler->packet_checksum     = 0;
            handler->packet_checksum_ctr = 2;
            return;
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
                }

                handler->packet_started = false;
            }

            return;
        }

        ++handler->packet_len;
        handler->packet = handler->packet ? realloc(handler->packet, handler->packet_len + 1) : malloc(1);
        handler->packet[handler->packet_len - 1] = c;
    }

    return;
}

void gdbserver_handler_continue(GDBServerHandler* handler) {
    handler->paused = false;
    _send_packet(handler, "C ", 2);
}

void gdbserver_handler_break(GDBServerHandler* handler, uint64_t addr) {
    char buffer[64];
    size_t len = snprintf(buffer, 64, "Z1 %lx", addr);

    _send_packet(handler, buffer, len);
}

bool gdbserver_handler_paused(GDBServerHandler* handler) {
    return handler->paused;
}

void gdbserver_handler_step(GDBServerHandler* handler) {
    handler->paused = false;
    _send_packet(handler, "s", 1);
}

void gdbserver_handler_loop(GDBServerHandler* handler) {
    _recv_packet(handler);

    if(handler->packet_len && handler->packet_ready) {
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
}
