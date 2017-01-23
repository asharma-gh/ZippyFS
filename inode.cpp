#include "inode.h"
using namespace std;

Inode::Inode(string path)
    : path_(path) {
    nlink_ = 1;
}


void Inode::set_mode(uint32_t mode) {
    mode_ = mode;
}
void Inode::inc_link() {
    nlink_++;
}
void Inode::set_mtime(unsigned long long mtime) {
    mtime_ = mtime;
}

void Inode::set_blocks(shared_ptr<vector<shared_ptr<Block>>> blocks) {
    blocks_ = *blocks;
}
