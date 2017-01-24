#include "inode.h"
using namespace std;

Inode::Inode(string path)
    : path_(path) {
    nlink_ = 1;
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
string Inode::get_record() {
    return path_ + " " + mode + " " + mtime_ + "0";
}
