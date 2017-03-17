#include "tire_fire.h"
#include <sys/mman.h>

using namespace std;

TireFire::TireFire(string path)
    : file_ (path) {
    cur_size_ = 0;
    int fd = ::open(file_.c_str(), O_CREAT | O_WRONLY | O_RDONLY, S_IRWXU);
    cur_ptr_ = mmap(0, cur_size_, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

    close(fd);

}

void*
TireFire::get_tire(size_t size) {
    // truncate the file to new length
    uint64_t old_size = cur_size_;
    cur_size_ += size;
    truncate(file_.c_str(), cur_size_);
    int fd = ::open(file_.c_str(), O_CREAT | O_WRONLY | O_RDONLY, S_IRWXU);
    cur_ptr_ = mremap(cur_ptr_, old_size, cur_size_, MREMAP_MAYMOVE);
    // return new region
    close(fd);
    return (char*)cur_ptr_ + old_size;
}

TireFire::~TireFire() {
    // flush change
    msync(cur_ptr_, cur_size_, MS_INVALIDATE | MS_ASYNC);
}
