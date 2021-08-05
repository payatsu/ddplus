#include <unistd.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "gtest/gtest.h"

#include "common.hpp"
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
    std::string src, dst;
    EXPECT_NO_THROW(parser.parse_transfer("1@23:4@56", src, dst));
    EXPECT_EQ(src, "1@23");
    EXPECT_EQ(dst, "4@56");

    EXPECT_NO_THROW(parser.parse_transfer("7@89:/home/alice/data.bin", src, dst));
    EXPECT_EQ(src, "7@89");
    EXPECT_EQ(dst, "/home/alice/data.bin");

    EXPECT_NO_THROW(parser.parse_transfer("/home/bob/data.bin:10@11", src, dst));
    EXPECT_EQ(src, "/home/bob/data.bin");
    EXPECT_EQ(dst, "10@11");

    EXPECT_NO_THROW(parser.parse_transfer("/home/charlie/data.bin:/home/derek/data.bin", src, dst));
    EXPECT_EQ(src, "/home/charlie/data.bin");
    EXPECT_EQ(dst, "/home/derek/data.bin");

    EXPECT_THROW(parser.parse_transfer(":/source/is/missing", src, dst), std::runtime_error);
    EXPECT_THROW(parser.parse_transfer("/dest/is/missing:", src, dst), std::runtime_error);
}

TEST_F(ParseTest, ParseRangeTest)
{
    std::size_t offset;
    std::size_t length;
    EXPECT_NO_THROW(parser.parse_range("1@23", offset, length));
    EXPECT_EQ(length,  1u);
    EXPECT_EQ(offset, 23u);

    EXPECT_NO_THROW(parser.parse_range("12@3", offset, length));
    EXPECT_EQ(length, 12u);
    EXPECT_EQ(offset,  3u);

    EXPECT_NO_THROW(parser.parse_range("123@456", offset, length));
    EXPECT_EQ(length, 123u);
    EXPECT_EQ(offset, 456u);

    EXPECT_NO_THROW(parser.parse_range("0xf@07", offset, length));
    EXPECT_EQ(length, 0xfu);
    EXPECT_EQ(offset,  07u);

    EXPECT_NO_THROW(parser.parse_range("0xff@077", offset, length));
    EXPECT_EQ(length, 0xffu);
    EXPECT_EQ(offset,  077u);

    EXPECT_NO_THROW(parser.parse_range("0xfff@0777", offset, length));
    EXPECT_EQ(length, 0xfffu);
    EXPECT_EQ(offset,  0777u);

    EXPECT_NO_THROW(parser.parse_range("1k@1G", offset, length));
    EXPECT_EQ(length, 1u << 10);
    EXPECT_EQ(offset, 1u << 30);

    EXPECT_NO_THROW(parser.parse_range("1M@1m", offset, length));
    EXPECT_EQ(length, 1u << 20);
    EXPECT_EQ(offset, 1u << 20);

    EXPECT_NO_THROW(parser.parse_range("1G@1K", offset, length));
    EXPECT_EQ(length, 1u << 30);
    EXPECT_EQ(offset, 1u << 10);

    EXPECT_THROW(parser.parse_range("an_illegal_notation",
                offset, length), std::runtime_error);
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

TEST(TargetTest, TransferTest)
{
    param prm;

    // from: memory mapped file
    // to  : memory mapped file
    {
        target src("/dev/zero", 0, 8);
        for(std::size_t i = 0; i < src.length(); ++i){
            src.offset()[i] = '\xff';
        }

        target dst_just("/dev/zero", 0, 8);
        EXPECT_EQ(src.transfer_to(dst_just, prm), 0);
        for(std::size_t i = 0; i < dst_just.length(); ++i){
            EXPECT_EQ(dst_just.offset()[i], '\xff');
        }

        target dst_short("/dev/zero", 0, 4);
        EXPECT_EQ(src.transfer_to(dst_short, prm), 0);
        for(std::size_t i = 0; i < dst_short.length(); ++i){
            EXPECT_EQ(dst_short.offset()[i], '\xff');
        }

        target dst_wide("/dev/zero", 0, 16);
        EXPECT_EQ(src.transfer_to(dst_wide, prm), 0);
        for(std::size_t i = 0; i < src.length(); ++i){
            EXPECT_EQ(dst_wide.offset()[i], '\xff');
        }
        for(std::size_t i = src.length(); i < dst_wide.length(); ++i){
            EXPECT_EQ(dst_wide.offset()[i], 0);
        }
    }

    // from:     memory mapped file
    // to  : non memory mapped file
    {
        const char* dst_file = "out.bin";
        unlink(dst_file);

        target src("/dev/zero", 0, 4096);
        EXPECT_EQ(src.transfer_to(target(dst_file), prm), 0);
        target dst(dst_file);
        EXPECT_EQ(dst.length(), src.length());
        EXPECT_EQ(dst.offset()[0], src.offset()[0]);
        EXPECT_EQ(dst.offset()[1], src.offset()[1]);
        EXPECT_EQ(dst.offset()[dst.length() - 1], src.offset()[src.length() - 1]);

        unlink(dst_file);
    }

    // from: non memory mapped file
    // to  :     memory mapped file
    {
        int fd[2];
        EXPECT_EQ(pipe(fd), 0);
        target src(fd[0]);

        char buf[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
        ssize_t size = sizeof(buf);

        write(fd[1], buf, size);
        target dst_just("/dev/zero", 0, size);
        EXPECT_EQ(src.transfer_to(dst_just, prm), 0);
        EXPECT_EQ(dst_just.offset()[       0], 'a');
        EXPECT_EQ(dst_just.offset()[       1], 'b');
        EXPECT_EQ(dst_just.offset()[size - 1], 'h');

        write(fd[1], buf, size);
        target dst_short("/dev/zero", 0, size / 2);
        EXPECT_EQ(src.transfer_to(dst_short, prm), 0);
        EXPECT_EQ(dst_short.offset()[0], 'a');
        EXPECT_EQ(dst_short.offset()[1], 'b');
        EXPECT_EQ(dst_short.offset()[3], 'd');
        EXPECT_EQ(dst_short.offset()[4],   0); // check for 'of by one' error.

        write(fd[1], buf, size);
        close(fd[1]);
        target dst_wide("/dev/zero", 0, size * 2);
        EXPECT_EQ(src.transfer_to(dst_wide, prm), 0);
        EXPECT_EQ(dst_wide.offset()[       0], 'a');
        EXPECT_EQ(dst_wide.offset()[       1], 'b');
        EXPECT_EQ(dst_wide.offset()[size - 1], 'h');
        EXPECT_EQ(dst_wide.offset()[size    ],   0);
    }

    // from: non memory mapped file
    // to  : non memory mapped file
    {
        int fd[2];
        EXPECT_EQ(pipe(fd), 0);
        target src(fd[0]);

        char buf[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
        ssize_t size = sizeof(buf);
        write(fd[1], buf, size);
        close(fd[1]);

        const char* dst_file = "out.bin";
        unlink(dst_file);

        EXPECT_EQ(src.transfer_to(target(dst_file), prm), 0);
        target dst(dst_file);
        EXPECT_EQ(dst.length(), static_cast<std::size_t>(size));
        EXPECT_EQ(dst.offset()[0], 'a');
        EXPECT_EQ(dst.offset()[1], 'b');
        EXPECT_EQ(dst.offset()[7], 'h');

        unlink(dst_file);
    }
}

// vim: expandtab shiftwidth=0 tabstop=4 :
