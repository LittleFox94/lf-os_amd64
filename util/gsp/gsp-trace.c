#include <stdio.h>
#include <stdbool.h>
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

    fprintf(stderr, "\nFlags\n");
    fprintf(stderr, "   -h               Give this help and exit with status -1\n");
    fprintf(stderr, "   -c               Issue continue to gdbserver after initialization\n");
}

int main(int argc, char* argv[]) {
    char* image  = NULL;
    char* output = "/dev/stdout";

    char* gdb_address    = "localhost";
    bool  issue_continue = false;

    int opt = 0;
    while((opt = getopt(argc, argv, "i:o:g:hc")) != -1) {
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

    gdbserver_handler_break(&gdbserver_handler, reader.entry);

    if(issue_continue) {
        gdbserver_handler_continue(&gdbserver_handler);
    }

    while(!feof(stdin)) {
        gdbserver_handler_loop(&gdbserver_handler);

        if(gdbserver_handler_paused(&gdbserver_handler) && !gdbserver_handler.waiting_for_regs) {
            trace_writer_write(&writer, gdbserver_handler.address);
            gdbserver_handler_step(&gdbserver_handler);
        }
    }

    return 0;
}
