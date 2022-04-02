#include <cstring>
#include <thread>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "gtest/gtest.h"

#include "common.hpp"
#include "option.hpp"
#include "target.hpp"

const char* progname = nullptr;

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

        target t(file, target_role::SRC);
        EXPECT_EQ(t.length(), static_cast<std::size_t>(buf.st_size));
        EXPECT_EQ(t.offset()[0], '\x7f');
        EXPECT_EQ(t.offset()[1], 'E');
        EXPECT_EQ(t.offset()[2], 'L');
        EXPECT_EQ(t.offset()[3], 'F');
    }

    // in case of) character device(/dev/mem).
    {
        const char* file = "/dev/zero";

        target t1(file, target_role::SRC, 0, 0x2000);
        EXPECT_EQ(t1.length(), 0x2000ul);
        EXPECT_EQ(t1.offset()[0x0000], 0);
        EXPECT_EQ(t1.offset()[0x0001], 0);
        EXPECT_EQ(t1.offset()[0x0fff], 0);
        EXPECT_EQ(t1.offset()[0x1000], 0);
        EXPECT_EQ(t1.offset()[0x1fff], 0);

        target t2(file, target_role::DST, 0x0fff, 0x2000);
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
        EXPECT_THROW(target t(file, target_role::SRC), std::runtime_error);
    }

    // in case of) other file. socket, symbolic link, block device, fifo.
    {
        // TODO: add test.
    }
}

class TransferFromMmapTest: public ::testing::Test{
protected:
    TransferFromMmapTest():
    src("/dev/zero", target_role::DST, 0, 8 << 20)
    {
        prm.verbose = true;
        prm.jobs = 4;

        for(std::size_t i = 0; i < src.length() / sizeof(std::size_t); ++i){
            reinterpret_cast<std::size_t*>(src.offset())[i] = i;
        }
    }

    ~TransferFromMmapTest(){}

    param prm;
    target src;
};

TEST_F(TransferFromMmapTest, ToMmappedTest)
{
    for(int i: {-3, -2, -1, 0, 1, 2, 3}){
        target dst("/dev/zero", target_role::DST, 0, src.length() + i);
        EXPECT_EQ(src.transfer_to(dst, prm), 0);
        const std::size_t min = std::min(dst.length(), src.length());
        EXPECT_EQ(std::memcmp(dst.offset(), src.offset(), min), 0);
        for(std::size_t j = min; j < dst.length(); ++j){
            EXPECT_EQ(dst.offset()[j], 0);
        }
    }
}

TEST_F(TransferFromMmapTest, ToRegularTest)
{
    for(int i: {0, 1, 2, 3}){
        target tmp("/dev/zero", target_role::DST, 0, src.length() - i);
        EXPECT_EQ(src.transfer_to(tmp, prm), 0);
        const char* dst_file = "out.bin";
        target dst(dst_file, target_role::DST);
        EXPECT_EQ(tmp.transfer_to(dst, prm), 0);
        dst.mmap(PROT_READ);
        EXPECT_EQ(std::memcmp(dst.offset(), tmp.offset(), dst.length()), 0);
        unlink(dst_file);
    }
}

TEST_F(TransferFromMmapTest, ToPipeTest)
{
    int pipefd[2];
    EXPECT_EQ(pipe(pipefd), 0);

    std::thread th([&](){
        target dst(pipefd[1]);
        EXPECT_EQ(src.transfer_to(dst, prm), 0);
        close(pipefd[1]);
    });

    target src2(pipefd[0]);
    target dst2("/dev/zero", target_role::DST, 0, src.length());
    EXPECT_EQ(src2.transfer_to(dst2, prm), 0);
    close(pipefd[0]);
    th.join();

    EXPECT_EQ(std::memcmp(dst2.offset(), src.offset(), dst2.length()), 0);
}

class TransferFromPipeTest: public TransferFromMmapTest{
protected:
    TransferFromPipeTest():
    TransferFromMmapTest(),
    pipefd(),
    thread_(pipe_input, pipefd, src)
    {
        while(pipefd[0] == 0 || pipefd[1] == 0){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    ~TransferFromPipeTest()
    {
        close(pipefd[0]);
        thread_.join();
    }

    static void pipe_input(int* pipefd, const target& src)
    {
        EXPECT_EQ(pipe(pipefd), 0);

        struct sigaction act = {};
        act.sa_handler = SIG_IGN;
        EXPECT_NE(sigaction(SIGPIPE, &act, nullptr), -1);

        const std::size_t chunk = 1024;
        int i = 0;
        std::size_t rest = src.length();
        do{
            const ssize_t ret = write(pipefd[1], src.offset() + i * chunk, std::min(rest, chunk));
            if(ret == -1){
                break;
            }
            rest -= ret;
            ++i;
        }while(0 < rest);

        close(pipefd[1]);
    }

    void restart(void)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        thread_.join();
        thread_ = std::thread(pipe_input, pipefd, src);
    }

    int pipefd[2];
    std::thread thread_;
};

TEST_F(TransferFromPipeTest, ToMmappedTest)
{
    for(int i: {-3, -2, -1, 0, 1, 2, 3}){
        target dst("/dev/zero", target_role::DST, 0, src.length() + i);
        EXPECT_EQ(target(pipefd[0]).transfer_to(dst, prm), 0);
        const std::size_t min = std::min(dst.length(), src.length());
        EXPECT_EQ(std::memcmp(dst.offset(), src.offset(), min), 0);
        for(std::size_t j = min; j < dst.length(); ++j){
            EXPECT_EQ(dst.offset()[j], 0);
        }

        restart();
    }
}

TEST_F(TransferFromPipeTest, ToRegularTest)
{
    const char* dst_file = "out.bin";
    target dst(dst_file, target_role::DST);
    EXPECT_EQ(target(pipefd[0]).transfer_to(dst, prm), 0);
    dst.mmap(PROT_READ);
    EXPECT_EQ(std::memcmp(dst.offset(), src.offset(), dst.length()), 0);
    unlink(dst_file);
}

// vim: expandtab shiftwidth=0 tabstop=4 :
