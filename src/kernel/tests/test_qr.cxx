#include <lfostest.h>

namespace LFOS {
    extern "C" {
        #define ksnprintf snprintf

        void log_read(char* buffer, size_t buffer_size, ssize_t offset) {
        }

#  define __qr_panic() testing::AssertionFailure() << "QR panic!";

        #include "../qr/qr.c"
        #include "../qr/rs.c"
    }
}

class QRTest : public testing::TestWithParam<int> {
    public:
        QRTest() : _version(GetParam()) {
            EXPECT_GE(_version, 1) << "QR code version must match 1 <= version <= 40";
            EXPECT_LE(_version, 40) << "QR code version must match 1 <= version <= 40";
        }

        void SetUp() override {
            EXPECT_EQ(LFOS::qr_all_codewords(_version), numModulesData() / 8);

            memset(_qr_data, 0, sizeof(_qr_data));

            LFOS::qr_fixed_patterns(_qr_data, _version);
            LFOS::qr_format_and_version(_qr_data, _version, 0);
        }

        void TearDown() override {
            return;
            int shouldBeUnset = numModulesSingleDimension();
            for(int x = shouldBeUnset; x < 177; ++x) {
                for(int y = 0; y < shouldBeUnset; ++y) {
                    EXPECT_PRED2([this](uint8_t x, uint8_t y) {
                        return this->dataUntouched(x, y);
                    }, x, y) <<"Destination array outside QR code version-defined bounds should be kept untouched";
                }
            }

            for(int y = shouldBeUnset; y < 177; ++y) {
                for(int x = 0; x < 177; ++x) {
                    EXPECT_PRED2([this](uint8_t x, uint8_t y) {
                        return this->dataUntouched(x, y);
                    }, x, y) <<"Destination array outside QR code version-defined bounds should be kept untouched";
                }
            }
        }

    protected:
        const int numModulesSingleDimension() {
            return 17 + (_version * 4);
        }

        const int numModulesTotal() {
            return numModulesSingleDimension() *
                   numModulesSingleDimension();
        }

        const int numModulesReserved() {
            auto alignmentPatterns = alignmentPatternPositions();
            int numAlignment = alignmentPatterns.size();

            int ret = (64 * 3)                                 // finder patterns
                    + (15 * 2)                                 // format information areas
                    + 1                                        // dark module
                    + (2 * (numModulesSingleDimension() - 16)) // timing
                    + (numAlignment * 25)                      // alignment patterns
                    + (_version >= 7 ? (2 * 18) : 0)           // version information bits
                    ;

            // finally, some alignment patterns will overlap with timing
            // information, leading to double-counted reserved bits - fixing
            // this here:
            for(auto pos = alignmentPatterns.cbegin(); pos != alignmentPatterns.cend(); pos++) {
                if(pos->first == 6 || pos->second == 6) {
                    ret -= 5;
                }
            }

            return ret;
        }

        int numModulesData() {
            return numModulesTotal() - numModulesReserved();
        }

        std::vector<std::pair<int, int>> alignmentPatternPositions() {
            std::set<int> positions;

            uint8_t *alignmentLocation = LFOS::qr_versions[_version].alignment_locations;
            while(*alignmentLocation) {
                positions.insert(*alignmentLocation);
                ++alignmentLocation;
            }

            std::vector<std::pair<int, int>> ret;
            for(auto x = positions.cbegin(); x != positions.cend(); x++) {
                for(auto y = positions.cbegin(); y != positions.cend(); y++) {
                    if(notInFinderPattern(*x, *y)) {
                        ret.push_back(std::pair<int, int>(*x, *y));
                    }
                }
            }

            return ret;
        }

        bool inFinderPattern(int x, int y) {
            int size = numModulesSingleDimension();
            return (x <=        8 && y <=        8)
                || (x <=        8 && y >= size - 8)
                || (x >= size - 8 && y <=        8)
                ;
        }

        bool notInFinderPattern(int x, int y) {
            return !inFinderPattern(x, y);
        }

        bool inAlignmentPattern(int x, int y) {
            auto positions = alignmentPatternPositions();

            for(auto it = positions.cbegin(); it != positions.cend(); it++) {
                bool x_included = x >= it->first  - 2 && x <= it->first  + 2;
                bool y_included = y >= it->second - 2 && y <= it->second + 2;
                if(x_included && y_included) {
                    return true;
                }
            }

            return false;
        }

        bool notInAlignmentPattern(int x, int y) {
            return !inAlignmentPattern(x, y);
        }

        bool dataUntouched(int x, int y) {
            int size = numModulesSingleDimension();
            return _qr_data[(y * 177) + x] == 0;
        }

        uint8_t _qr_data[177 * 177];
        const int _version = 1;
};

TEST_P(QRTest, qr_overflow_check) {
    int bytes = numModulesData() / 8;
    unsigned char data[bytes + 1];
    data[bytes] = 0;

    const char* s = "ABCDEFGHHIJKLMNOPQRSTUVWXYZ0123456789abcdefgijklmnopqrstuvwxyz";
    for(int i = 0; i < bytes; ++i) {
        data[i] = s[i % strlen(s)];
    }

    LFOS::qr_store_data(_qr_data, _version, data);
    LFOS::qr_mask(_qr_data, _version, 0);
}

TEST_P(QRTest, qr_full_data) {
    int bytes = LFOS::qr_userdata(_version);
    char data[bytes + 1];
    data[bytes] = 0;

    const char* s = "ABCDEFGHHIJKLMNOPQRSTUVWXYZ0123456789abcdefgijklmnopqrstuvwxyz";
    for(int i = 0; i < bytes; ++i) {
        data[i] = s[i % strlen(s)];
    }

    LFOS::qr_encode(_qr_data, (uint8_t*)data, strlen(data));
}

TEST_P(QRTest, qr_next_module) {
    int qr_size = numModulesSingleDimension();
    uint8_t x = qr_size - 1;
    uint8_t y = x;

    int numBits = numModulesData();
    for(int i = 0; i <= numBits; ++i) {
        LFOS::qr_next_module(_qr_data, _version, &x, &y);

        if(i < numBits - 1) {
            EXPECT_LE(x, qr_size - 1) << "X should stay inside QR code boundaries";
            EXPECT_LE(y, qr_size - 1) << "Y should stay inside QR code boundaries";
        }

        EXPECT_NE(x, 6) << "X should never be 6 (vertical timing information)";
        EXPECT_NE(y, 6) << "Y should never be 6 (horizontal timing information)";

        EXPECT_PRED2([this](uint8_t x, uint8_t y) {
            return this->notInFinderPattern(x, y);
        }, x, y) << "Should never write in finder pattern";

        EXPECT_PRED2([this](uint8_t x, uint8_t y) {
            return this->notInAlignmentPattern(x, y);
        }, x, y) << "Should never write in alignment pattern";
    }

    /*int qr_size = numModulesSingleDimension();
    uint8_t x = qr_size - 1;
    uint8_t y = qr_size - 1;

    LFOS::qr_next_module(_qr_data, qr_size, &x, &y);
    EXPECT_EQ(x, qr_size - 2) << "Correctly decreased x";
    EXPECT_EQ(y, qr_size - 1) << "Correctly not decreased y";

    LFOS::qr_next_module(_qr_data, qr_size, &x, &y);
    EXPECT_EQ(x, qr_size - 1) << "Correctly not decreased x";
    EXPECT_EQ(y, qr_size - 2) << "Correctly decreased y";

    LFOS::qr_next_module(_qr_data, qr_size, &x, &y);
    EXPECT_EQ(x, qr_size - 2) << "Correctly decreased x";
    EXPECT_EQ(y, qr_size - 2) << "Correctly decreased y";

    LFOS::qr_reserve(_qr_data, y-1, x+1);
    LFOS::qr_next_module(_qr_data, qr_size, &x, &y);
    EXPECT_EQ(x, qr_size - 1) << "Correctly decreased x";
    EXPECT_EQ(y, qr_size - 3) << "Correctly decreased y";

    LFOS::qr_next_module(_qr_data, qr_size, &x, &y);
    EXPECT_EQ(x, qr_size - 2) << "Correctly decreased x";
    EXPECT_EQ(y, qr_size - 3) << "Correctly decreased y";

    x = qr_size - 16;
    y = 5;
    LFOS::qr_next_module(_qr_data, qr_size, &x, &y);
    EXPECT_EQ(x, qr_size - 15) << "Correctly decreased x";
    EXPECT_EQ(y, 7) << "Correctly decreased y";*/
}

INSTANTIATE_TEST_SUITE_P(
    AllQRVersions, QRTest,
    testing::Range(1, 41),
    [](const testing::TestParamInfo<QRTest::ParamType>& info) {
        std::stringstream name;
        name << "QR_version_" << info.param;
        return name.str();
    }
);

TEST(KernelQR, qr_right_column) {
    EXPECT_EQ(LFOS::qr_right_column(176), 176) << "Correctly detects even and > 6 column as right column";
    EXPECT_EQ(LFOS::qr_right_column(175), 176) << "Correctly determines right column for odd and > 6 column";
    EXPECT_EQ(LFOS::qr_right_column(174), 174) << "Correctly detects even and > 6 column as right column";
    EXPECT_EQ(LFOS::qr_right_column(173), 174) << "Correctly determines right column for odd and > 6 column";
    EXPECT_EQ(LFOS::qr_right_column(0), 1) << "Correctly determines right column for even and < 6 column";
    EXPECT_EQ(LFOS::qr_right_column(1), 1) << "Correctly detects odd and > 6 column as right column";
    EXPECT_EQ(LFOS::qr_right_column(2), 3) << "Correctly determines right column for even and < 6 column";
    EXPECT_EQ(LFOS::qr_right_column(3), 3) << "Correctly detects odd and > 6 column as right column";
    EXPECT_EQ(LFOS::qr_right_column(4), 5) << "Correctly detects odd and > 6 column as right column";
    EXPECT_EQ(LFOS::qr_right_column(5), 5) << "Correctly detects odd and > 6 column as right column";

    EXPECT_EQ(LFOS::qr_right_column(7), 8) << "Correctly detects odd and > 6 column as right column";
    EXPECT_EQ(LFOS::qr_right_column(8), 8) << "Correctly detects odd and > 6 column as right column";
}

TEST(KernelQR, qr_upwards) {
    EXPECT_EQ(LFOS::qr_upwards(177, 176), true) << "Correctly detects right side of right-most column as upwards";
    EXPECT_EQ(LFOS::qr_upwards(177, 175), true) << "Correctly detects left side of right-most column as upwards";

    EXPECT_EQ(LFOS::qr_upwards(177, 174), false) << "Correctly detects right side of right-most column as upwards";
    EXPECT_EQ(LFOS::qr_upwards(177, 173), false) << "Correctly detects left side of right-most column as upwards";

    EXPECT_EQ(LFOS::qr_upwards(21, 20), true) << "Correctly detects right side of right-most column as upwards";
    EXPECT_EQ(LFOS::qr_upwards(21, 19), true) << "Correctly detects left side of right-most column as upwards";
    EXPECT_EQ(LFOS::qr_upwards(21, 18), false) << "Correctly detects right side of right-most column as upwards";
    EXPECT_EQ(LFOS::qr_upwards(21, 17), false) << "Correctly detects left side of right-most column as upwards";
}


TEST(KernelQR, qr_interleaved_data_index) {
    EXPECT_EQ(LFOS::qr_interleaved_data_index(1, 0), 0);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(1, 1), 1);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(1, 2), 2);

    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 0), 0);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 1), 4);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 2), 8);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 3), 12);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 4), 16);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 5), 20);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 6), 24);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 7), 28);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 8), 32);

    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 68), 1);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 69), 5);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 70), 9);

    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 136), 2);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 137), 6);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 138), 10);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 204), 272);

    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 205), 3);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 206), 7);
    EXPECT_EQ(LFOS::qr_interleaved_data_index(10, 207), 11);
}
