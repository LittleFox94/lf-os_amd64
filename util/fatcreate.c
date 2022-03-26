#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>
#include <libgen.h>

static const unsigned default_sector_size = 512;
static const unsigned default_size        = 128 * 1024;
static char*    const default_label       = "LF OS Boot";

#define UNUSED(x) (void)(x)

void strupr(char* s) {
    do {
        *s = toupper(*s);
    } while(*(s++));
}

void help(char* ex) {
    fprintf(
        stderr,
        "Usage: %s [ -s <size_in_sectors> ] [ -b <bootsect_file> ] -o <outfile> <list of inputs>\n\n"

        "%s creates a new FAT32 file system containing the files and directories given as arguments.\n"
        "Directories are descended recursively.\n\n"

        "OPTIONS\n"
        "   -s size_in_sectors  Size of the filesystem in sectors, defaults to %u\n"
        "   -S sector_size      Size of a sector in bytes, defaults to %u\n"
        "   -l filesystem_label Label of the filesystem, defaults to %s\n"
        "   -b bootsect_file    File to include as boot sector code\n"
        "   -o outfile          File to store the result in\n",
        ex, ex,
        default_size, default_sector_size, default_label
    );

    exit(-1);
}

unsigned validate_sector_size(char* ex, char* opt) {
    char* validate;
    int sector_size = strtoul(opt, &validate, 10);

    // there are more valid sector sizes but I don't care right now
    if ((sector_size != 512 && sector_size != 4096) || validate == opt) {
        fprintf(stderr, "Invalid sector size given. Sector size must be either 512 or 4096.\n");
        help(ex);
    }

    return sector_size;
}

unsigned validate_size(char* ex, char* opt) {
    char* validate;
    int size = strtoul(opt, &validate, 10);

    // there are more valid sector sizes but I don't care right now
    if (size < 32 || validate == opt) {
        fprintf(stderr, "Invalid size given. We need at least 32 sectors.\n");
        help(ex);
    }

    return size;
}

FILE* validate_bootsect(char* ex, char* opt, size_t* size) {
    FILE* ret = fopen(opt, "rb");
    if(!ret) {
        fprintf(stderr, "Error opening boot sector file: %s (%d)\n", strerror(errno), errno);
        help(ex);
    }

    fseek(ret, 0, SEEK_END);
    *size = ftell(ret);
    rewind(ret);

    return ret;
}

FILE* validate_outfile(char* ex, char* opt) {
    FILE* ret = fopen(opt, "wb");
    if(!ret) {
        fprintf(stderr, "Error opening output file: %s (%d)\n", strerror(errno), errno);
        help(ex);
    }

    return ret;
}

struct superblock {
    uint8_t  jump[3];
    char     oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fats;
    uint16_t _unused1;
    uint16_t _unused2;
    uint8_t  media_description;
    uint16_t _unused4;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t sectors;
    uint32_t sectors_per_fat;
    uint16_t fat_flags;
    uint16_t fat_version_number;
    uint32_t root_start;
    uint16_t fsinfo_sector;
    uint16_t backup_superblock;
    uint8_t  _reserved1[12];
    uint8_t  drive_number;
    uint8_t  _unused5;
    uint8_t  ext_boot_signature;
    uint32_t serial;
    uint8_t  label[11];
    char     fat_version_string[8];
    uint8_t  bootsect_code[420];
    uint16_t boot_signature;
} __attribute((packed));

struct fsinfoblock {
    uint32_t fsinfo_signature1;
    uint8_t  _reserved2[480];
    uint32_t fsinfo_signature2;
    uint32_t last_known_free_clusters;
    uint32_t last_allocated_cluster;
    uint8_t  _reserved3[12];
    uint32_t fsinfo_signature3;
} __attribute__((packed));

struct directory_entry {
    union {
        struct {
            char filename[11];

            uint8_t attr_readonly:  1;
            uint8_t attr_hidden:    1;
            uint8_t attr_system:    1;
            uint8_t attr_vollabel:  1;
            uint8_t attr_directory: 1;
            uint8_t attr_archive:   1;
            uint8_t attr_device:    1;
            uint8_t attr_reserved:  1;

            uint8_t vfat_casing;
            uint8_t  create_time_10ms;

            uint16_t create_time_seconds: 5;
            uint16_t create_time_minutes: 6;
            uint16_t create_time_hours:   5;

            uint16_t create_time_day:     5;
            uint16_t create_time_month:   4;
            uint16_t create_time_year:    7;

            uint16_t last_access_time_day:     5;
            uint16_t last_access_time_month:   4;
            uint16_t last_access_time_year:    7;

            uint16_t first_cluster_hi;

            uint16_t last_modify_time_seconds: 5;
            uint16_t last_modify_time_minutes: 6;
            uint16_t last_modify_time_hours:   5;

            uint16_t last_modify_time_day:     5;
            uint16_t last_modify_time_month:   4;
            uint16_t last_modify_time_year:    7;

            uint16_t first_cluster_lo;

            uint32_t file_size;
        }__attribute__((packed));

        struct {
            uint8_t sequence:       5;
            uint8_t _reserved1:     1;
            uint8_t last_logical:   1;
            uint8_t _reserved2:     1;

            uint16_t name1[5];
            uint8_t attributes;
            uint8_t type;
            uint8_t checksum;
            uint16_t name2[6];
            uint16_t first_cluster;
            uint16_t name3[2];
        }__attribute__((packed)) lfn;
    };
};

typedef struct {
    FILE* outfile;
    struct superblock superblock;
    struct fsinfoblock infoblock;
    uint32_t* fat;
    size_t    fat_size;

    struct directory_entry* rootdir;
    size_t                  rootdir_entries;

    uint32_t clusters;
    size_t   data_start;
} context;

uint32_t find_free_cluster(context* ctx) {
    unsigned cluster = ctx->infoblock.last_allocated_cluster;

    for(; cluster < ctx->clusters + 2; cluster++) {
        if(!ctx->fat[cluster]) {
            return cluster;
        }
    }

    // didn't find a free cluster starting at the last allocated cluster,
    // retry starting from the beginning
    cluster = 0;
    for(; cluster < ctx->clusters + 2; cluster++) {
        if(!ctx->fat[cluster]) {
            return cluster;
        }
    }

    // no free cluster found
    exit(ENOSPC);
    return 0;
}

/**
 * Writes data to disk, using the parameters from the superblock and updating the fsinfoblock. Returns the
 * first cluster where the data is stored. Depends on fsinfoblock to be up-to-date.
 */
uint32_t store_data(context* ctx, void* data, size_t size, uint32_t start) {
    unsigned cluster_size = ctx->superblock.bytes_per_sector * ctx->superblock.sectors_per_cluster;
    unsigned clusters     = (size + cluster_size - 1) / cluster_size;

    uint8_t* cluster_data = malloc(cluster_size);

    unsigned first_cluster;

    if (start) {
        first_cluster = start;
    }
    else {
        first_cluster = find_free_cluster(ctx);
    }

    unsigned previous_cluster = 0;
    unsigned current_cluster  = first_cluster;

    for(unsigned i = 0; i < clusters; ++i) {
        if(!ctx->fat[current_cluster]) {
            ctx->infoblock.last_allocated_cluster = current_cluster;
            --ctx->infoblock.last_known_free_clusters;
        }

        if(!ctx->fat[current_cluster]) {
            ctx->fat[current_cluster] = 0x0FFFFFF8;
        }

        if (previous_cluster) {
            ctx->fat[previous_cluster] = current_cluster;
        }

        size_t amount = size - (i * cluster_size);
        if(amount > cluster_size) {
            amount = cluster_size;
        }

        memset(cluster_data, 0, cluster_size);
        memcpy(cluster_data, (uint8_t*)data + (i * cluster_size), amount);
        fseek(ctx->outfile, ctx->data_start + (cluster_size * (current_cluster - 2)), SEEK_SET);
        fwrite(cluster_data, cluster_size, 1, ctx->outfile);
        fflush(ctx->outfile);

        if(i < clusters - 1) {
            previous_cluster = current_cluster;

            if(start) {
                current_cluster = ctx->fat[current_cluster];
            }
            else {
                current_cluster = 0;
            }

            if(!current_cluster || current_cluster == 0xffffff8) {
                current_cluster = find_free_cluster(ctx);

                if(start) {
                    // this should not happen as it makes fragmented images and all cases should be handled
                    fprintf(stderr, "Warning: rewriting data but needing a new cluster\n");
                }
            }
        }
    }

    free(cluster_data);
    return first_cluster;
}

uint32_t store_file(context* ctx, char* path) {
    FILE* f = fopen(path, "rb");
    if(!f) {
        fprintf(stderr, "Error opening file %s for reading: %s (%d)\n", path, strerror(errno), errno);
        return 0;
    }

    uint32_t first_cluster = 0;
    uint32_t last_cluster = 0;

    size_t cluster_size = ctx->superblock.bytes_per_sector *
                          ctx->superblock.sectors_per_cluster;
    void* buffer = malloc(cluster_size);

    while(!feof(f)) {
        memset(buffer, 0, cluster_size);
        size_t r = fread(buffer, 1, cluster_size, f);
        if(!r) {
            break;
        }

        uint32_t cluster = store_data(ctx, buffer, cluster_size, 0);

        if(!first_cluster) {
            first_cluster = cluster;
        }

        if(last_cluster) {
            ctx->fat[last_cluster] = cluster;
        }

        last_cluster = cluster;
    }

    fclose(f);
    free(buffer);

    return first_cluster;
}

void write_bootsect(context* ctx, FILE* bootsect, size_t bootsect_size) {
    unsigned sectors = (bootsect_size + ctx->superblock.bytes_per_sector - 1) / ctx->superblock.bytes_per_sector;
    ctx->superblock.reserved_sectors   += sectors - 1;
    ctx->superblock.fsinfo_sector      += sectors - 1;

    fseek(ctx->outfile, sizeof(struct superblock), SEEK_SET);

    uint8_t* sector = malloc(ctx->superblock.bytes_per_sector);
    for(unsigned i = 0; i < sectors; ++i) {
        memset(sector, 0, ctx->superblock.bytes_per_sector);
        fread(sector, ctx->superblock.bytes_per_sector, 1, bootsect);

        if(!i) {
            struct superblock* bootsect = (struct superblock*)sector;
            memcpy(ctx->superblock.bootsect_code, bootsect->bootsect_code, sizeof(bootsect->bootsect_code));
            memcpy(ctx->superblock.jump, bootsect->jump, sizeof(bootsect->jump));
        }
        else {

            fwrite(sector, ctx->superblock.bytes_per_sector, 1, ctx->outfile);
        }
    }

    fclose(bootsect);
    free(sector);
}

void initialize_fat(context* ctx) {
    size_t clusters = 0, last_clusters = 0;

    // the FAT needs some sectors, reducing the size of the data area. How much it needs depends
    // on the number of clusters, which again depends on the size of the FAT... calculate FAT size
    // and available clusters until it didn't change in the loop.
    // There surely is a better way to do this -- Mara @LittleFox94 Grosch, 2022-03-05
    do {
        last_clusters = clusters;

        clusters =
            (
                ctx->superblock.sectors -
                ctx->superblock.reserved_sectors -
                (
                    ctx->superblock.sectors_per_fat *
                    2
                )
            )
            / ctx->superblock.sectors_per_cluster;

        // +2: first two entries in FAT are special
        ctx->fat_size = sizeof(uint32_t) * (clusters + 2);

        ctx->superblock.sectors_per_fat =
            (
                ctx->fat_size +
                ctx->superblock.bytes_per_sector -
                1
            ) /
            ctx->superblock.bytes_per_sector;

    } while(clusters != last_clusters);

    size_t alloc_size = ctx->fat_size + (ctx->superblock.bytes_per_sector - 1);
    ctx->fat = malloc(alloc_size);
    memset(ctx->fat, 0, alloc_size);
    ctx->fat[0] = 0xFFFFFFF8;
    ctx->fat[1] = 0x0FFFFFFF;

    ctx->infoblock.last_known_free_clusters = clusters;
    ctx->clusters = clusters;
}

void fatify_name(char** dst, const char* fullname, struct directory_entry* dir, size_t dirlen) {
    char* ret = malloc(12);
    memset(ret, ' ', 11);
    ret[11] = 0;

    size_t namelen = strlen(fullname);

    char* ext = strchr(fullname, '.');
    if(ext) {
        ext++;
        size_t extlen = strlen(ext);
        namelen -= extlen + 1;

        if (extlen <= 3) {
            memcpy(ret + 8, ext, extlen);
        }
    }
    else {
        ext = "   ";
    }

    if (namelen <= 8) {
        memcpy(ret, fullname, namelen);
    }
    else {
        int collision = 1;
        int cnt = 1;
        int prefixlen = 6;

        while(collision) {
            collision = 0;

            for(int i = 10; i <= cnt && prefixlen > 0; i *= 10) {
                prefixlen--;
            }

            // our compiler tries to helpfully warn us about the string being truncated,
            // but this is exactly what we want to do here - truncate the file name to 8.3
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
            snprintf(ret, 12, "%.*s~%d%3s", prefixlen, fullname, cnt, ext);
#pragma GCC diagnostic pop

            strupr(ret);

            for(size_t i = 0; i < dirlen; ++i) {
                if(memcmp(dir[i].filename, ret, 11) == 0) {
                    ++cnt;
                    collision = 1;
                    break;
                }
            }
        }
    }

    strupr(ret);

    *dst = ret;
}

void stat2fat(struct directory_entry* entry, struct stat* stat) {
    struct tm ctime;
    localtime_r(&stat->st_ctim.tv_sec, &ctime);
    entry->create_time_seconds = ctime.tm_sec / 2;
    entry->create_time_minutes = ctime.tm_min;
    entry->create_time_hours   = ctime.tm_hour;
    entry->create_time_day     = ctime.tm_mday;
    entry->create_time_month   = ctime.tm_mon + 1;
    entry->create_time_year    = ctime.tm_year - 80;

    struct tm atime;
    localtime_r(&stat->st_atim.tv_sec, &atime);
    entry->last_access_time_day     = atime.tm_mday;
    entry->last_access_time_month   = atime.tm_mon + 1;
    entry->last_access_time_year    = atime.tm_year - 80;

    struct tm mtime;
    localtime_r(&stat->st_mtim.tv_sec, &mtime);
    entry->last_modify_time_seconds = mtime.tm_sec / 2;
    entry->last_modify_time_minutes = mtime.tm_min;
    entry->last_modify_time_hours   = mtime.tm_hour;
    entry->last_modify_time_day     = mtime.tm_mday;
    entry->last_modify_time_month   = mtime.tm_mon + 1;
    entry->last_modify_time_year    = mtime.tm_year - 80;

    entry->file_size = stat->st_size;
}

size_t lfn_entries(char* name) {
    size_t len = strlen(name);
    return (len + 12) / 13;
}

uint8_t lfn_checksum(struct directory_entry* entry) {
    uint8_t ret = 0;
    for(int i = 0; i < 11; ++i) {
        ret = ((ret & 1) << 7) + (ret >> 1) + entry->filename[i];
    }

    return ret;
}

void make_lfn(char* name, struct directory_entry* dest, size_t max_entries) {
    size_t num_entries = lfn_entries(name);
    if(num_entries != max_entries) {
        fprintf(stderr, "max_entries called with value not equal to number of required entries (%lu != %lu)\n", num_entries, max_entries);
        exit(-1);
    }

    struct directory_entry* entry = dest + max_entries - 1;
    size_t ei = 0;

    for(int i = 0; i < strlen(name); ++i) {
        if(ei >= 13) {
            ei = 0;

            entry->lfn.last_logical = 0;
            entry--;

            if(entry < dest) {
                fprintf(stderr, "more than max_entries LFN entries!\n");
                exit(-1);
            }
        }

        if(!entry->lfn.sequence) {
            entry->lfn.checksum = lfn_checksum(dest+max_entries);
            entry->lfn.attributes = 0x0F;
            entry->lfn.sequence = max_entries - (entry - dest);
            entry->lfn.last_logical = 1;
        }

        wchar_t c = name[i];

        if(ei < 5) {
            entry->lfn.name1[ei] = c;
        }
        else if(ei < 11) {
            entry->lfn.name2[ei - 5] = c;
        }
        else if(ei < 13) {
            entry->lfn.name3[ei - 11] = c;
        }

        ei++;
    }
}

uint32_t store_directory(context* ctx, uint32_t parent_dir_start, const char* src) {
    size_t num_entries = 2;

    DIR* dir = opendir(src);
    struct dirent* cur;
    while((cur = readdir(dir))) {
        if(cur->d_name[0] == '.') {
            continue;
        }

        num_entries += 1 + lfn_entries(cur->d_name);
    }

    struct directory_entry* direntries = malloc(sizeof(struct directory_entry) * num_entries);
    memset(direntries, 0, sizeof(struct directory_entry) * num_entries);

    unsigned free_direntry = 0;
    direntries[free_direntry].attr_directory = 1;
    memcpy(direntries[free_direntry].filename, ".          ", 11);
    free_direntry++;

    direntries[free_direntry].attr_directory = 1;
    direntries[free_direntry].first_cluster_lo = parent_dir_start & 0xFFFF;
    direntries[free_direntry].first_cluster_hi = parent_dir_start >> 16;
    memcpy(direntries[free_direntry].filename, "..         ", 11);
    free_direntry++;

    uint32_t dircluster = store_data(ctx, direntries, sizeof(struct directory_entry) * num_entries, 0);
    direntries[0].first_cluster_lo = dircluster & 0xFFFF;
    direntries[0].first_cluster_hi = dircluster >> 16;
    store_data(ctx, direntries, sizeof(struct directory_entry) * num_entries, dircluster);

    rewinddir(dir);

    while((cur = readdir(dir))) {
        if(cur->d_name[0] == '.') {
            continue;
        }

        size_t lfn = lfn_entries(cur->d_name);

        char* fullpath = malloc(strlen(src) + strlen(cur->d_name) + 2);
        sprintf(fullpath, "%s/%s", src, cur->d_name);

        struct stat s;
        if(stat(fullpath, &s)) {
            fprintf(stderr, "error processing \"%s\": %s (%d)\n", fullpath, strerror(errno), errno);
            continue;
        }

        if(!(s.st_mode & (S_IFREG | S_IFDIR))) {
            fprintf(stderr, "error processing \"%s\": invalid file type (%d)\n", fullpath, s.st_mode & S_IFMT);
            continue;
        }

        struct directory_entry* entry = direntries + free_direntry + lfn;

        stat2fat(entry, &s);

        char* fatname;
        fatify_name(&fatname, cur->d_name, direntries, num_entries);
        memcpy(entry->filename, fatname, 11);
        free(fatname);

        make_lfn(cur->d_name, direntries + free_direntry, lfn);

        uint32_t cluster = 0;

        if(s.st_mode & S_IFDIR) {
            entry->attr_directory = 1;
            entry->file_size = 0;
            cluster = store_directory(ctx, dircluster, fullpath);
        }
        else if(s.st_mode &  S_IFREG) {
            cluster = store_file(ctx, fullpath);
        }

        if(!cluster && s.st_size && s.st_mode & S_IFREG) {
            fprintf(stderr, "error processing \"%s\": not stored on image\n", fullpath);
            continue;
        }

        entry->first_cluster_lo = cluster & 0xFFFF;
        entry->first_cluster_hi = cluster >> 16;

        free_direntry += 1 + lfn;

        free(fullpath);
    }

    closedir(dir);

    store_data(ctx, direntries, sizeof(struct directory_entry) * num_entries, dircluster);
    free(direntries);

    return dircluster;
}

void process_input(context* ctx, char* input) {
    struct stat s;
    if(stat(input, &s)) {
        fprintf(stderr, "error processing \"%s\": %s (%d)\n", input, strerror(errno), errno);
        return;
    }

    if(!(s.st_mode & (S_IFREG | S_IFDIR))) {
        fprintf(stderr, "error processing \"%s\": invalid file type (%d)\n", input, s.st_mode & S_IFMT);
        return;
    }

    char* name = basename(input);

    size_t lfn = lfn_entries(name);
    ctx->rootdir_entries += 1 + lfn;
    ctx->rootdir = realloc(ctx->rootdir, ctx->rootdir_entries * sizeof(struct directory_entry));
    memset(ctx->rootdir + ctx->rootdir_entries - lfn - 1, 0, sizeof(struct directory_entry) * (lfn + 1));

    struct directory_entry* entry = ctx->rootdir + ctx->rootdir_entries - 1;

    stat2fat(entry, &s);

    char* fatname;
    fatify_name(&fatname, name, ctx->rootdir, ctx->rootdir_entries);
    memcpy(entry->filename, fatname, 11);
    free(fatname);

    make_lfn(name, entry - lfn, lfn);

    uint32_t cluster = 0;

    if(s.st_mode & S_IFDIR) {
        entry->attr_directory = 1;
        entry->file_size = 0;
        cluster = store_directory(ctx, 0, input);
    }
    else {
        cluster = store_file(ctx, input);
    }

    if(!cluster && s.st_size && s.st_mode & S_IFREG) {
        fprintf(stderr, "error processing \"%s\": not stored on image\n", input);
        return;
    }

    entry->first_cluster_lo = cluster & 0xFFFF;
    entry->first_cluster_hi = cluster >> 16;
}

int main(int argc, char* argv[]) {
    assert(sizeof(struct superblock) == 512);
    assert(sizeof(struct fsinfoblock) == 512);
    assert(sizeof(struct directory_entry) == 32);

    unsigned sector_size = default_sector_size;
    unsigned size        = default_size;
    char* label          = default_label;

    FILE* bootsect       = 0;
    size_t bootsect_size = 0;

    FILE* outfile        = 0;

    int c;
    while((c = getopt(argc, argv, "hS:s:l:b:o:")) != -1) {
        switch(c) {
        case 'S':
            sector_size = validate_sector_size(argv[0], optarg);
            break;
        case 's':
            size = validate_size(argv[0], optarg);
            break;
        case 'l':
            label = optarg;
            break;
        case 'b':
            bootsect = validate_bootsect(argv[0], optarg, &bootsect_size);
            break;
        case 'o':
            outfile = validate_outfile(argv[0], optarg);
            break;
        case 'h':
        default:
            help(argv[0]);
        }
    }

    if(!outfile) {
        fprintf(stderr, "No output file given\n");
        help(argv[0]);
    }

    context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.outfile = outfile;

    memcpy(ctx.superblock.jump, "\xEB\x3C\x90", 3);
    memcpy(ctx.superblock.fat_version_string, "FAT32   ", 8);
    ctx.infoblock.fsinfo_signature1 = 0x41615252;
    ctx.infoblock.fsinfo_signature2 = 0x61417272;
    ctx.infoblock.fsinfo_signature3 = 0xaa550000;

    memcpy(ctx.superblock.oem, "LF OS   ", 8);

    ctx.superblock.bytes_per_sector = sector_size;
    ctx.superblock.sectors_per_cluster = 1;
    ctx.superblock.sectors_per_track = 255;
    ctx.superblock.heads = 1;
    ctx.superblock.reserved_sectors = 2; // boot sector and fs info block
    ctx.superblock.fats = 2;
    ctx.superblock.media_description = 0xF8;
    ctx.superblock.sectors = size;
    ctx.superblock.root_start = 2;
    ctx.superblock.fsinfo_sector = 1;
    ctx.superblock.backup_superblock = 0;
    ctx.superblock.ext_boot_signature = 0x29;
    ctx.superblock.serial = 0x051f3713;
    ctx.superblock.boot_signature = 0xAA55;

    ctx.infoblock.last_allocated_cluster = 0x1;

    if(bootsect) {
        write_bootsect(&ctx, bootsect, bootsect_size);
    }

    initialize_fat(&ctx);

    ctx.data_start =
        (
            ctx.superblock.bytes_per_sector *
            (
                ctx.superblock.reserved_sectors +
                (ctx.superblock.sectors_per_fat * 2)
            )
        );

    ctx.rootdir_entries = 1;
    ctx.rootdir = malloc(ctx.rootdir_entries * sizeof(struct directory_entry));
    memset(ctx.rootdir, 0, sizeof(struct directory_entry) * ctx.rootdir_entries);

    char fslabel[12];
    snprintf(fslabel, 12, "%-11s", label);

    memcpy(ctx.superblock.label,    fslabel, 11);
    memcpy(ctx.rootdir[0].filename, fslabel, 11);
    ctx.rootdir[0].attr_vollabel = 1;
    ctx.rootdir[0].file_size = 0;

    for(int i = optind; i < argc; i++) {
        process_input(&ctx, argv[i]);
    }

    ctx.superblock.root_start = store_data(&ctx, ctx.rootdir, ctx.rootdir_entries * sizeof(struct directory_entry), 0);

    rewind(outfile);

    fwrite(&ctx.superblock, sizeof(struct superblock), 1, outfile);

    fseek(outfile, (sector_size * ctx.superblock.fsinfo_sector), SEEK_SET);
    fwrite(&ctx.infoblock,  sizeof(struct fsinfoblock),  1, outfile);

    fwrite(ctx.fat, ctx.superblock.bytes_per_sector, ctx.superblock.sectors_per_fat, outfile);
    fwrite(ctx.fat, ctx.superblock.bytes_per_sector, ctx.superblock.sectors_per_fat, outfile);

    fseek(outfile, (sector_size * size) - 1, SEEK_SET);
    fwrite("\0", 1, 1, outfile);
    fclose(outfile);

    free(ctx.fat);
    free(ctx.rootdir);
}
