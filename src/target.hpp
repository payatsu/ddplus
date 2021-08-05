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

    int hexdump(int fd)const;

    static ssize_t read(int fd, void* buf, size_t count);
    static ssize_t write(int fd, const void* buf, size_t count);
    static void close(int*);

    const static long page_size_;
};

#endif // TARGET_HPP_

// vim: expandtab shiftwidth=0 tabstop=4 :
