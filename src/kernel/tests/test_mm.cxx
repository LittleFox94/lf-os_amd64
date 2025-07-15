#include <set>
#include <iomanip>

#include <gtest/gtest.h>

#include "../mm.cpp"

void PrintTo(const mm_page_list_entry& entry, std::ostream* os) {
    *os << "{ 0x" << std::hex << std::setw(4) << std::setfill('0') << entry.start << ", 0x" << std::dec << entry.count << "}";
}

struct PageListEntryCombinationTestCase {
    mm_page_list_entry                  regionA;
    mm_page_list_entry                  regionB;
    bool                                expectedIntersects;
    std::optional<mm_page_list_entry>   expectedPartBefore;
    std::optional<mm_page_list_entry>   expectedPartAfter;

    friend void PrintTo(const PageListEntryCombinationTestCase& testCase, std::ostream* os) {
        *os << "regionA = ";
        PrintTo(testCase.regionA, os);

        *os << ", ";

        *os << "regionB = ";
        PrintTo(testCase.regionB, os);
    }
};

class PageListEntryCombinationTest :
    public testing::TestWithParam<PageListEntryCombinationTestCase>
{
};

TEST_P(PageListEntryCombinationTest, Intersects) {
    auto params = GetParam();
    EXPECT_EQ(params.regionA.intersects(params.regionB), params.expectedIntersects);
}

TEST_P(PageListEntryCombinationTest, PartBefore) {
    auto params = GetParam();
    auto expected = params.expectedPartBefore;
    auto res = params.regionA.part_before(params.regionB);

    EXPECT_EQ(res, expected);
}

TEST_P(PageListEntryCombinationTest, PartAfter) {
    auto params = GetParam();
    auto expected = params.expectedPartAfter;
    auto res = params.regionA.part_after(params.regionB);

    EXPECT_EQ(res, expected);
}

INSTANTIATE_TEST_SUITE_P(
    PageListCombinations,
    PageListEntryCombinationTest,
    testing::Values(
        PageListEntryCombinationTestCase{ { 0x0000, 1 }, { 0x1000, 1 }, false,                   {        },                    {         }},
        PageListEntryCombinationTestCase{ { 0x0000, 2 }, { 0x1000, 1 },  true, mm_page_list_entry{0x0000, 1},                   {         }},
        PageListEntryCombinationTestCase{ { 0x0000, 3 }, { 0x1000, 1 },  true, mm_page_list_entry{0x0000, 1}, mm_page_list_entry{0x2000, 1}},
        PageListEntryCombinationTestCase{ { 0x0000, 3 }, { 0x1000, 2 },  true, mm_page_list_entry{0x0000, 1},                   {         }},
        PageListEntryCombinationTestCase{ { 0x0000, 3 }, { 0x0000, 1 },  true,                   {         }, mm_page_list_entry{0x1000, 2}},
        PageListEntryCombinationTestCase{ { 0x0000, 3 }, { 0x0000, 2 },  true,                   {         }, mm_page_list_entry{0x2000, 1}},
        PageListEntryCombinationTestCase{ { 0x1000, 3 }, { 0x0000, 1 }, false,                   {         },                   {         }},
        PageListEntryCombinationTestCase{ { 0x1000, 3 }, { 0x0000, 2 },  true,                   {         }, mm_page_list_entry{0x2000, 2}},
        PageListEntryCombinationTestCase{ { 0x1000, 3 }, { 0x0000, 3 },  true,                   {         }, mm_page_list_entry{0x3000, 1}},
        PageListEntryCombinationTestCase{ { 0x1000, 3 }, { 0x0000, 4 },  true,                   {         },                   {         }}
    ),
    [](const testing::TestParamInfo<PageListEntryCombinationTest::ParamType>& info) {
        std::stringstream name;
        name << std::hex << std::setw(4) << std::setfill('0') << info.param.regionA.start << "_" << std::dec << info.param.regionA.count << "__";
        name << std::hex << std::setw(4) << std::setfill('0') << info.param.regionB.start << "_" << std::dec << info.param.regionB.count;
        return name.str();
    }
);

class MemoryManagementTest: public testing::Test {
    public:
        virtual void SetUp() override {
        }

        virtual void TearDown() override {
            // we have to clear it manually instead of having the destructor do
            // it to prevent fun wrong-order destruction of this list and the
            // ones in the allocators.
            mm_physical_page_list.clear();
        }

    protected:
        // checks if the list is sorted, without overlaps and without count=0 entries
        void checkIntegrity() {
            mm_print_regions(MM_INVALID);

            uint64_t last_end = 0;

            for(auto region : mm_physical_page_list) {
                EXPECT_NE(region.count, 0)        << "Expect region to not have count=0";
                EXPECT_GE(region.start, last_end) << "Expect region to start after the previous one";

                last_end = region.start + (region.count * 4096);
            }
        }

        auto region_count() {
            std::size_t regions = 0;
            for(auto it = mm_physical_page_list.begin(); it != mm_physical_page_list.end(); ++it) {
                ++regions;
            }
            return regions;
        }
};

TEST_F(MemoryManagementTest, Simple) {
    mm_mark_physical_pages(0x0000, 1, MM_FREE);
    mm_mark_physical_pages(0x1000, 1, MM_FREE);
    checkIntegrity();
    EXPECT_EQ(region_count(), 1);

    auto allocated = reinterpret_cast<uint64_t>(mm_alloc_pages(1));
    EXPECT_EQ(allocated, 0);
    checkIntegrity();
    EXPECT_EQ(region_count(), 2);

    mm_mark_physical_pages(allocated, 1, MM_FREE);
    checkIntegrity();
    EXPECT_EQ(region_count(), 1);

    mm_mark_physical_pages(0x3000, 1, MM_FREE);
    checkIntegrity();
    EXPECT_EQ(region_count(), 2);

    mm_mark_physical_pages(0x2000, 1, MM_FREE);
    checkIntegrity();
    EXPECT_EQ(region_count(), 1);

    mm_mark_physical_pages(0x5000, 1, MM_FREE);
    mm_mark_physical_pages(0x7000, 1, MM_FREE);
    mm_mark_physical_pages(0x9000, 1, MM_FREE);
    checkIntegrity();
    EXPECT_EQ(region_count(), 4);

    mm_mark_physical_pages(0x4000, 1, MM_FREE);
    mm_mark_physical_pages(0x6000, 1, MM_FREE);
    mm_mark_physical_pages(0x8000, 1, MM_FREE);
    checkIntegrity();
    EXPECT_EQ(region_count(), 1);

    mm_mark_physical_pages(0x0000, 4, MM_ALLOCATED);
    checkIntegrity();
    EXPECT_EQ(region_count(), 2);

    mm_mark_physical_pages(0x6000, 4, MM_ALLOCATED);
    checkIntegrity();
    EXPECT_EQ(region_count(), 3);

    mm_mark_physical_pages(0x4000, 2, MM_ALLOCATED);
    checkIntegrity();
    EXPECT_EQ(region_count(), 1);
}

TEST_F(MemoryManagementTest, Holes) {
    mm_mark_physical_pages(0x0000, 16, MM_FREE);
    checkIntegrity();
    EXPECT_EQ(region_count(), 1);

    mm_mark_physical_pages(0x1000, 1, MM_ALLOCATED);
    mm_mark_physical_pages(0x3000, 1, MM_ALLOCATED);
    mm_mark_physical_pages(0x5000, 1, MM_ALLOCATED);
    checkIntegrity();
    EXPECT_EQ(region_count(), 7);
    mm_print_regions(MM_INVALID);

    mm_mark_physical_pages(0x0000, 16, MM_ALLOCATED);
    checkIntegrity();
    EXPECT_EQ(region_count(), 1);
    mm_print_regions(MM_INVALID);
}

TEST_F(MemoryManagementTest, RealMemoryMap) {
    // actual memory map in QEMU with 512Mi memory
    mm_mark_physical_pages(0x00000000,    160, MM_FREE);
    mm_mark_physical_pages(0x00100000,   1792, MM_FREE);
    mm_mark_physical_pages(0x00808000,      3, MM_FREE);
    mm_mark_physical_pages(0x0080C000,      5, MM_FREE);
    mm_mark_physical_pages(0x01780000, 105250, MM_FREE);
    mm_mark_physical_pages(0x1B707000,   1125, MM_FREE);
    mm_mark_physical_pages(0x1bb8c000,    726, MM_FREE);
    mm_mark_physical_pages(0x1c987000,    190, MM_FREE);
    mm_mark_physical_pages(0x1df1d000,     14, MM_FREE);
    mm_mark_physical_pages(0x1E032000,      1, MM_FREE);
    mm_mark_physical_pages(0x1E93F000,    193, MM_RESERVED);

    // this entry is actually overlapping with the next ... BUT since
    // both have the same status, they should be merged anyway.
    mm_mark_physical_pages(0x1F4ED000,    512, MM_RESERVED);
    mm_mark_physical_pages(0x1F5ED000,    256, MM_RESERVED);
    mm_mark_physical_pages(0x1FE00000,    111, MM_RESERVED);
    mm_mark_physical_pages(0x1FEF4000,    132, MM_RESERVED);

    EXPECT_EQ(mm_highest_address(), 0x1FF78000);

    std::set<uint64_t> allocated;

    for(size_t i = 0; i < 256; ++i) {
        auto res = reinterpret_cast<uint64_t>(mm_alloc_pages(1));
        EXPECT_EQ(allocated.count(res), 0) << "0x" << std::hex << res << " has been allocated twice!";
        allocated.insert(res);
    }

    EXPECT_EQ(allocated.size(), 256);

    checkIntegrity();
}
