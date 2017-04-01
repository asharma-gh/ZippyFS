#include "block.h"
#include <iostream>
using namespace std;

Block::Block() {
    actual_size_ = 0;
    dirty_ = false;
    data_ = {0};
}

Block::Block(const uint8_t* data, uint64_t size) {
    actual_size_ = 0;
    data_ = {0};
    insert_data(data, size, 0);
}


int
Block::insert(const uint8_t* data, uint64_t size, uint64_t offset) {
    insert_data(data, size, offset);
    return 0;
}

void
Block::insert_data(const uint8_t* data, uint64_t size, uint64_t offset) {
    cout << "writing.." << endl;
    for (unsigned int ii = offset, jj = 0; jj < size && ii < SIZE_; ii++, jj++) {
        data_[ii] = data[jj];
    }
    if (size + offset > actual_size_) {
        actual_size_ = size + offset;
    }


}
const
array<uint8_t, SIZE_>
Block::get_data_ar() {
    return data_;
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
    return vector<uint8_t>(data_.begin(), data_.begin() + actual_size_);
}
