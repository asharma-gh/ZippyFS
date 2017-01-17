#include "block.h"
using namespace std;

Block::Block(string data) {
    insert_data(data);
}

int Block::insert(string data) {
    if (has_data)
        return -1;
    else
        insert_data(data);
    return 0;
}

void Block::insert_data(string data) {
    for (int i = 0; i < data.length(); i++) {
        data_[i] = (uint8_t)data[i];
    }
    has_data = true;

}

void Block::set_dirty() noexcept {
    dirty_ = true;
}

bool Block::is_dirty() const noexcept {
    return dirty_;
}
