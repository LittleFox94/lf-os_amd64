#include <cstdint>

#include <lfostest.h>
#include <gtest/gtest.h>

namespace LFOS {
    extern "C" {
        #include <tpa.c>
    }

    class TpaTest : public ::testing::Test {
        public:
            TpaTest()
                : _tpa(tpa_new(&kernel_alloc, sizeof(uint64_t), _tpa_page_size, 0)) {
            }

        protected:
            const size_t _tpa_page_size = 4096;
            tpa_t*       _tpa;
    };

    TEST_F(TpaTest, Empty) {
        EXPECT_EQ(tpa_size(_tpa),    4096) << "Size of TPA with no entries correct";
        EXPECT_EQ(tpa_entries(_tpa), 0)    << "Entrycount of TPA with no entries correct";
        EXPECT_EQ(tpa_length(_tpa), -1)    << "Length of TPA with no entries correct";
    }

    TEST_F(TpaTest, BasicData) {
        size_t idx   = 0x1337;
        uint64_t val = 23;
        tpa_set(_tpa, idx, &val);

        {
            SCOPED_TRACE("Checking sizes with a single entry");

            size_t page_num = (idx + tpa_entries_per_page(_tpa) - 1) / tpa_entries_per_page(_tpa);
            size_t len      = tpa_entries_per_page(_tpa) * page_num;

            EXPECT_EQ(tpa_size(_tpa),    8192) << "Size of TPA with a single entry correct";
            EXPECT_EQ(tpa_entries(_tpa), 1)    << "Entrycount of TPA with a single entry correct";
            EXPECT_EQ(tpa_length(_tpa),  len)  << "Length of TPA with no entries correct";
        }

        {
            SCOPED_TRACE("Checking retrieval of known-good value");

            void* entry = tpa_get(_tpa, idx);

            EXPECT_NE(entry, 0)        << "Entry returned";
            EXPECT_EQ(*(uint64_t*)entry, val) << "Correct entry value";
        }

        {
            SCOPED_TRACE("Checking retrieval of known-bad value");

            void* non_entry = tpa_get(_tpa, idx + 1);
            EXPECT_EQ(non_entry, 0) << "Not existing entry returns 0";
        }

        {
            SCOPED_TRACE("Checking size after deleting all data");

            tpa_set(_tpa, idx, 0);

            EXPECT_EQ(tpa_size(_tpa),    _tpa_page_size) << "Size of TPA with all entries deleted";
            EXPECT_EQ(tpa_entries(_tpa), 0)              << "Entrycount of TPA with all entries deleted";
            EXPECT_EQ(tpa_length(_tpa),  -1)             << "Length of TPA with all entries deleted";
        }
    }

    TEST_F(TpaTest, LotsOfData) {
        uint64_t val = 42;

        const size_t entry_count = (rand() / (double)RAND_MAX) * 256 * 1024;
        RecordProperty("EntryCount", entry_count);


        const size_t num_pages         = (entry_count + tpa_entries_per_page(_tpa) - 1)
                                       / tpa_entries_per_page(_tpa);
        const size_t expected_tpa_size = _tpa_page_size                 // page for the tpa_t header
                                       + (num_pages * _tpa_page_size);  // all the pages
        const size_t expected_tpa_len  = num_pages * tpa_entries_per_page(_tpa);

        for(size_t i = 0; i < entry_count; ++i) {
            tpa_set(_tpa, i, &val);
        }

        EXPECT_EQ(tpa_size(_tpa),    expected_tpa_size) << "Size of TPA with lots of entries";
        EXPECT_EQ(tpa_entries(_tpa), entry_count)       << "Entrycount of TPA with lots of entries";
        EXPECT_EQ(tpa_length(_tpa),  expected_tpa_len)  << "Length of TPA with lots of entries";
    }
}
