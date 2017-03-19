#include "tire_fire.h"
#include <sys/mman.h>
#include <string.h>
using namespace std;

TireFire::TireFire(string path)
    : file_ (path) {
    cur_size_ = 0;
    latest_index = 0;
    cur_ptr_ = nullptr;


}

uint64_t
TireFire::get_tire(size_t size) {
    uint64_t old_size = cur_size_;
    if (cur_ptr_ == nullptr) {
        fd_ = ::open(file_.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
        truncate(file_.c_str(), size);
        cur_ptr_ = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (cur_ptr_ == (void*)-1)
            cout << "ERROR MMAP! ERRNO: " << strerror(errno) << endl;
        cur_size_ = size;
        // only do this on the first go
        goto addtomap;
    }

    // truncate the file to new length
    cur_size_ += size;
    truncate(file_.c_str(), cur_size_);
    cur_ptr_ = mremap(cur_ptr_, old_size, cur_size_, MREMAP_MAYMOVE);
    if (cur_ptr_ == (void*)-1)
        cout << "ERROR MMAP get tire! ERRNO: " << strerror(errno) << endl;

    //  change old ents
    for (auto ent : index_to_ptr) {
        index_to_ptr[ent.first] = (char*)cur_ptr_ + index_to_offset[ent.first];
    }

addtomap:
    index_to_offset[latest_index] = old_size;
    index_to_ptr[latest_index] = (char*)cur_ptr_ + old_size;

    return latest_index++;
}

void*
TireFire::get_memory(uint32_t index) {
    if (index_to_ptr.find(index) == index_to_ptr.end())
        return nullptr;
    return index_to_ptr[index];
}

void
TireFire::flush_head() {

    // make header file to offset into the other one
    string head = file_ + ".head";

    TireFire fl(head);
    cout << "num head bytes: " << to_string(index_to_offset.size() * sizeof(uint64_t));
    auto ar = fl.get_tire(index_to_offset.size() * sizeof(uint64_t));
    uint64_t* mem = (uint64_t*)fl.get_memory(ar);
    for (auto ent : index_to_offset) {
        cout << "flushing " << to_string(ent.first) << " " << to_string(ent.second) << endl;
        mem[ent.first] = ent.second;
    }

    // destructor is called, will flush for us
}

TireFire::~TireFire() {
    cout << "Destroying.." << endl;
    close(fd_);
    // flush change
    msync(cur_ptr_, cur_size_, MS_INVALIDATE | MS_ASYNC);
    munmap(cur_ptr_, cur_size_);
}
