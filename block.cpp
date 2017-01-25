#include "block.h"

using namespace std;

Block::Block() {

}
Block::Block(const uint8_t* data, uint64_t size) {
    insert_data(data, size, 0);
}


int
Block::insert(const uint8_t* data, uint64_t size, uint64_t offset) {
    insert_data(data, size, offset);
    return 0;
}

void
Block::insert_data(const uint8_t* data, uint64_t size, uint64_t offset) {
    for (unsigned int ii = offset, jj = 0; jj < size; ii++, jj++) {
        data_[ii] = data[jj];
    }
    if (size + offset > actual_size_)
        actual_size_ = size + offset;

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
