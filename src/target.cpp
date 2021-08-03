#include "target.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "misc.hpp"

const long target::page_size_ = sysconf(_SC_PAGESIZE);

target::target(const char* filename, std::size_t offset, std::size_t length)
: ptr_to_fd_(new int(open(filename, O_RDWR)), close),
p_(),
preamble_(offset - (offset & ~(static_cast<std::size_t>(page_size_) - 1))),
pa_length_(length + preamble_)
{
    if(*ptr_to_fd_ == -1){
        ERROR_THROW("", filename);
    }

    if(page_size_ == -1){
        ERROR_THROW("", "sysconf");
    }

    void* m = mmap(nullptr, pa_length_,
            PROT_READ | PROT_WRITE, MAP_SHARED, *ptr_to_fd_,
            static_cast<off_t>(offset & ~(
                    static_cast<std::size_t>(page_size_) - 1)));

    if(m == MAP_FAILED){
        ERROR_THROW("", "mmap");
    }

    p_.reset(reinterpret_cast<char*>(m),
            [this](char* p){munmap(p, this->pa_length_);});
}

void target::close(int* fd_ptr)
{
    if(0 <= *fd_ptr){
        ::close(*fd_ptr);
    }
    delete fd_ptr;
}

// vim: expandtab shiftwidth=0 tabstop=4 :
