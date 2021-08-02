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
    EXPECT_EQ(0, parser.parse_range("1:23", range));
    EXPECT_EQ(range.base,    1);
    EXPECT_EQ(range.length, 23);

    EXPECT_EQ(0, parser.parse_range("12:3", range));
    EXPECT_EQ(range.base,  12);
    EXPECT_EQ(range.length, 3);

    EXPECT_EQ(0, parser.parse_range("123:456", range));
    EXPECT_EQ(range.base,   123);
    EXPECT_EQ(range.length, 456);

    EXPECT_EQ(0, parser.parse_range("0xf:07", range));
    EXPECT_EQ(range.base,  0xf);
    EXPECT_EQ(range.length, 07);

    EXPECT_EQ(0, parser.parse_range("0xff:077", range));
    EXPECT_EQ(range.base,  0xff);
    EXPECT_EQ(range.length, 077);

    EXPECT_EQ(0, parser.parse_range("0xfff:0777", range));
    EXPECT_EQ(range.base,  0xfff);
    EXPECT_EQ(range.length, 0777);

    EXPECT_EQ(0, parser.parse_range("1k:1G", range));
    EXPECT_EQ(range.base,   1 << 10);
    EXPECT_EQ(range.length, 1 << 30);

    EXPECT_EQ(0, parser.parse_range("1M:1m", range));
    EXPECT_EQ(range.base,   1 << 20);
    EXPECT_EQ(range.length, 1 << 20);

    EXPECT_EQ(0, parser.parse_range("1G:1K", range));
    EXPECT_EQ(range.base,   1 << 30);
    EXPECT_EQ(range.length, 1 << 10);
}

// vim: expandtab shiftwidth=0 tabstop=4 :
