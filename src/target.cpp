#include "target.hpp"
#include <cctype>
#include <cstdio>
#include <thread>
#include <fcntl.h>
#include <sys/mman.h>
#include "common.hpp"
#include "misc.hpp"
#include "sched.hpp"
#include "sighandler.hpp"

endian to_endian(const std::string& str)
{
    if(str == "host"){
        return endian::HOST;
    }else if(str == "big"){
        return endian::BIG;
    }else if(str == "little"){
        return endian::LITTLE;
    }else{
        errno = EINVAL;
        ERROR_THROW("invalid endian");
    }
}

const long target::page_size_ = sysconf(_SC_PAGESIZE);

target::target(const std::string& filename, target_role role,
        std::size_t offset, std::size_t length)
: ptr_to_fd_(new int(iohelper::open(filename.c_str(), select_file_flags(role),
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)), iohelper::close),
mmapped_data_(),
stat_(iohelper::fstat(*ptr_to_fd_)),
offset_(offset),
length_(init_length(length, role)),
page_offset_(offset_ & (static_cast<std::size_t>(page_size_) - 1))
{
    if(*ptr_to_fd_ == -1){
        ERROR_THROW(filename);
    }

    if(page_size_ == -1){
        ERROR_THROW("sysconf");
    }

    preprocess(role);

    if(length_ == 0){
        return;
    }

    int prot = 0;
    switch(role){
    case target_role::SRC: prot = PROT_READ;  break;
    case target_role::DST: prot = PROT_WRITE; break;
    default: ERROR_THROW(""); break;
    }

    mmap(prot);
}

target::target(int fd)
: ptr_to_fd_(new int(fd), [](int*){/* do nothing. */}),
mmapped_data_(),
stat_(iohelper::fstat(*ptr_to_fd_)),
offset_(),
length_(),
page_offset_()
{}

int target::transfer_to(const target& dest, const param& prm)const
{
    stopwatch sw(prm.verbose, std::string(__func__) + ": ");
    set_scheduling_policy(prm.scheduling_policy);
    set_signal_handler();

    if(iohelper::lseek(*ptr_to_fd_, 0, SEEK_SET) == -1){
        ERROR("lseek");
    }

    const std::size_t jobs = static_cast<std::size_t>(prm.jobs);

    if(mmapped_data_){
        if(dest.mmapped_data_){
            iohelper::memcpy(dest.offset(), offset(), std::min(length_, dest.length_),
                    prm.scheduling_policy, jobs);
        }else if(prm.hexdump_enabled){
            if(hexdump(*dest.ptr_to_fd_, mmapped_data_.get(),
                        offset_, length_, page_offset_, prm) != 0){
                ERROR("hexdump");
            }
        }else{
            switch(dest.stat_.st_mode & S_IFMT){
            case S_IFREG: case S_IFLNK:
                if(iohelper::pwrite(*dest.ptr_to_fd_, offset(), length_,
                            static_cast<off_t>(dest.length_), prm.scheduling_policy, jobs) == -1){
                    ERROR("pwrite");
                }
                dest.length_ += length_;
                break;
            case S_IFSOCK: case S_IFIFO: case S_IFCHR:
                if(iohelper::write(*dest.ptr_to_fd_, offset(), length_) == -1){
                    ERROR("write");
                }
                break;
            default:
                ERROR_THROW("unsupported st_mode");
                break;
            }
        }
    }else{
        if(dest.mmapped_data_){
            ssize_t ret;
            std::size_t count = 0ul;
            while((ret = iohelper::read(*ptr_to_fd_, dest.offset() + count, dest.length_ - count)) != 0){
                if(ret == -1){
                    ERROR("read");
                }
                count += static_cast<std::size_t>(ret);
            }
        }else{
            if(passthrough(dest) != 0){
                ERROR("passthrough");
            }
        }
    }

    if(dest.mmapped_data_){
        if(msync(mmapped_data_.get(), page_offset_ + length_, MS_SYNC) == -1){
            ERROR("msync");
        }
    }
    if(fsync(*dest.ptr_to_fd_) == -1){
        if(errno != EROFS && errno != EINVAL){
            ERROR("fsync");
        }
    }

    return 0;
}

void target::mmap(int prot)
{
    void* m = ::mmap(nullptr, page_offset_ + length_, prot, MAP_SHARED | MAP_POPULATE, *ptr_to_fd_,
            static_cast<off_t>(offset_ & ~(
                    static_cast<std::size_t>(page_size_) - 1)));

    if(m == MAP_FAILED){
        ERROR_THROW("mmap");
    }

    mmapped_data_.reset(reinterpret_cast<char*>(m),
            [&](char* p){::munmap(p, page_offset_ + length_);});
}

std::size_t target::init_length(std::size_t length, target_role role)
{
    switch(stat_.st_mode & S_IFMT){
    case S_IFREG:
    case S_IFLNK:
        if(role == target_role::SRC){
            return static_cast<std::size_t>(stat_.st_size);
        }
        return length;
    case S_IFSOCK:
    case S_IFBLK:
    case S_IFCHR:
    case S_IFIFO:
        return length;
    case S_IFDIR:
    default:
        ERROR_THROW("unsupported st_mode");
        break;
    }
}

void target::preprocess(target_role role)
{
    switch(stat_.st_mode & S_IFMT){
    case S_IFREG:
    case S_IFLNK:
        if(role == target_role::DST && iohelper::ftruncate(*ptr_to_fd_, 0) == -1){
            ERROR_THROW("ftruncate");
        }
        break;
    case S_IFSOCK:
    case S_IFBLK:
    case S_IFCHR:
    case S_IFIFO:
        break;
    case S_IFDIR:
    default:
        ERROR_THROW("unsupported st_mode");
        break;
    }
}

int target::passthrough(const target& dest)const
{
    const std::size_t buff_size = static_cast<std::size_t>(page_size_) * 16ul;
    std::shared_ptr<char[]> buff(new char[buff_size]);

    ssize_t r_ret;
    while((r_ret = iohelper::read(*ptr_to_fd_, buff.get(), buff_size)) != 0){
        if(r_ret == -1){
            ERROR("read");
        }
        const std::size_t r = static_cast<std::size_t>(r_ret);
        ssize_t w_ret = iohelper::write(*dest.ptr_to_fd_, buff.get(), r);
        if(w_ret == -1){
            ERROR("write");
        }
        switch(dest.stat_.st_mode & S_IFMT){
        case S_IFREG: case S_IFLNK: dest.length_ += r; break;
        default: break;
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
        ERROR_THROW(std::string("unsupported bit width: ") + std::to_string(width));
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
    default: ERROR_THROW(""); break;
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
        ERROR_BASE("write", /* do nothing */);
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
        ERROR("std::snprintf");
    }
    if(static_cast<std::size_t>(size_ - count_) <= static_cast<std::size_t>(printf_ret)){
        ERROR("truncating occurred in std::snprintf");
    }

    count_ += static_cast<decltype(count_)>(printf_ret);

    if(size_ * 8 / 10 < count_){
        /* if buf_ is filled 80% or more, flush it. */
        ssize_t ret = iohelper::write(fd_, buf_.get(), count_);
        if(ret == -1){
            ERROR("write");
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

ssize_t target::iohelper::pwrite(int fd, const void* buf, size_t count, off_t offset)
{
    std::size_t done = 0;
    ssize_t ret;
    do{
        do{
            ret = ::pwrite(fd, reinterpret_cast<const char*>(buf) + done,
                    count - done, offset + static_cast<off_t>(done));
        }while(ret == -1 && errno == EINTR);
        if(ret == -1){
            return ret;
        }
        done += static_cast<std::size_t>(ret);
    }while(done < count);
    return ret;
}

off_t target::iohelper::lseek(int fd, off_t offset, int whence)
{
    off_t ret = ::lseek(fd, offset, whence);
    if(ret == -1){
        if(errno != ESPIPE){
            return ret;
        }
        ret = 0;
    }
    return ret;
}

int target::iohelper::ftruncate(int fd, off_t length)
{
    int ret;
    do{
        ret = ::ftruncate(fd, length);
    }while(ret == -1 && errno == EINTR);
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

struct stat target::iohelper::fstat(int fd)
{
    struct stat buf;
    if(::fstat(fd, &buf) == -1){
        ERROR_THROW("fstat");
    }
    return buf;
}

void *target::iohelper::memcpy(void *dest, const void *src, size_t n, int sched_policy, size_t jobs)
{
    std::vector<std::thread> threads;
    const std::size_t len = n / jobs;

    char* d = reinterpret_cast<char*>(dest);
    const char* s = reinterpret_cast<const char*>(src);

    for(unsigned int i = 0; i < jobs; ++i){
        threads.emplace_back(std::thread([sched_policy](void* dp, const void* sp, std::size_t l){
            set_scheduling_policy(sched_policy);
            set_signal_handler();
            std::memcpy(dp, sp, l);
        }, d + i * len, s + i * len, len));
    }
    for(unsigned int i = 0; i < jobs; ++i){
        threads.at(i).join();
    }
    const std::size_t round = len * jobs;
    const std::size_t residue = n % jobs;
    std::memcpy(d + round, s + round, residue);

    return dest;
}

ssize_t target::iohelper::pwrite(int fd, const void* buf, size_t count, off_t offset, int sched_policy, size_t jobs)
{
    std::vector<std::thread> threads;
    const std::size_t len = count / jobs;

    const char* b = reinterpret_cast<const char*>(buf);

    for(unsigned int i = 0; i < jobs; ++i){
        threads.emplace_back(std::thread([fd, sched_policy](const void* bp, size_t cnt, off_t os){
            set_scheduling_policy(sched_policy);
            set_signal_handler();
            if(iohelper::pwrite(fd, bp, cnt, os) == -1){
                ERROR_THROW("pwrite");
            }
        }, b + i * len, len, offset + static_cast<off_t>(i * len)));
    }
    for(unsigned int i = 0; i < jobs; ++i){
        threads.at(i).join();
    }
    const std::size_t round = len * jobs;
    const std::size_t residue = count % jobs;
    if(iohelper::pwrite(fd, b + round, residue, static_cast<off_t>(round)) == -1){
        ERROR("pwrite");
    }

    return static_cast<ssize_t>(count);
}

// vim: set expandtab shiftwidth=0 tabstop=4 :
