#include "block_cache.h"
#include "util.h"
//#include "inode.h"
#include "fuse_ops.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <cstdio>
#include <unistd.h>
using namespace std;
/**
 * TODO:
 * make all this actually work
 */
BlockCache::BlockCache(string path_to_shdw)
    : path_to_shdw_(path_to_shdw) {}

int
BlockCache::write(string path, const uint8_t* buf, uint64_t size, uint64_t offset) {
    // create blocks for buf
    uint64_t num_blocks = Util::ulong_ceil(size, Block::get_logical_size());
    bool loaded_in = in_cache(path);
    // for this file, make a block and add it to cache
    uint64_t curr_idx = 0;
    uint64_t block_size = 0;
    for (unsigned int block_idx = offset / Block::get_logical_size();  block_idx < num_blocks; block_idx++) {
        curr_idx += block_size;
        if (size < curr_idx + Block::get_logical_size())
            block_size = size - curr_idx;
        else
            block_size = Block::get_logical_size();

        // invalidate old block if it exists
        if (loaded_in) {
            auto block_map = file_cache_.find(path)->second;
            if (block_map.find(block_idx) != block_map.end())
                block_map[block_idx]->set_dirty();
        }
        // finally create block with that much space at the current byte
        shared_ptr<Block> ptr(new Block(buf + curr_idx, block_size));
        // add newly formed block to file cache
        file_cache_[path][block_idx] = ptr;
    }
    assert(curr_idx + block_size == size);

    // record meta data to cache_data
    cache_data_[path] = (path + " [RW] " + to_string(Util::get_time()) +  " 0");
    return 0;
}

int
BlockCache::read(string path, uint8_t* buf, uint64_t size, uint64_t offset) {
    // get blocks
    uint64_t read_bytes = 0;
    bool offsetted = false;
    auto data = file_cache_.find(path)->second;
    auto num_blocks = data.size();
    for (unsigned int block_idx = offset / Block::get_logical_size(); block_idx < num_blocks; block_idx++) {
        // we can read this block, find the data
        auto block = data.find(block_idx)->second;
        auto block_data = block->get_data();
        // offset into the data and add all to buf
        // should only offset once
        auto offset_amt = 0;
        if (offsetted == false) {
            offset_amt = offset < Block::get_logical_size() ? offset : (offset % Block::get_logical_size());
            offsetted = true;
        }
        for (auto byte = block_data.begin() + offset_amt;
                byte != block_data.end() || read_bytes < size; byte++) {
            buf[read_bytes++] = *byte;
        }
    }
    assert(read_bytes == size);
    return 0;
}

bool
BlockCache::in_cache(string path) {
    (void)path;
    return file_cache_.find(path) != file_cache_.end();
}

int
BlockCache::flush_to_shdw() {
    // make index file for cache
    string idx_path = path_to_shdw_ + "/index.idx";
    int idx_fd = open(idx_path.c_str(), O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    if (idx_fd == -1)
        perror("Open failed");

    // create files for each item in cache
    for (auto const& entry : file_cache_) {
        // load previous version to shadow director

        //load_to_cache(killme);

        foobar(entry.first.c_str());
        // create path to file in shadow dir
        string shdw_file_path = path_to_shdw_ + (entry.first).substr(1);
        // get permissions of prev version
        struct stat st;
        mode_t mode = 0;
        int res = stat(shdw_file_path.c_str(), &st);
        if (res == -1)
            mode = S_IRUSR | S_IWUSR;
        else
            mode = st.st_mode;
        // open previous version / make new one
        int file_fd = open(shdw_file_path.c_str(), O_CREAT | O_WRONLY, mode);
        // write blocks to it
        for (auto const& data : entry.second) {
            // extract information for current block
            uint64_t block_idx = data.first;
            shared_ptr<Block> block = data.second;
            vector<uint8_t> block_data = block->get_data();
            uint64_t block_size = block->get_actual_size();
            // create a literal buffer for writes to file
            char buf[block_size];
            int ii = 0;
            for (auto byte : block_data) {
                buf[ii] = byte;
                ii++;
            }
            // do a write to file, offsetted based on block idx
            if (pwrite(file_fd, buf, block_size, block_idx * Block::get_logical_size()) == -1)
                perror("Error flushing block to file\n");

        }
        close(file_fd);
        // record to index file
        const char* record = cache_data_[entry.first].c_str();
        ::write(idx_fd, record, strlen(record));

    }
    close(idx_fd);
    return 0;
}
