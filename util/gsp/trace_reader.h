#ifndef _TRACE_READER_H_INCLUDED
#define _TRACE_READER_H_INCLUDED

#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    FILE* file;
} TraceReader;

int trace_reader_init(const char* file, TraceReader* reader);
bool trace_reader_eof(TraceReader* reader);
uint64_t trace_reader_read(TraceReader* reader);

#endif
