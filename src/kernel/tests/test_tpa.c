#include "lfostest.h"
#include <tpa.c>

size_t tpa_entries_per_page(tpa_t* tpa);

__attribute__ ((visibility ("default"))) void testmain(TestUtils* t) {
    tpa_t* tpa = tpa_new(alloc, sizeof(uint64_t), 4096, 0);

    t->eq_size_t(4096, tpa_size(tpa), "Size of TPA with no entries correct");
    t->eq_size_t(0, tpa_entries(tpa), "Entrycount of TPA with no entries correct");
    t->eq_size_t(-1, tpa_length(tpa), "Length of TPA with no entries correct");

    uint64_t val = 23;
    tpa_set(tpa, 0x1337, &val);
    t->eq_size_t(8192, tpa_size(tpa), "Size of TPA with a single entry correct");
    t->eq_size_t(1, tpa_entries(tpa), "Entrycount of TPA with a single entry correct");
    t->eq_size_t(tpa_entries_per_page(tpa) * 20, tpa_length(tpa), "Length of TPA with a single entry correct");

    void* entry = tpa_get(tpa, 0x1337);
    t->ne_ptr_t(0, (ptr_t)entry, "Entry returned");
    t->eq_size_t(23, *(uint64_t*)entry, "Correct entry value");

    void* non_entry = tpa_get(tpa, 0x1338);
    t->eq_ptr_t(0, (ptr_t)non_entry, "Not existing entry returns 0");

    tpa_set(tpa, 0x1337, 0);
    t->eq_size_t(4096, tpa_size(tpa), "Size of TPA with all entries deleted");
    t->eq_size_t(0, tpa_entries(tpa), "Entrycount of TPA with all entries deleted");
    t->eq_size_t(-1, tpa_length(tpa), "Length of TPA with all entries deleted");

    for(size_t i = 0; i < 1024 * 1024; ++i) {
        tpa_set(tpa, i, &val);
    }

    t->eq_size_t(16916480, tpa_size(tpa), "Size of TPA with lots of entries");
    t->eq_size_t(1048576, tpa_entries(tpa), "Entrycount of TPA with lots of entries");
    t->eq_size_t(tpa_entries_per_page(tpa) * 4129, tpa_length(tpa), "Length of TPA with lots of entries");
}
