#include "lfostest.h"

#include "../../src/kernel/tpa.c"

int main() {
    bool error = false;

    tpa_t* tpa = tpa_new(malloc, free, sizeof(uint64_t), 4096, 0);

    eq(4096, tpa_size(tpa), "Size of TPA with no entries correct");
    eq(0, tpa_entries(tpa), "Entrycount of TPA with no entries correct");
    eq(-1, tpa_length(tpa), "Length of TPA with no entries correct");

    uint64_t val = 23;
    tpa_set(tpa, 0x1337, &val);
    eq(8192, tpa_size(tpa), "Size of TPA with a single entry correct");
    eq(1, tpa_entries(tpa), "Entrycount of TPA with a single entry correct");
    eq(tpa_entries_per_page(tpa) * 20, tpa_length(tpa), "Length of TPA with a single entry correct");

    void* entry = tpa_get(tpa, 0x1337);
    ne(0, entry, "Entry returned");
    eq(23, *(uint64_t*)entry, "Correct entry value");

    void* non_entry = tpa_get(tpa, 0x1338);
    eq(0, non_entry, "Not existing entry returns 0");

    tpa_set(tpa, 0x1337, 0);
    eq(4096, tpa_size(tpa), "Size of TPA with all entries deleted");
    eq(0, tpa_entries(tpa), "Entrycount of TPA with all entries deleted");
    eq(-1, tpa_length(tpa), "Length of TPA with all entries deleted");

    for(size_t i = 0; i < 1024 * 1024; ++i) {
        tpa_set(tpa, i, &val);
    }

    eq(16916480, tpa_size(tpa), "Size of TPA with lots of entries");
    eq(1048576, tpa_entries(tpa), "Entrycount of TPA with lots of entries");
    eq(tpa_entries_per_page(tpa) * 4129, tpa_length(tpa), "Length of TPA with lots of entries");

    return error ? 1 : 0;
}
