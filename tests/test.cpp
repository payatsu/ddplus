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

class ParseTest: public ::testing::Test{
protected:
    ParseTest(): parser(argc_, const_cast<char**>(argv_)){}

    const char* argv_[3] = {
        "cmd",
        "-r",
        "0x123@0x456",
    };
    int argc_ = sizeof(argv_)/sizeof(argv_[0]);
    option_parser parser;
};

TEST_F(ParseTest, ParseRangeTest)
{
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

    EXPECT_NE(0, parser.parse_range("an_illegal_notation", range));
}

// vim: expandtab shiftwidth=0 tabstop=4 :
