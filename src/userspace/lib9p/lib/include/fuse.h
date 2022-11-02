/**
 * What's this; a crossover episode?!
 * Yes. Yes, this is a reimplementation of the libfuse interface. Everything available is by-spec
 * as it is in libfuse, but some things might just not be there (yet).
 *
 * Documentation of libfuse: https://libfuse.github.io/doxygen/structfuse__operations.html
 */

#pragma once

#if __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

#include <sys/stat.h>

struct fuse_conn_info {
    size_t max_read;
    size_t max_write;
};

struct fuse_context {
    void* private_data;
};

struct fuse_config {
    int kernel_cache; // only provided for compat, no effect
};

enum fuse_readdir_flags {
    FUSE_READDIR_PLUS = (1 << 0)
};

enum fuse_fill_dir_flags {
    FUSE_FILL_DIR_PLUS = (1 << 1)
};

struct fuse_file_info {
    int      flags;
    uint64_t fh;
};

typedef int(*fuse_fill_dir_t)(void* buf, const char* name, const struct stat* stbuf, off_t off, enum fuse_fill_dir_flags flags);

struct fuse_operations {
    void*(*init)(struct fuse_conn_info*, struct fuse_config*);
    void(*destroy)(void*);

    int(*open)   (const char*,                             struct fuse_file_info*);
    int(*release)(const char*,                             struct fuse_file_info*);
    int(*read)   (const char*,       char*, size_t, off_t, struct fuse_file_info*);
    int(*write)  (const char*, const char*, size_t, off_t, struct fuse_file_info*);

    int(*opendir)   (const char*,                                struct fuse_file_info*);
    int(*releasedir)(const char*,                                struct fuse_file_info*);
    int(*readdir)   (const char*, void*, fuse_fill_dir_t, off_t, struct fuse_file_info*, enum fuse_readdir_flags);

    int(*getattr) (const char*, struct stat*, struct fuse_file_info*);
};

struct fuse_args {
    int    argc;
    char** argv ;
    int    allocated;
};

#define FUSE_ARGS_INIT(argc, argv) (struct fuse_args){ .allocated = 0, .argc = argc, .argv = argv }

struct fuse_opt {
    const char*   templ;
    unsigned long offset;
    int           value;
};

#define FUSE_OPT_END (struct fuse_opt){ .templ = 0, .offset = 0, .value = 0 }
#define fuse_opt_parse(...)     0
#define fuse_opt_add_arg(...)   0
#define fuse_opt_free_args(...) 0

int fuse_main(int argc, char* argv[], const struct fuse_operations* ops, void* private_data);

struct fuse_context* fuse_get_context();

#if __cplusplus
}
#endif
