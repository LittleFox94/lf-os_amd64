#include <cstdint>

#include <lfostest.h>

#include <tpa.h>

class TpaTest : public ::testing::Test {
    public:
        TpaTest()
            : _tpa(TPA<uint64_t>::create(&kernel_alloc, _tpa_page_size, 0)) {
        }

        virtual ~TpaTest() {
            _tpa->destroy();
        }

    protected:
        const size_t _tpa_page_size = 4096;
        TPA<uint64_t>*       _tpa;

        size_t _tpa_entries_per_page() { return _tpa->entries_per_page(); }
        //size_t tpa_entries_per_page() { return _tpa->entries_per_page(); }
        //size_t tpa_entries_per_page() { return _tpa->entries_per_page(); }
};

TEST_F(TpaTest, Empty) {
    EXPECT_EQ(_tpa->size(),    4096) << "Size of TPA with no entries correct";
    EXPECT_EQ(_tpa->entries(), 0)    << "Entrycount of TPA with no entries correct";
    EXPECT_EQ(_tpa->length(), -1)    << "Length of TPA with no entries correct";
}

TEST_F(TpaTest, BasicData) {
    size_t idx   = 0x1337;
    uint64_t val = 23;
    _tpa->set(idx, &val);

    {
        SCOPED_TRACE("Checking sizes with a single entry");

        size_t page_num = (idx + _tpa_entries_per_page() - 1) / _tpa_entries_per_page();
        size_t len      = _tpa_entries_per_page() * page_num;

        EXPECT_EQ(_tpa->size(),    8192) << "Size of TPA with a single entry correct";
        EXPECT_EQ(_tpa->entries(), 1)    << "Entrycount of TPA with a single entry correct";
        EXPECT_EQ(_tpa->length(),  len)  << "Length of TPA with no entries correct";
    }

    {
        SCOPED_TRACE("Checking retrieval of known-good value");

        void* entry = _tpa->get(idx);

        EXPECT_NE(entry, (void*)0)        << "Entry returned";
        EXPECT_EQ(*(uint64_t*)entry, val) << "Correct entry value";
    }

    {
        SCOPED_TRACE("Checking retrieval of known-bad value");

        void* non_entry = _tpa->get(idx + 1);
        EXPECT_EQ(non_entry, (void*)0) << "Not existing entry returns 0";
    }

    {
        SCOPED_TRACE("Checking size after deleting all data");

        _tpa->set(idx, 0);

        EXPECT_EQ(_tpa->size(),    _tpa_page_size) << "Size of TPA with all entries deleted";
        EXPECT_EQ(_tpa->entries(), 0)              << "Entrycount of TPA with all entries deleted";
        EXPECT_EQ(_tpa->length(),  -1)             << "Length of TPA with all entries deleted";
    }
}

TEST_F(TpaTest, LotsOfData) {
    uint64_t val = 42;

    const size_t entry_count = (rand() / (double)RAND_MAX) * 256 * 1024;
    RecordProperty("EntryCount", entry_count);

    const size_t num_pages         = (entry_count + _tpa_entries_per_page() - 1)
                                   / _tpa_entries_per_page();
    const size_t expected_tpa_size = _tpa_page_size                 // page for the tpa_t header
                                   + (num_pages * _tpa_page_size);  // all the pages
    const size_t expected_tpa_len  = num_pages * _tpa_entries_per_page();

    for(size_t i = 0; i < entry_count; ++i) {
        _tpa->set(i, &val);
    }

    EXPECT_EQ(_tpa->size(),    expected_tpa_size) << "Size of TPA with lots of entries";
    EXPECT_EQ(_tpa->entries(), entry_count)       << "Entrycount of TPA with lots of entries";
    EXPECT_EQ(_tpa->length(),  expected_tpa_len)  << "Length of TPA with lots of entries";
}
