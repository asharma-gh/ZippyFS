#include "inode.h"
#include "util.h"
using namespace std;

Inode::Inode(string path)
    : path_(path) {
    nlink_ = 1;
    ctime_ = Util::get_time();
    size_ = 0;
}


void Inode::set_mode(uint32_t mode) {
    mode_ = mode;
}

void Inode::inc_link(std::string ref) {
    links_.push_back(ref);
    nlink_++;
}

void Inode::set_mtime(unsigned long long mtime) {
    mtime_ = mtime;
}

void Inode::set_size(unsigned long long size) {
    size_ = size;
}
uint64_t Inode::get_size() {
    return size_;
}


string Inode::get_record() {
    return path_ + " " + to_string(mode_) + " " + to_string(mtime_) + "0";
}
