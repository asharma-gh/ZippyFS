#include "block.h"
using namespace std;

Block::Block() {

}
Block::Block(const uint8_t* data, uint64_t size) {
    if (size > get_logical_size())
        throw domain_error("goof");
    insert_data(data, size);
}


int
Block::insert(const uint8_t* data, uint64_t size) {
    if (has_data_)
        return -1;
    else
        insert_data(data, size);
    return 0;
}

void
Block::insert_data(const uint8_t* data, uint64_t size) {
    for (unsigned int i = 0; i < size; i++) {
        data_[i] = data[i];
    }
    has_data_ = true;
    actual_size_ = size;

}

void
Block::set_dirty() {
    dirty_ = true;
}

bool
Block::is_dirty() {
    return dirty_;
}

uint64_t
Block::get_actual_size() {
    return actual_size_;
}

vector<uint8_t>
Block::get_data() {
    return vector<uint8_t>(data_.begin(), data_.end());
}
