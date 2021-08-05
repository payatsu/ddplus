#include "target.hpp"
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "misc.hpp"

const long target::page_size_ = sysconf(_SC_PAGESIZE);

target::target(const std::string& filename, std::size_t offset, std::size_t length)
: ptr_to_fd_(new int(open(filename.c_str(),O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)), close),
mmapped_data_(),
offset_(offset),
length_(length),
page_offset_()
{
    if(*ptr_to_fd_ == -1){
        ERROR_THROW("", filename);
    }

    if(page_size_ == -1){
        ERROR_THROW("", "sysconf");
    }

    struct stat buf;
    if(fstat(*ptr_to_fd_, &buf) == -1){
        ERROR_THROW("", "fstat");
    }

    switch(buf.st_mode & S_IFMT){
    case S_IFREG:
    case S_IFLNK:
        offset_ = 0u;
        length_ = static_cast<std::size_t>(buf.st_size);
        break;
    case S_IFSOCK:
    case S_IFBLK:
    case S_IFCHR:
    case S_IFIFO:
        break;
    case S_IFDIR:
    default:
        ERROR_THROW("", "unsupported st_mode");
        break;
    }

    page_offset_ = offset_ & (static_cast<std::size_t>(page_size_) - 1);

    if(length_ == 0){
        return;
    }

    void* m = mmap(nullptr, page_offset_ + length_,
            PROT_READ | PROT_WRITE, MAP_SHARED, *ptr_to_fd_,
            static_cast<off_t>(offset_ & ~(
                    static_cast<std::size_t>(page_size_) - 1)));

    if(m == MAP_FAILED){
        ERROR_THROW("", "mmap");
    }

    mmapped_data_.reset(reinterpret_cast<char*>(m),
            [&](char* p){munmap(p, page_offset_ + length_);});
}

target::target(int fd)
: ptr_to_fd_(new int(fd), [](int*){/* do nothing. */}),
mmapped_data_(),
offset_(),
length_(),
page_offset_()
{}

int target::transfer_to(const target& dest, bool hexdump_enabled)const
{
    if(mmapped_data_){
        if(dest.mmapped_data_){
            std::memcpy(dest.offset(), offset(), std::min(length(), dest.length()));
        }else{
            if(hexdump_enabled){
                if(hexdump(*dest.ptr_to_fd_) != 0){
                    ERROR("", "hexdump");
                }
            }else{
                ssize_t w_ret = write(*dest.ptr_to_fd_, offset(), length());
                if(w_ret == -1){
                    ERROR("", "write");
                }
            }
        }
    }else{
        const std::size_t buff_size = static_cast<std::size_t>(page_size_) * 16ul;
        std::shared_ptr<char[]> buff(new char[buff_size]);

        ssize_t r_ret;
        std::size_t transfer_count = 0ul;
        while((r_ret = read(*ptr_to_fd_, buff.get(), buff_size)) != 0){
            if(r_ret == -1){
                ERROR("", "read");
            }
            std::size_t r = static_cast<std::size_t>(r_ret);
            if(dest.mmapped_data_){
                if(dest.length() < transfer_count + r){
                    r = dest.length() - transfer_count;
                }
                std::memcpy(dest.offset() + transfer_count, buff.get(), r);
                transfer_count += r;
                if(dest.length() <= transfer_count){
                    break;
                }
            }else{
                ssize_t w_ret = write(*dest.ptr_to_fd_, buff.get(), r);
                if(w_ret == -1){
                    ERROR("", "write");
                }
            }
        }
    }
    return 0;
}

#define SNPRINTF(fd, buff, size, count, ...) \
    do{ \
        const int printf_ret = std::snprintf( \
                buff + count, \
                size - count, __VA_ARGS__); \
        \
        if(printf_ret < 0){ \
            ERROR("", "std::snprintf"); \
        } \
        if(size - count <= static_cast<std::size_t>(printf_ret)){ \
            ERROR("", "truncating occurred in std::snprintf"); \
        } \
        \
        count += static_cast<decltype(count)>(printf_ret); \
        \
        if(size * 8 / 10 < count){ \
            /* if buff is filled 80% or more, flush it. */ \
            ssize_t ret = write(fd, buff, count); \
            if(ret == -1){ \
                ERROR("", "write"); \
            } \
            count = 0; \
        } \
    }while(false)

int target::hexdump(int fd)const
{
    const char* p = mmapped_data_.get();
    std::size_t column_heading = offset_ - page_offset_;

    const std::size_t buff_size = static_cast<std::size_t>(page_size_) * 20ul;
    std::shared_ptr<char[]> buff(new char[buff_size]);
    std::size_t write_count = 0;

    SNPRINTF(fd, buff.get(), buff_size, write_count,
    "                 0        4        8        c       \n"
    "                 -----------------------------------\n");

    char chars[0x11] = {}; // also contains termination character '\0'.

    for(std::size_t i = 0; i < page_offset_ + length_; ++i){


        // TODO: 読み飛ばし部分の処理を作る。

        if((i & 0xf) == 0x0){
            SNPRINTF(fd, buff.get(), buff_size, write_count, "%016lx", column_heading);
            column_heading += 0x10;
        }

        if((i & 0x3) == 0x0){
            SNPRINTF(fd, buff.get(), buff_size, write_count, " ");
        }

        SNPRINTF(fd, buff.get(), buff_size, write_count, "%02x", static_cast<unsigned int>(p[i] & 0xff));
        chars[i & 0xf] = std::isprint(p[i]) ? p[i] : '.';

        if((i & 0xf) == 0xf){
            SNPRINTF(fd, buff.get(), buff_size, write_count, " >%s<", chars);
            std::memset(chars, '\0', sizeof(chars));
            SNPRINTF(fd, buff.get(), buff_size, write_count, "\n");
        }
    }

    for(std::size_t i = 0;
            i < 2 * (~(page_offset_ + length_ -1) & 0xf)
            + ((~(page_offset_ + length_ -1)) >> 2 & 0x3); ++i){
        SNPRINTF(fd, buff.get(), buff_size, write_count, " ");
    }
    SNPRINTF(fd, buff.get(), buff_size, write_count, " >%s<\n", chars);

    if(0 < write_count){
        ssize_t ret = write(fd, buff.get(), write_count);
        if(ret == -1){
            ERROR("", "write");
        }
    }
    return 0;
}

ssize_t target::read(int fd, void* buf, size_t count)
{
    ssize_t ret;
    do{
        ret = ::read(fd, buf, count);
    }while(ret == -1 && errno == EINTR);
    return ret;
}

ssize_t target::write(int fd, const void* buf, size_t count)
{
    std::size_t done = 0;
    ssize_t ret;
    do{
        do{
            ret = ::write(fd, reinterpret_cast<const char*>(buf) + done, count - done);
        }while(ret == -1 && errno == EINTR);
        if(ret == -1){
            return ret;
        }
        done += static_cast<std::size_t>(ret);
    }while(done < count);
    return ret;
}

void target::close(int* fd_ptr)
{
    if(0 <= *fd_ptr){
        int ret;
        do{
            ret = ::close(*fd_ptr);
        }while(ret == -1 && errno == EINTR);
    }
    delete fd_ptr;
}

// vim: expandtab shiftwidth=0 tabstop=4 :
