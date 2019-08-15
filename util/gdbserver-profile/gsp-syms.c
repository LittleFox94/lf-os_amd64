#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "image_reader.h"
#include "trace_reader.h"

void usage(char* argv0) {
    fprintf(stderr, "Usage: %s -hc -i program.elf -t program.trace [ -o program.trace.syms ]\n\n", argv0);
    fprintf(stderr, "Parameters\n");
    fprintf(stderr, "   -i $image_file   Compiled program to read for symbols\n");
    fprintf(stderr, "   -t $trace_file   File created by gsp-trace\n");
    fprintf(stderr, "   -o $output_file  Where to write the trace with symbols, defaults to $trace_file.syms\n");

    fprintf(stderr, "\nFlags\n");
    fprintf(stderr, "   -h               Give this help and exit with status -1\n");
}

int main(int argc, char* argv[]) {
    char* image      = NULL;
    char* trace_file = NULL;
    char* output     = NULL;

    int opt = 0;
    while((opt = getopt(argc, argv, "i:o:t:h")) != -1) {
        switch(opt) {
            case 'i':
                image = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case 't':
                trace_file = optarg;
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

    if(!trace_file) {
        fprintf(stderr, "Missing trace file file\n");
        usage(argv[0]);
        return -1;
    }

    if(!output) {
        output = malloc(strlen(trace_file) + 6);
        snprintf(output, strlen(trace_file) + 6, "%s.syms", trace_file);
    }

    ImageReader image_reader;
    if(image_reader_init(image, &image_reader) != 0) {
        fprintf(stderr, "Error reading image file\n");
        return -1;
    }

    TraceReader trace_reader;
    if(trace_reader_init(trace_file, &trace_reader) != 0) {
        fprintf(stderr, "Error initializing trace file reader\n");
        return -1;
    }

    FILE* out_file = fopen(output, "w");

    while(!trace_reader_eof(&trace_reader)) {
        uint64_t addr = trace_reader_read(&trace_reader);

        Symbol* symbol;
        if((symbol = image_reader_get_symbol(&image_reader, addr)) != NULL) {
            fprintf(out_file, "%s", symbol->name);

            if(addr > symbol->address) {
                fprintf(out_file, "(+0x%lx)", addr - symbol->address);
            }

            fprintf(out_file, "\n");
        }
    }

    return 0;
}
