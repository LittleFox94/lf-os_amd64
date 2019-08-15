#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "trace_reader.h"

int trace_reader_init(const char* trace_file_path, TraceReader* reader) {
    if((reader->file = fopen(trace_file_path, "rb")) == NULL) {
        fprintf(stderr, "Error opening trace file for reading: %d (%s)", errno, strerror(errno));
        return -1;
    }

    return 0;
}

bool trace_reader_eof(TraceReader* reader) {
    return feof(reader->file);
}

uint64_t trace_reader_read(TraceReader* reader) {
    uint64_t ret = 0;
    while(!fscanf(reader->file, "%lx", &ret));
    return ret;
}
