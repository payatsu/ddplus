#ifndef TARGET_HPP_
#define TARGET_HPP_

#include <memory>
#include <string>
#include <unistd.h>
#include "fwd.hpp"

enum class target_role{
    SRC,
    DST,
};

enum class endian{
    HOST,
    BIG,
    LITTLE,
};

class target{
public:
    target(const std::string& filename, std::size_t offset = 0ul, std::size_t length = 0ul);
    target(int fd);
    target(const target&) = default;
    ~target(){}

    int transfer_to(const target& dest, const param& prm)const;

    // deprecated.
    char* offset()const{return mmapped_data_.get() + page_offset_;}
    std::size_t length()const{return length_;}

private:
    std::shared_ptr<int> ptr_to_fd_;
    std::shared_ptr<char> mmapped_data_;
    std::size_t offset_;
    std::size_t length_;
    std::size_t page_offset_;

    static int hexdump(int fd, const char* data, std::size_t offset,
            std::size_t length, std::size_t page_offset, int width);
    static std::uint64_t fetch(const void* p, int width, endian e = endian::HOST);

    const static long page_size_;

    class iohelper{
    public:
        iohelper(int fd, std::size_t size);
        ~iohelper();

        template <typename... Args>
        int snprintf(const char* format, Args... args); // defined in .cpp file.

    public:
        static ssize_t read(int fd, void* buf, size_t count);
        static ssize_t write(int fd, const void* buf, size_t count);
        static void close(int*);

    private:
        const int fd_;
        const std::size_t size_;
        std::shared_ptr<char[]> buf_;
        std::size_t count_;
    };
};

#endif // TARGET_HPP_

// vim: expandtab shiftwidth=0 tabstop=4 :
