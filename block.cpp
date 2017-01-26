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
    cout << "inserting data with size " << size << " offset " << offset << endl;
    cout << "Current size " << actual_size_ << endl;
    unsigned int ii = offset;
    for (unsigned int jj = 0; jj < size; ii++, jj++) {
        data_[ii] = data[jj];
    }
    if (size + offset > actual_size_) {
        actual_size_ = size + offset;
        cout << "changed size to " << actual_size_ << endl;
    }

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
