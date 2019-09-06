#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

void help(char* argv0) {
    fprintf(stderr, "%s -i file [ -o file.embed.c ]\n", argv0);

    fprintf(stderr, "\nPARAMETERS\n");
    fprintf(stderr, "  -i inputFile   Read file and transform to embedded C code\n");
    fprintf(stderr, "  -o outputFile  Output file, defaults to $inputFile.embed.c\n");
    fprintf(stderr, "  -m mode        File mode. Not yet implemented, will allow loading image files\n");

    fprintf(stderr, "\nFLAGS\n");
    fprintf(stderr, "  -h             Show this help\n");

    fprintf(stderr, "\nMODES (-m)\n");
    fprintf(stderr, "  raw (default)  Embed the file as is\n");

    exit(-1);
}

void write_header(FILE* file, size_t len) {
    fprintf(file, "/* automatically generated file */\n");
    fprintf(file, "/* changes will be lost! */\n\n");

    fprintf(file, "#include <stdint.h>\n\n");

    fprintf(file, "const uint64_t dataLen = %lu;\n", len);
    fprintf(file, "const uint8_t  data[]  = {\n");
}

void write_footer(FILE* file) {
    fseek(file, -3, SEEK_END);
    fprintf(file, "\n};\n");
}

void embed_raw(const char* input, const char* output) {
    FILE* inputFile = NULL;
    if((inputFile = fopen(input, "rb")) == NULL) {
        fprintf(stderr, "Error opening input file for reading: %d (%s)\n", errno, strerror(errno));
        exit(-1);
    }

    FILE* outputFile = NULL;
    if((outputFile = fopen(output, "w")) == NULL) {
        fprintf(stderr, "Error opening output file for writing: %d (%s)\n", errno, strerror(errno));
        exit(-1);
    }

    fseeko(inputFile, 0, SEEK_END);
    size_t len = ftello(inputFile);
    fseeko(inputFile, 0, SEEK_SET);

    write_header(outputFile, len);

    const size_t bufferSize = 16;

    uint8_t* buffer = malloc(bufferSize);
    size_t read;

    while((read = fread(buffer, 1, bufferSize, inputFile)) > 0) {
        fprintf(outputFile, "    ");

        for(size_t i = 0; i < read; ++i) {
            fprintf(outputFile, "0x%02x, ", (unsigned)buffer[i]);
        }

        fprintf(outputFile, "\n");
    }

    write_footer(outputFile);
}

int main(int argc, char* argv[]) {
    char* input    = NULL;
    char* output   = NULL;
    char* mode     = "raw";

    char opt;
    while((opt = getopt(argc, argv, "hi:o:m:")) != -1) {
        switch(opt) {
            case 'i':
                input = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case 'm':
                mode = optarg;
                break;
            default:
            case 'h':
                help(argv[0]);
                break;
        }
    }

    if(input == NULL) {
        help(argv[0]);
    }

    if(output == NULL) {
        output = malloc(strlen(input) + strlen(".embed.c"));
        sprintf(output, "%s.embed.c", input);
    }

    if(strcmp(mode, "raw") == 0) {
        embed_raw(input, output);
    }
    else {
        help(argv[0]);
    }

    return 0;
}
