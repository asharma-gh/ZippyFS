#include "block_cache.h"
#include "util.h"
using namespace std;
/**
 * TODO:
 * make all this actually work
 */
BlockCache::BlockCache(string path_to_shdw)
    : path_to_shdw_(path_to_shdw) {

}

int
BlockCache::write(string path, const uint8_t* buf, uint64_t size, uint64_t offset) {
    // create blocks for buf
    uint64_t num_blocks = Util::ulong_ceil(size, Block::get_logical_size());
    bool in_cache = file_cache_.find(path) == file_cache_.end();
    // check if path has blocks at those indexes
    // for each block, add to cache for file
    uint64_t curr_idx = 0;
    uint64_t block_size = 0;
    for (unsigned int block_idx = offset / Block::get_logical_size();  block_idx < num_blocks; block_idx++) {
        curr_idx += block_size;
        if (size < curr_idx + Block::get_logical_size())
            block_size = size - curr_idx;
        else
            block_size = Block::get_logical_size();
        if (in_cache) {
            // invalidate old block if it exists
            auto block_map = file_cache_.find(path)->second;
            if (block_map.find(block_idx) != block_map.end())
                block_map[block_idx]->set_dirty();
        }
        // finally create block with that much space at the current byte
        shared_ptr<Block> ptr(new Block(buf + curr_idx, block_size));

        // add newly formed block to file cache
        file_cache_[path][block_idx] = ptr;
    }


    // write meta data to cache_data
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
    return true;
}


int
flush_to_shdw() {
    return 0;
}
