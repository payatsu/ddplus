#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "gtest/gtest.h"

#include "option.hpp"

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(ParseTest, ParseRangeTest)
{
    const char* argv[] = {
        "cmd",
        "-r",
        "0x123:0x456",
    };
    option_parser parser(sizeof(argv)/sizeof(argv[0]), const_cast<char**>(argv));

    range range;
    EXPECT_EQ(0, parser.parse_range("1@23", range));
    EXPECT_EQ(range.length,  1u);
    EXPECT_EQ(range.base,   23u);

    EXPECT_EQ(0, parser.parse_range("12@3", range));
    EXPECT_EQ(range.length, 12u);
    EXPECT_EQ(range.base,   3u);

    EXPECT_EQ(0, parser.parse_range("123@456", range));
    EXPECT_EQ(range.length, 123u);
    EXPECT_EQ(range.base,   456u);

    EXPECT_EQ(0, parser.parse_range("0xf@07", range));
    EXPECT_EQ(range.length, 0xfu);
    EXPECT_EQ(range.base,    07u);

    EXPECT_EQ(0, parser.parse_range("0xff@077", range));
    EXPECT_EQ(range.length, 0xffu);
    EXPECT_EQ(range.base,    077u);

    EXPECT_EQ(0, parser.parse_range("0xfff@0777", range));
    EXPECT_EQ(range.length, 0xfffu);
    EXPECT_EQ(range.base,    0777u);

    EXPECT_EQ(0, parser.parse_range("1k@1G", range));
    EXPECT_EQ(range.length, 1u << 10);
    EXPECT_EQ(range.base,   1u << 30);

    EXPECT_EQ(0, parser.parse_range("1M@1m", range));
    EXPECT_EQ(range.length, 1u << 20);
    EXPECT_EQ(range.base,   1u << 20);

    EXPECT_EQ(0, parser.parse_range("1G@1K", range));
    EXPECT_EQ(range.length, 1u << 30);
    EXPECT_EQ(range.base,   1u << 10);
}

// vim: expandtab shiftwidth=0 tabstop=4 :
