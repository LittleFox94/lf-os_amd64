#ifndef _TRACE_WRITER_INCLUDED
#define _TRACE_WRITER_INCLUDED

#include <stdint.h>
#include <unistd.h>

typedef struct {
    FILE* trace_file;
} TraceWriter;

int trace_writer_init(const char* trace_file_path, TraceWriter* writer);
void trace_writer_write(TraceWriter* writer, uint64_t addr);

#endif
