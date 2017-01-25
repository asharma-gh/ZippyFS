#include "inode.h"
#include "util.h"
using namespace std;

Inode::Inode(string path)
    : path_(path) {
    nlink_ = 1;
    ul_ctime_ = Util::get_time();
    ul_mtime_ = Util::get_time();
    fill_time(&ts_mtime_);
    fill_time(&ts_ctime_);
    size_ = 0;
}


void Inode::set_mode(uint32_t mode) {
    mode_ = mode;
}

void Inode::inc_link(std::string ref) {
    links_.push_back(ref);
    nlink_++;
}

void Inode::update_mtime() {
    ul_mtime_ = Util::get_time();
    fill_time(&ts_mtime_);
}

void Inode::set_size(unsigned long long size) {
    size_ = size;
}

uint64_t Inode::get_size() {
    return size_;
}

vector<string> Inode::get_refs() {
    return links_;
}


string Inode::get_record() {
    return path_ + " " + to_string(mode_) + " " + to_string(ul_mtime_) + "0";
}

void Inode::add_block(uint64_t block_index, shared_ptr<Block> block) {
    if (blocks_.find(block_index) != blocks_.end())
        blocks_[block_index]->set_dirty();
    blocks_[block_index] = block;
}

void Inode::remove_block(uint64_t block_index) {
    blocks_.erase(block_index);
}

void Inode::stat(struct stat* st) {
    st->st_dev = 0;
    st->st_ino = -1;
    st->st_uid = getuid();
    st->st_gid = getgid();
    st->st_mode = mode_;
    st->st_nlink = nlink_;
    st->st_blocks = blocks_.size();
    st->st_size = size_;
    st->st_ctim = ts_ctime_;
    st->st_mtim = ts_mtime_;
}

int Inode::read(uint8_t* buf, uint64_t size, uint64_t offset) {
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
        cout << "BLOCK SIZE " << block_data.size()
             << endl;
        for (auto byte = block_data.begin() + offset_amt;
                byte != block_data.end() && read_bytes < size; byte++) {
            buf[read_bytes++] = *byte;
        }
    }
    assert(read_bytes == size);
    cout << "buffer "<< buf << endl;
    return size;
}
int Inode::flush_to_fd(int fd) {
    for (auto const& data : blocks_) {
        // extract information for current block
        uint64_t block_idx = data.first;
        shared_ptr<Block> block = data.second;
        vector<uint8_t> block_data = block->get_data();
        uint64_t block_size = block->get_actual_size();
        // create a literal buffer for writes to file
        char buf[block_size];
        cout << "BLOCK SIZE " << block_size;
        cout << " BUF SIZE " << sizeof(buf) << endl;
        for (uint64_t ii = 0;  ii < block_size; ii++) {
            buf[ii] = block_data[ii];
            cout << ii << endl;
        }
        // do a write to file, offsetted based on block idx
        if (pwrite(fd, buf, block_size, block_idx * Block::get_logical_size()) == -1)
            perror("Error flushing block to file\n");
    }

    close(fd);
    return 0;
}
