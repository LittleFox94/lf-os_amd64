#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "trace_writer.h"

int trace_writer_init(const char* trace_file_path, TraceWriter* writer) {
    if((writer->trace_file = fopen(trace_file_path, "wb")) == NULL) {
        fprintf(stderr, "Error opening trace file for writing: %d (%s)", errno, strerror(errno));
        return -1;
    }

    return 0;
}

void trace_writer_write(TraceWriter* writer, uint64_t addr) {
    fprintf(writer->trace_file, "%016lx\n", addr);
}
