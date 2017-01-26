#include "inode.h"
#include "util.h"
#include "libgen.h"
#include <iostream>
using namespace std;

Inode::Inode(string path)
    : path_(path) {
    nlink_ = 1;
    ul_ctime_ = Util::get_time();
    ul_mtime_ = Util::get_time();
    fill_time(&ts_mtime_);
    fill_time(&ts_ctime_);
    size_ = 0;
    links_.insert(path);
}


void
Inode::set_mode(uint32_t mode) {
    if (S_ISDIR(mode))
        nlink_ = nlink_ < 2 ? 2 : nlink_;
    mode_ = mode;
}
uint32_t
Inode::get_mode() {
    return mode_;
}

void
Inode::inc_link(std::string ref) {
    links_.insert(ref);
    nlink_++;
}

void
Inode::dec_link(string path) {
    nlink_--;
    links_.erase(path);
    if (nlink_ == 0)
        blocks_.clear();
}
uint32_t
Inode::get_link() {
    return nlink_;
}

bool
Inode::has_block(uint64_t block_index) {
    cout << "checking if block " << block_index << endl;
    //  if (blocks_.find(1) == blocks_.end())
    //     cout << "ok" << endl;
    // cout << (blocks_.find(block_index) != blocks_.end()) << endl;
    return blocks_.find(block_index) != blocks_.end();
}

shared_ptr<Block>
Inode::get_block(uint64_t block_index) {
    return blocks_.find(block_index)->second;
}

void
Inode::update_mtime() {
    ul_mtime_ = Util::get_time();
    fill_time(&ts_mtime_);
}
unsigned long long
Inode::get_ull_mtime() {
    return ul_mtime_;
}

void
Inode::set_size(unsigned long long size) {
    size_ = size;
}

uint64_t
Inode::get_size() {
    return size_;
}

vector<string>
Inode::get_refs() {
    vector<string> t (links_.begin(), links_.end());
    return t;
}


string
Inode::get_record() {
    return (path_ + " " + to_string(mode_) + " " + to_string(ul_mtime_) + " 0\n");
}

void
Inode::add_block(uint64_t block_index, shared_ptr<Block> block) {
    if (blocks_.find(block_index) != blocks_.end())
        blocks_[block_index]->set_dirty();
    blocks_[block_index] = block;
}

void
Inode::remove_block(uint64_t block_index) {
    blocks_.erase(block_index);
}

int
Inode::stat(struct stat* stbuf) {
    stbuf->st_mode = mode_;
    stbuf->st_nlink = nlink_;
    stbuf->st_blocks = blocks_.size();
    stbuf->st_size = size_;
    stbuf->st_ctim = ts_ctime_;
    stbuf->st_mtim = ts_mtime_;
    return 0;
}

int
Inode::is_dir() {
    return S_ISDIR(mode_);
}

int
Inode::read(uint8_t* buf, uint64_t size, uint64_t offset) {
    cout << "READING FROM INODE " << endl;
    // get blocks
    uint64_t read_bytes = 0;
    bool offsetted = false;
    auto num_blocks = blocks_.size();
    cout << "SIZE " << size << " OFFSET " << offset << " DATA SIZE " << num_blocks << endl;
    for (unsigned int block_idx = offset / Block::get_logical_size(); block_idx < num_blocks && read_bytes < size; block_idx++) {
        // we can read this block, find the data
        auto block = blocks_.find(block_idx)->second;
        auto block_data = block->get_data();
        // offset into the data and add all to buf
        // should only offset once
        auto offset_amt = 0;
        if (offsetted == false) {
            offset_amt = offset < Block::get_logical_size() ? offset : (offset % Block::get_logical_size());
            offsetted = true;
        }
        cout << "BLOCK SIZE " << block->get_actual_size()
             << endl;
        uint64_t blksz = block->get_actual_size();
        uint64_t ii = offset_amt;
        for (auto byte = block_data.begin() + offset_amt;
                byte != block_data.end() && ii < blksz && read_bytes < size; ii++, byte++) {

            buf[read_bytes++] = *byte;
        }
    }
    //assert(read_bytes == size);
    cout << "DONE READING FROM INODE " << endl;
    cout << "READ BYTES: " << read_bytes << endl;
    return size;
}
int
Inode::flush_to_fd(int fd) {
    for (auto const& data : blocks_) {
        // extract information for current block
        uint64_t block_idx = data.first;
        shared_ptr<Block> block = data.second;
        vector<uint8_t> block_data = block->get_data();
        uint64_t block_size = block->get_actual_size();
        cout << "got block size" << endl;
        // create a literal buffer for writes to file
        char* buf = (char*)malloc(block_size * sizeof(char));
        memset(buf, 0, block_size * sizeof(char));
        cout << "BLOCK SIZE " << block_size;
        cout << " BUF SIZE " << sizeof(buf) << endl;
        for (uint64_t ii = 0;  ii < block_size; ii++) {
            buf[ii] = block_data[ii];
        }
        cout << "writing... " << endl;
        // do a write to file, offsetted based on block idx
        if (pwrite(fd, buf, block_size * sizeof(char), block_idx * Block::get_logical_size()) == -1)
            perror("Error flushing block to file\n");
        free(buf);
    }

    close(fd);
    return 0;
}
