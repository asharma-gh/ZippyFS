#include "tire_fire.h"
#include <sys/mman.h>

using namespace std;

TireFire::TireFire(string path)
    : file_ (path) {
    cur_size_ = 0;
    latest_index = 0;
    int fd = ::open(file_.c_str(), O_CREAT | O_WRONLY | O_RDONLY, S_IRWXU);
    cur_ptr_ = mmap(0, cur_size_, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);

}

uint32_t
TireFire::get_tire(size_t size) {
    // truncate the file to new length
    uint64_t old_size = cur_size_;
    cur_size_ += size;
    truncate(file_.c_str(), cur_size_);
    cur_ptr_ = mremap(cur_ptr_, old_size, cur_size_, MREMAP_MAYMOVE);

    //  change old ents
    for (auto ent : index_to_ptr) {
        index_to_ptr[ent.first] = (char*)cur_ptr_ + index_to_offset[ent.first];
    }

    index_to_offset[latest_index] = cur_size_;
    index_to_ptr[latest_index] = (char*)cur_ptr_ + old_size;

    return latest_index++;
}
void*
TireFire::get_memory(uint32_t index) {
    if (index_to_ptr.find(index) == index_to_ptr.end())
        return nullptr;
    return index_to_ptr[index];
}

TireFire::~TireFire() {
    // flush change
    msync(cur_ptr_, cur_size_, MS_INVALIDATE | MS_ASYNC);
}
