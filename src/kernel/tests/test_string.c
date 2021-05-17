#include <lfostest.h>
#include <string.c>

__attribute__ ((visibility ("default"))) void testmain(TestUtils* t) {
    char buffer[20];

    t->eq_size_t(13, strlen("Hello, world!"), "simple strlen correct");
    t->eq_size_t(2,  strlen("\t\t"), "control characters strlen correct");
    t->eq_size_t(0,  strlen("\0"), "double empty strlen correct");
    t->eq_size_t(0,  strlen(0), "null pointer strlen correct");

    t->eq_size_t(13, sputs(buffer, 20, "Hello, world!", 13), "sputs return correct");
    t->eq_size_t(2,  sputi(buffer, 20, 42, 10), "sputi return correct");
    t->eq_size_t(2,  sputui(buffer, 20, 23, 10), "sputui return correct");
    t->eq_size_t(4,  sputbytes(buffer, 20, 4096), "sputbytes return correct");

    t->lt_int(0,     strcmp("bar", "foo"), "strcmp returns negative correctly");
    t->gt_int(0,     strcmp("foo", "bar"), "strcmp returns positive correctly");
    t->eq_int(0,     strcmp("foo", "foo"), "strcmp returns zero correctly");

    t->lt_int(0, strcasecmp("bar", "foo"), "strcasecmp returns negative correctly");
    t->gt_int(0, strcasecmp("foo", "bar"), "strcasecmp returns positive correctly");
    t->eq_int(0, strcasecmp("foo", "foo"), "strcasecmp returns zero correctly");
    t->eq_int(0, strcasecmp("FoO", "foo"), "strcasecmp returns zero correctly");
}
