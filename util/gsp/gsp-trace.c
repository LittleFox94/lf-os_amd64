#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "image_reader.h"
#include "gdbserver_handler.h"
#include "trace_writer.h"

void usage(char* argv0) {
    fprintf(stderr, "Usage: %s -hc -i program.elf [ -o /dev/stdout ] [ -g localhost:1234 ]\n\n", argv0);
    fprintf(stderr, "Parameters\n");
    fprintf(stderr, "   -i $image_file   Compiled program to read for symbols\n");
    fprintf(stderr, "   -o $output_file  Where to write the trace, defaults to /dev/stdout\n");
    fprintf(stderr, "   -g $gdbserver    Where to reach the gdbserver, defaults to localhost:1234\n");
    fprintf(stderr, "   -s $start_symbol Where to start profiling (defaults to entry point of image)\n");
    fprintf(stderr, "   -S $stop_symbol  Where to stop profiling (defaults to never)\n");

    fprintf(stderr, "\nFlags\n");
    fprintf(stderr, "   -h               Give this help and exit with status -1\n");
    fprintf(stderr, "   -c               Issue continue to gdbserver after initialization\n");
}

int main(int argc, char* argv[]) {
    char* image  = NULL;
    char* output = "/dev/stdout";

    char* gdb_address    = "localhost";
    bool  issue_continue = false;

    char* start_symbol = NULL;
    char* stop_symbol  = NULL;

    int opt = 0;
    while((opt = getopt(argc, argv, "i:o:g:hcs:S:")) != -1) {
        switch(opt) {
            case 'i':
                image = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case 'g':
                gdb_address = optarg;
                break;
            case 'c':
                issue_continue = true;
                break;
            case 's':
                start_symbol = optarg;
                break;
            case 'S':
                stop_symbol = optarg;
                break;
            case 'h':
            default:
                usage(argv[0]);
                return -1;
        }
    }

    if(!image) {
        fprintf(stderr, "Missing image file\n");
        usage(argv[0]);
        return -1;
    }

    ImageReader reader;
    if(image_reader_init(image, &reader) != 0) {
        fprintf(stderr, "Error reading image file\n");
        return -1;
    }

    GDBServerHandler gdbserver_handler;
    if(gdbserver_handler_init(gdb_address, &gdbserver_handler) != 0) {
        fprintf(stderr, "Error initializing gdbserver handler\n");
        return -1;
    }

    TraceWriter writer;
    if(trace_writer_init(output, &writer) != 0) {
        fprintf(stderr, "Error initializing trace file writer\n");
        return -1;
    }

    if(start_symbol != NULL) {
        bool found = false;
        for(size_t i = 0;i < reader.num_symbols; ++i) {
            if(strcmp(reader.symbols[i].name, start_symbol) == 0) {
                gdbserver_handler_break(&gdbserver_handler, reader.symbols[i].address);
                found = true;
            }
        }

        if(!found) {
            start_symbol = NULL;
        }
    }

    if(start_symbol == NULL) {
        gdbserver_handler_break(&gdbserver_handler, reader.entry);
    }

    uint64_t stop_symbol_address = 0;
    if(stop_symbol != NULL) {
        bool found = false;
        for(size_t i = 0;i < reader.num_symbols; ++i) {
            if(strcmp(reader.symbols[i].name, stop_symbol) == 0) {
                stop_symbol_address = reader.symbols[i].address;
                found = true;
            }
        }

        if(!found) {
            stop_symbol = NULL;
        }
    }

    if(issue_continue) {
        gdbserver_handler_continue(&gdbserver_handler);
    }

    while(!feof(stdin)) {
        gdbserver_handler_loop(&gdbserver_handler);

        if(gdbserver_handler_paused(&gdbserver_handler) && !gdbserver_handler.waiting_for_regs) {
            trace_writer_write(&writer, gdbserver_handler.address);

            if(stop_symbol != NULL && gdbserver_handler.address == stop_symbol_address) {
                gdbserver_handler_stop(&gdbserver_handler);
                return 0;
            }

            gdbserver_handler_step(&gdbserver_handler);
        }
    }

    return 0;
}
