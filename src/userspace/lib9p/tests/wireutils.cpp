#include <stdint.h>
#include <vector>
#include <stdexcept>

#include <gtest/gtest.h>

#include <9p/wireutils>

TEST(WireUtils, uint8_t) {
    std::stringstream serialized;
    lib9p::WireUtils::write(uint8_t(0xAB), serialized);

    EXPECT_EQ(serialized.str().length(), 1);
    EXPECT_EQ(memcmp(serialized.str().data(), "\xAB", 1), 0);

    serialized.seekg(0);

    EXPECT_EQ(lib9p::WireUtils::read<uint8_t>(serialized), 0xAB);
}

TEST(WireUtils, uint16_t) {
    std::stringstream serialized;
    lib9p::WireUtils::write(uint16_t(0xFFA0), serialized);

    EXPECT_EQ(serialized.str().length(), 2);
    EXPECT_EQ(memcmp(serialized.str().data(), "\xA0\xFF", 2), 0);

    serialized.seekg(0);

    EXPECT_EQ(lib9p::WireUtils::read<uint16_t>(serialized), 0xFFA0);
}

TEST(WireUtils, uint32_t) {
    std::stringstream serialized;
    lib9p::WireUtils::write(uint32_t(0xA0B0C0D0), serialized);

    EXPECT_EQ(serialized.str().length(), 4);
    EXPECT_EQ(memcmp(serialized.str().data(), "\xD0\xC0\xB0\xA0", 4), 0);

    serialized.seekg(0);

    EXPECT_EQ(lib9p::WireUtils::read<uint32_t>(serialized), 0xA0B0C0D0);
}

TEST(WireUtils, uint64_t) {
    std::stringstream serialized;
    lib9p::WireUtils::write(uint64_t(0xA0B0C0D0E0F0A1A2), serialized);

    EXPECT_EQ(serialized.str().length(), 8);
    EXPECT_EQ(memcmp(serialized.str().data(), "\xA2\xA1\xF0\xE0\xD0\xC0\xB0\xA0", 8), 0);

    serialized.seekg(0);

    EXPECT_EQ(lib9p::WireUtils::read<uint64_t>(serialized), 0xA0B0C0D0E0F0A1A2);
}

TEST(WireUtils, string) {
    std::stringstream serialized;
    lib9p::WireUtils::write("Hello, world!", serialized);

    EXPECT_EQ(serialized.str().length(), 15);
    EXPECT_EQ(memcmp(serialized.str().data(), "\xd\0Hello, world!", 15), 0);

    serialized.seekg(0);

    EXPECT_EQ(lib9p::WireUtils::read<std::string>(serialized), "Hello, world!");
}

TEST(WireUtils, vector_of_string) {
    std::vector<std::string> strings(
        { "Hello", "World", "!" }
    );

    std::stringstream serialized;
    lib9p::WireUtils::write(strings, serialized);

    EXPECT_EQ(serialized.str().length(), 19);
    EXPECT_EQ(memcmp(serialized.str().data(), "\03\0\05\0Hello\05\0World\01\0!", 19), 0);

    serialized.seekg(0);

    EXPECT_EQ(lib9p::WireUtils::read<std::vector<std::string>>(serialized), strings);
}

TEST(WireUtils, basic_string_uint8_t) {
    std::stringstream serialized;
    lib9p::WireUtils::write(std::basic_string<uint8_t>({ 0x23, 0x42, 42, 23 }), serialized);

    EXPECT_EQ(serialized.str().length(), 8);
    EXPECT_EQ(memcmp(serialized.str().data(), "\04\0\0\0\x23\x42\x2a\x17", 8), 0);

    serialized.seekg(0);

    EXPECT_EQ(memcmp(lib9p::WireUtils::read<std::basic_string<uint8_t>>(serialized).data(), "\x23\x42\x2a\x17", 4), 0);
}

TEST(WireUtils, Qid) {
    lib9p::Qid qid(lib9p::Qid::QTFILE, 1337, 0xBADC0DE);

    std::stringstream serialized;
    lib9p::WireUtils::write(qid, serialized);

    EXPECT_EQ(serialized.str().length(), 13);
    EXPECT_EQ(memcmp(serialized.str().data(), "\0\x39\x5\0\0\xde\xc0\xad\xb\0\0\0\0", 13), 0);

    serialized.seekg(0);

    EXPECT_EQ(lib9p::WireUtils::read<lib9p::Qid>(serialized), qid);
}

TEST(WireUtils, vector_of_Qid) {
    std::vector<lib9p::Qid> qids({
        lib9p::Qid(lib9p::Qid::QTFILE, 1337, 0xBADC0DE),
        lib9p::Qid(lib9p::Qid::QTFILE, 42,   0xBADF00D),
        lib9p::Qid(lib9p::Qid::QTFILE, 23,   0xBEEF),
    });

    std::stringstream serialized;
    lib9p::WireUtils::write(qids, serialized);

    EXPECT_EQ(serialized.str().length(), 41);

    serialized.seekg(0);

    EXPECT_EQ(lib9p::WireUtils::read<std::vector<lib9p::Qid>>(serialized), qids);
}

TEST(WireUtils, Stat) {
    lib9p::Qid qid(lib9p::Qid::QTFILE, 1337, 0xBADC0DE);
    lib9p::Stat stat(42, 0xBADDEF, qid, 0777, 1337, 42, 23, "README.md", "root", "root", "littlefox");

    std::stringstream serialized;
    lib9p::WireUtils::write(stat, serialized);

    EXPECT_EQ(serialized.str().length(), 77);
    EXPECT_EQ(memcmp(serialized.str().data(), "\x4b\0\x49\0\x2a\0\xef\xdd\xba\0\0\x39\x5\0\0\xde\xc0\xad\xb\0\0\0\0\xff\x1\0\0\x39\x5\0\0\x2a\0\0\0\x17\0\0\0\0\0\0\0\x9\0README.md\x4\0root\x4\0root\x9\0littlefox", 77), 0);

    serialized.seekg(0);

    EXPECT_EQ(lib9p::WireUtils::read<lib9p::Stat>(serialized), stat);
}
