#include "target.hpp"
#include <cctype>
#include <cstdio>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "common.hpp"
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

int target::transfer_to(const target& dest, const param& prm)const
{
    if(mmapped_data_){
        if(dest.mmapped_data_){
            std::memcpy(dest.offset(), offset(), std::min(length_, dest.length_));
        }else{
            if(prm.hexdump_enabled){
                if(hexdump(*dest.ptr_to_fd_, mmapped_data_.get(),
                            offset_, length_, page_offset_, prm.width) != 0){
                    ERROR("", "hexdump");
                }
            }else{
                ssize_t w_ret = write(*dest.ptr_to_fd_, offset(), length_);
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
                if(dest.length_ < transfer_count + r){
                    r = dest.length_ - transfer_count;
                }
                std::memcpy(dest.offset() + transfer_count, buff.get(), r);
                transfer_count += r;
                if(dest.length_ <= transfer_count){
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

int target::hexdump(int fd, const char* data, std::size_t offset,
        std::size_t length, std::size_t page_offset, int width)
{
    const std::size_t bufsize = static_cast<std::size_t>(page_size_) * 20ul;
    std::shared_ptr<char[]> buff(new char[bufsize]);
    char* b = buff.get();
    std::size_t write_count = 0;

    SNPRINTF(fd, b, bufsize, write_count,
    "Offset           0       %s4        8       %sc         ASCII\n"
    "---------------- --------%s-----------------%s--------  -----------------\n",
    width < 64 ? " ": "", width < 64 ? " ": "",
    width < 64 ? " ": "", width < 64 ? " ": "");

    std::size_t column_heading = offset & ~0xful;
    bool needs_column_heading_print = true;

    // this contains space not only for termination character '\0',
    // but also for preceeding character '>'.
    char ascii[0x10 + 2] = {'>'};

    for(std::size_t i = page_offset & ~0xful; i < page_offset + length;
            i += static_cast<std::size_t>(width) / 8){

        if(needs_column_heading_print){
            SNPRINTF(fd, b, bufsize, write_count, "%016lx", column_heading);
            column_heading += 0x10;
            needs_column_heading_print = false;
        }

        if((i & 0x3) == 0x0){
            SNPRINTF(fd, b, bufsize, write_count, " ");
        }

        if(i < page_offset){
            SNPRINTF(fd, b, bufsize, write_count, "  ");
            ascii[(i & 0xf)    ] = ' ';
            ascii[(i & 0xf) + 1] = '>';
        }else{
            SNPRINTF(fd, b, bufsize, write_count, "%0*lx", width / 4, fetch(data + i, width));
            for(std::size_t j = 0; j < static_cast<std::size_t>(width) / 8; ++j){
                ascii[((i + j) & 0xf) + 1] = std::isprint(data[i + j]) ? data[i + j] : '.';
            }
        }

        if(((i + static_cast<std::size_t>(width) / 8) & 0xf) == 0x0){
            SNPRINTF(fd, b, bufsize, write_count, " %s<\n", ascii);
            std::memset(ascii, '\0', sizeof(ascii));
            ascii[0] = '>';
            needs_column_heading_print = true;
        }
    }

    for(std::size_t i = 0;
            i < 2 * (~(page_offset + length -1) & 0xf)
            + ((~(page_offset + length -1)) >> 2 & 0x3); ++i){
        SNPRINTF(fd, b, bufsize, write_count, " ");
    }
    if(ascii[1]){
        SNPRINTF(fd, b, bufsize, write_count, " %s<\n", ascii);
    }

    if(0 < write_count){
        ssize_t ret = write(fd, b, write_count);
        if(ret == -1){
            ERROR("", "write");
        }
    }
    return 0;
}

std::uint64_t target::fetch(const void* p, int width)
{
    std::uint64_t mask = 0;
    std::uint64_t ret = 0;
    switch(width){
    case 8:
        mask = 0x00000000000000ff;
        ret = *reinterpret_cast<const std::uint8_t*>(p) & mask;
        break;
    case 16:
        mask = 0x000000000000ffff;
        ret = htobe16(*reinterpret_cast<const std::uint16_t*>(p)) & mask;
        break;
    case 32:
        mask = 0x00000000ffffffff;
        ret = htobe32(*reinterpret_cast<const std::uint32_t*>(p)) & mask;
        break;
    case 64:
        mask = 0xffffffffffffffff;
        ret = htobe64(*reinterpret_cast<const std::uint64_t*>(p)) & mask;
        break;
    default:
        ERROR_THROW("", std::string("unsupported bit width: ") + std::to_string(width));
        break;
    }
    return ret;
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
