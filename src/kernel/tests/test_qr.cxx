#include <lfostest.h>

namespace LFOS {
    extern "C" {
        #include "../qr.c"
    }

    TEST(KernelQR, qr_right_column) {
        EXPECT_EQ(qr_right_column(176), 176) << "Correctly detects even and > 6 column as right column";
        EXPECT_EQ(qr_right_column(175), 176) << "Correctly determines right column for odd and > 6 column";
        EXPECT_EQ(qr_right_column(174), 174) << "Correctly detects even and > 6 column as right column";
        EXPECT_EQ(qr_right_column(173), 174) << "Correctly determines right column for odd and > 6 column";
        EXPECT_EQ(qr_right_column(0), 1) << "Correctly determines right column for even and < 6 column";
        EXPECT_EQ(qr_right_column(1), 1) << "Correctly detects odd and > 6 column as right column";
        EXPECT_EQ(qr_right_column(2), 3) << "Correctly determines right column for even and < 6 column";
        EXPECT_EQ(qr_right_column(3), 3) << "Correctly detects odd and > 6 column as right column";
    }

    TEST(KernelQR, qr_upwards) {
        EXPECT_EQ(qr_upwards(177, 176), true) << "Correctly detects right side of right-most column as upwards";
        EXPECT_EQ(qr_upwards(177, 175), true) << "Correctly detects left side of right-most column as upwards";

        EXPECT_EQ(qr_upwards(177, 174), false) << "Correctly detects right side of right-most column as upwards";
        EXPECT_EQ(qr_upwards(177, 173), false) << "Correctly detects left side of right-most column as upwards";

        EXPECT_EQ(qr_upwards(21, 20), true) << "Correctly detects right side of right-most column as upwards";
        EXPECT_EQ(qr_upwards(21, 19), true) << "Correctly detects left side of right-most column as upwards";
        EXPECT_EQ(qr_upwards(21, 18), false) << "Correctly detects right side of right-most column as upwards";
        EXPECT_EQ(qr_upwards(21, 17), false) << "Correctly detects left side of right-most column as upwards";
    }

    TEST(KernelQR, qr_next_module) {
        uint8_t qr_size = 177;
        qr_data qr;
        memset(qr, 0, sizeof(qr));

        qr_fixed_patterns(qr, 40);
        qr_format_and_version(qr, 40, 0);

        uint8_t x = qr_size - 1;
        uint8_t y = qr_size - 1;

        qr_next_module(qr, qr_size, &x, &y);
        EXPECT_EQ(x, qr_size - 2) << "Correctly decreased x";
        EXPECT_EQ(y, qr_size - 1) << "Correctly not decreased y";

        qr_next_module(qr, qr_size, &x, &y);
        EXPECT_EQ(x, qr_size - 1) << "Correctly not decreased x";
        EXPECT_EQ(y, qr_size - 2) << "Correctly decreased y";

        qr_next_module(qr, qr_size, &x, &y);
        EXPECT_EQ(x, qr_size - 2) << "Correctly decreased x";
        EXPECT_EQ(y, qr_size - 2) << "Correctly decreased y";

        qr_reserve(qr, y-1, x+1);
        qr_next_module(qr, qr_size, &x, &y);
        EXPECT_EQ(x, qr_size - 1) << "Correctly decreased x";
        EXPECT_EQ(y, qr_size - 3) << "Correctly decreased y";

        qr_next_module(qr, qr_size, &x, &y);
        EXPECT_EQ(x, qr_size - 2) << "Correctly decreased x";
        EXPECT_EQ(y, qr_size - 3) << "Correctly decreased y";

        x = qr_size - 16;
        y = 5;
        qr_next_module(qr, qr_size, &x, &y);
        EXPECT_EQ(x, qr_size - 15) << "Correctly decreased x";
        EXPECT_EQ(y, 7) << "Correctly decreased y";
    }

    TEST(KernelQR, qr_interleaved_data_index) {
        EXPECT_EQ(qr_interleaved_data_index(1, 0), 0);
        EXPECT_EQ(qr_interleaved_data_index(1, 1), 1);
        EXPECT_EQ(qr_interleaved_data_index(1, 2), 2);

        EXPECT_EQ(qr_interleaved_data_index(10, 0), 0);
        EXPECT_EQ(qr_interleaved_data_index(10, 1), 68);
        EXPECT_EQ(qr_interleaved_data_index(10, 2), 136);
    }
}
