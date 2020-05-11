#include <lfostest.h>

#include "../../src/kernel/string.c"

int main() {
    bool error = false;
    char buffer[20];

    eq(13, strlen("Hello, world!"), "simple strlen correct");
    eq(2, strlen("\t\t"), "control characters strlen correct");
    eq(0, strlen("\0"), "double empty strlen correct");
    eq(0, strlen(0), "null pointer strlen correct");

    eq(13, sputs(buffer, 20, "Hello, world!", 13), "sputs return correct");
    eq(2, sputi(buffer, 20, 42, 10), "sputi return correct");
    eq(2, sputui(buffer, 20, 23, 10), "sputui return correct");
    eq(4, sputbytes(buffer, 20, 4096), "sputbytes return correct");

    return error ? 1 : 0;
}
