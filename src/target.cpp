#include "target.hpp"
#include <cctype>
#include <cstdio>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "common.hpp"
#include "misc.hpp"

endian to_endian(const std::string& str)
{
    if(str == "host"){
        return endian::HOST;
    }else if(str == "big"){
        return endian::BIG;
    }else if(str == "little"){
        return endian::LITTLE;
    }else{
        ERROR_THROW("", "");
    }
}

const long target::page_size_ = sysconf(_SC_PAGESIZE);

target::target(const std::string& filename, target_role role,
        std::size_t offset, std::size_t length)
: ptr_to_fd_(new int(iohelper::open(filename.c_str(), select_file_flags(role),
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)), iohelper::close),
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

    int prot = 0;
    switch(role){
    case target_role::SRC: prot = PROT_READ;  break;
    case target_role::DST: prot = PROT_WRITE; break;
    default: ERROR_THROW("", ""); break;
    }
    void* m = mmap(nullptr, page_offset_ + length_, prot, MAP_SHARED, *ptr_to_fd_,
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
                            offset_, length_, page_offset_, prm) != 0){
                    ERROR("", "hexdump");
                }
            }else{
                ssize_t w_ret = iohelper::write(*dest.ptr_to_fd_, offset(), length_);
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
        while((r_ret = iohelper::read(*ptr_to_fd_, buff.get(), buff_size)) != 0){
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
                ssize_t w_ret = iohelper::write(*dest.ptr_to_fd_, buff.get(), r);
                if(w_ret == -1){
                    ERROR("", "write");
                }
            }
        }
    }
    return 0;
}

int target::hexdump(int fd, const char* data, std::size_t offset,
        std::size_t length, std::size_t page_offset, const param& prm)
{
    const std::size_t bufsize = static_cast<std::size_t>(page_size_) * 20ul;

    iohelper ioh(fd, bufsize);
    ioh.snprintf(
    "Offset%*s ""0       %s4        8       %sc         ASCII\n"
    "%.*s "     "--------%s-----------------%s--------  ----------------\n",
    2 * sizeof(std::size_t) - 6, "",             prm.width < 64 ? " ": "", prm.width < 64 ? " ": "",
    2 * sizeof(std::size_t), "----------------", prm.width < 64 ? "-": "", prm.width < 64 ? "-": "");

    std::size_t column_heading = offset & ~0xful;
    bool needs_column_heading_print = true;

    // this contains space not only for termination character '\0',
    // but also for preceeding character '>'.
    char ascii[0x10 + 2] = {'>'};

    const std::size_t bytewise_width = static_cast<std::size_t>(prm.width) / 8;
    for(std::size_t i = page_offset & ~0xful; i < page_offset + length; i += bytewise_width){

        if(needs_column_heading_print){
            ioh.snprintf("%0*zx", 2 * sizeof(std::size_t), column_heading);
            column_heading += 0x10;
            needs_column_heading_print = false;
        }

        if((i & 0x3) == 0x0){
            ioh.snprintf(" ");
        }

        if(i < page_offset){
            ioh.snprintf("%*s", 2 * static_cast<int>(bytewise_width), "");
            std::snprintf(ascii, sizeof(ascii), "%*s>",
                    static_cast<int>((i + bytewise_width) & ~(bytewise_width - 1)), "");
        }else{
            ioh.snprintf("%0*lx", prm.width / 4, fetch(data + i, prm.width, prm.endianness));
            const std::uint64_t value_in_stack = fetch(data + i, prm.width);
            const char* p = reinterpret_cast<const char*>(&value_in_stack);
            for(std::size_t j = 0; j < bytewise_width; ++j){
                ascii[((i + j) & 0xful) + 1] = std::isprint(p[j]) ? p[j] : '.';
            }
        }

        if(((i + bytewise_width) & 0xful) == 0x0){
            ioh.snprintf(" %s<\n", ascii);
            std::memset(ascii, '\0', sizeof(ascii));
            ascii[0] = '>';
            needs_column_heading_print = true;
        }
    }

    const std::size_t padding_for_hex = (0xful
            - (((page_offset + length - 1 + bytewise_width) & ~(bytewise_width - 1)) & 0xful)
            + 1) & 0xful;
    const std::size_t padding_for_sep = ((~(page_offset + length -1)) >> 2 & 0x3);
    ioh.snprintf("%*s", static_cast<int>(2 * padding_for_hex + padding_for_sep), "");

    if(ascii[1]){
        ioh.snprintf(" %s<\n", ascii);
    }
    return 0;
}

std::uint64_t target::fetch(const void* p, int width, endian e)
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
        switch(e){
        case endian::BIG:    ret = htobe16(*reinterpret_cast<const std::uint16_t*>(p)) & mask; break;
        case endian::LITTLE: ret = htole16(*reinterpret_cast<const std::uint16_t*>(p)) & mask; break;
        case endian::HOST:
        default:             ret =         *reinterpret_cast<const std::uint16_t*>(p)  & mask; break;
        }
        break;
    case 32:
        mask = 0x00000000ffffffff;
        switch(e){
        case endian::BIG:    ret = htobe32(*reinterpret_cast<const std::uint32_t*>(p)) & mask; break;
        case endian::LITTLE: ret = htole32(*reinterpret_cast<const std::uint32_t*>(p)) & mask; break;
        case endian::HOST:
        default:             ret =         *reinterpret_cast<const std::uint32_t*>(p)  & mask; break;
        }
        break;
    case 64:
        mask = 0xffffffffffffffff;
        switch(e){
        case endian::BIG:    ret = htobe64(*reinterpret_cast<const std::uint64_t*>(p)) & mask; break;
        case endian::LITTLE: ret = htole64(*reinterpret_cast<const std::uint64_t*>(p)) & mask; break;
        case endian::HOST:
        default:             ret =         *reinterpret_cast<const std::uint64_t*>(p)  & mask; break;
        }
        break;
    default:
        ERROR_THROW("", std::string("unsupported bit width: ") + std::to_string(width));
        break;
    }
    return ret;
}

int target::select_file_flags(target_role r)
{
    int ret = 0;
    switch(r){
    case target_role::SRC: ret = O_RDONLY;         break;
    case target_role::DST: ret = O_RDWR | O_CREAT; break;
    default: ERROR_THROW("", ""); break;
    }
    return ret;
}

target::iohelper::iohelper(int fd, std::size_t size)
:fd_(fd),
size_(size),
buf_(new char[size_]),
count_(){}

target::iohelper::~iohelper()
{
    if(count_ <= 0){
        return;
    }
    ssize_t ret = iohelper::write(fd_, buf_.get(), count_);
    if(ret == -1){
        ERROR_BASE("", "write", /* nothing */);
    }
}

template <typename... Args>
__attribute__((format(printf, 2, 3)))
int target::iohelper::snprintf(const char* format, Args... args)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wformat-security"
    const int printf_ret = std::snprintf(
            buf_.get() + count_,
            size_ - count_, format, args...);
#pragma GCC diagnostic pop

    if(printf_ret < 0){
        ERROR("", "std::snprintf");
    }
    if(size_ - count_ <= static_cast<std::size_t>(printf_ret)){
        ERROR("", "truncating occurred in std::snprintf");
    }

    count_ += static_cast<decltype(count_)>(printf_ret);

    if(size_ * 8 / 10 < count_){
        /* if buf_ is filled 80% or more, flush it. */
        ssize_t ret = iohelper::write(fd_, buf_.get(), count_);
        if(ret == -1){
            ERROR("", "write");
        }
        count_ = 0;
    }
    return 0;
}

int target::iohelper::open(const char* pathname, int flags, mode_t mode)
{
    int ret;
    do{
        ret = ::open(pathname, flags, mode);
    }while(ret == -1 && errno == EINTR);
    return ret;
}

ssize_t target::iohelper::read(int fd, void* buf, size_t count)
{
    ssize_t ret;
    do{
        ret = ::read(fd, buf, count);
    }while(ret == -1 && errno == EINTR);
    return ret;
}

ssize_t target::iohelper::write(int fd, const void* buf, size_t count)
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

void target::iohelper::close(int* fd_ptr)
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
