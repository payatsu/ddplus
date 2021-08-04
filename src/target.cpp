#include "target.hpp"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "misc.hpp"

const long target::page_size_ = sysconf(_SC_PAGESIZE);

target::target(const char* filename, std::size_t offset, std::size_t length)
: ptr_to_fd_(new int(open(filename, O_RDWR | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH)), close),
mmapped_data_(),
preamble_(),
pa_length_()
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
        offset = 0u;
        length = static_cast<std::size_t>(buf.st_size);
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

    const std::size_t pa_offset = offset & ~(static_cast<std::size_t>(page_size_) - 1);
    preamble_ = offset - pa_offset;
    pa_length_ = length + preamble_;

    if(length == 0){
        return;
    }

    void* m = mmap(nullptr, pa_length_,
            PROT_READ | PROT_WRITE, MAP_SHARED, *ptr_to_fd_,
            static_cast<off_t>(offset & ~(
                    static_cast<std::size_t>(page_size_) - 1)));

    if(m == MAP_FAILED){
        ERROR_THROW("", "mmap");
    }

    mmapped_data_.reset(reinterpret_cast<char*>(m),
            [this](char* p){munmap(p, this->pa_length_);});
}

target::target(int fd)
: ptr_to_fd_(new int(fd), [](int*){/* do nothing. */}),
mmapped_data_(),
preamble_(),
pa_length_()
{}

int target::transfer_to(const target& dest)const
{
    if(mmapped_data_){
        if(dest.mmapped_data_){
            std::memcpy(dest.offset(), offset(), std::min(length(), dest.length()));
        }else{
            std::size_t write_count = 0;
            do{
                ssize_t w_ret = write(*dest.ptr_to_fd_,
                        offset() + write_count, length() - write_count);
                if(w_ret == -1){
                    if(errno == EINTR){
                        continue;
                    }
                    ERROR("", "write");
                }
                write_count += static_cast<std::size_t>(w_ret);
            }while(write_count < length());
        }
    }else{
        const std::size_t buff_size = static_cast<std::size_t>(page_size_) * 16ul;
        std::shared_ptr<char[]> buff(new char[buff_size]);

        ssize_t r_ret;
        std::size_t transfer_count = 0ul;
        while((r_ret = read(*ptr_to_fd_, buff.get(), buff_size)) != 0){
            if(r_ret == -1){
                if(errno == EINTR){
                    continue;
                }
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
                std::size_t write_count = 0;
                do{
                    ssize_t w_ret = write(*dest.ptr_to_fd_,
                            buff.get() + write_count, r - write_count);
                    if(w_ret == -1){
                        if(errno == EINTR){
                            continue;
                        }
                        ERROR("", "write");
                    }
                    write_count += static_cast<std::size_t>(w_ret);
                }while(write_count < r);
            }
        }
    }
    return 0;
}

void target::close(int* fd_ptr)
{
    if(0 <= *fd_ptr){
        while(true){
            int ret = ::close(*fd_ptr);
            if(ret != -1 || errno != EINTR){
                break;
            }
        }
    }
    delete fd_ptr;
}

// vim: expandtab shiftwidth=0 tabstop=4 :
