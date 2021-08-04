#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "gtest/gtest.h"

#include "option.hpp"
#include "target.hpp"

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

TEST_F(ParseTest, ParseTransferTest)
{
    std::string from, to;
    EXPECT_EQ(0, parser.parse_transfer("1@23:4@56", from, to));
    EXPECT_EQ(from, "1@23");
    EXPECT_EQ(to,   "4@56");

    EXPECT_EQ(0, parser.parse_transfer("7@89:/home/alice/data.bin", from, to));
    EXPECT_EQ(from, "7@89");
    EXPECT_EQ(to,   "/home/alice/data.bin");

    EXPECT_EQ(0, parser.parse_transfer("/home/bob/data.bin:10@11", from, to));
    EXPECT_EQ(from, "/home/bob/data.bin");
    EXPECT_EQ(to,   "10@11");

    EXPECT_EQ(0, parser.parse_transfer("/home/charlie/data.bin:/home/derek/data.bin", from, to));
    EXPECT_EQ(from, "/home/charlie/data.bin");
    EXPECT_EQ(to,   "/home/derek/data.bin");

    EXPECT_NE(0, parser.parse_transfer(":/source/is/missing", from, to));
    EXPECT_NE(0, parser.parse_transfer("/dest/is/missing:", from, to));
}

TEST_F(ParseTest, ParseRangeTest)
{
    range range;
    EXPECT_EQ(0, parser.parse_range("1@23", range));
    EXPECT_EQ(range.length,  1u);
    EXPECT_EQ(range.offset, 23u);

    EXPECT_EQ(0, parser.parse_range("12@3", range));
    EXPECT_EQ(range.length, 12u);
    EXPECT_EQ(range.offset,  3u);

    EXPECT_EQ(0, parser.parse_range("123@456", range));
    EXPECT_EQ(range.length, 123u);
    EXPECT_EQ(range.offset, 456u);

    EXPECT_EQ(0, parser.parse_range("0xf@07", range));
    EXPECT_EQ(range.length, 0xfu);
    EXPECT_EQ(range.offset,  07u);

    EXPECT_EQ(0, parser.parse_range("0xff@077", range));
    EXPECT_EQ(range.length, 0xffu);
    EXPECT_EQ(range.offset,  077u);

    EXPECT_EQ(0, parser.parse_range("0xfff@0777", range));
    EXPECT_EQ(range.length, 0xfffu);
    EXPECT_EQ(range.offset,  0777u);

    EXPECT_EQ(0, parser.parse_range("1k@1G", range));
    EXPECT_EQ(range.length, 1u << 10);
    EXPECT_EQ(range.offset, 1u << 30);

    EXPECT_EQ(0, parser.parse_range("1M@1m", range));
    EXPECT_EQ(range.length, 1u << 20);
    EXPECT_EQ(range.offset, 1u << 20);

    EXPECT_EQ(0, parser.parse_range("1G@1K", range));
    EXPECT_EQ(range.length, 1u << 30);
    EXPECT_EQ(range.offset, 1u << 10);

    EXPECT_NE(0, parser.parse_range("an_illegal_notation", range));
}

TEST(TargetTest, ConstructionTest)
{
    // in case of) regular file.
    {
        const char* file = "test.o";
        struct stat buf;
        EXPECT_NE(stat(file, &buf), -1);

        target t(file);
        EXPECT_EQ(t.length(), static_cast<std::size_t>(buf.st_size));
        EXPECT_EQ(t.offset()[0], '\x7f');
        EXPECT_EQ(t.offset()[1], 'E');
        EXPECT_EQ(t.offset()[2], 'L');
        EXPECT_EQ(t.offset()[3], 'F');
    }

    // in case of) character device(/dev/mem).
    {
        const char* file = "/dev/zero";

        target t1(file, 0, 0x2000);
        EXPECT_EQ(t1.length(), 0x2000ul);
        EXPECT_EQ(t1.offset()[0x0000], 0);
        EXPECT_EQ(t1.offset()[0x0001], 0);
        EXPECT_EQ(t1.offset()[0x0fff], 0);
        EXPECT_EQ(t1.offset()[0x1000], 0);
        EXPECT_EQ(t1.offset()[0x1fff], 0);

        target t2(file, 0x0fff, 0x2000);
        EXPECT_EQ(t2.length(), 0x2000ul);
        EXPECT_EQ(t2.offset()[0x0000], 0);
        EXPECT_EQ(t2.offset()[0x0001], 0);
        EXPECT_EQ(t2.offset()[0x0fff], 0);
        EXPECT_EQ(t2.offset()[0x1000], 0);
        EXPECT_EQ(t2.offset()[0x1fff], 0);
    }

    // in case of) directory.
    {
        const char* file = "/";
        EXPECT_THROW(target t(file), std::runtime_error);
    }

    // in case of) other file. socket, symbolic link, block device, fifo.
    {
        // TODO: add test.
    }
}

// vim: expandtab shiftwidth=0 tabstop=4 :
