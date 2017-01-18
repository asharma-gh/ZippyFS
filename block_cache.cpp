#include "block_cache.h"
using namespace std;

BlockCache::BlockCache(string path_to_shdw)
    : path_to_shdw_(path_to_shdw) {

}

int
BlockCache::write(string path, const uint8_t* buf, uint64_t size, uint64_t offset) {
    // create blocks for buf. Number of blocks = ceil(size / block_size)
    uint64_t num_blocks = (size + Block::get_logical_size() - 1) / Block::get_logical_size();
    // check if path has blocks at those indexes
    if (file_cache_.find(path) == file_cache_.end()) {
        // for each block, add to cache for file
        uint64_t cached_bytes = 0;
        uint64_t block_size;
        for (unsigned int block_count = 0; block_count < num_blocks; block_count++) {

            // first jump to latest byte
            cached_bytes += block_size;

            // then determine how much space we have to add
            if (cached_bytes + Block::get_logical_size() > size)
                block_size = size - cached_bytes;
            else {
                block_size = Block::get_logical_size();
            }

            // finally create block with that much space at the current byte
            shared_ptr<Block> ptr(new Block(buf + cached_bytes, block_size));

            // add newly formed block to file cache
            file_cache_[path][offset + block_count] = ptr;
        }


    }
    // file_cache_.insert( buf to block))
    // if it doesnt then just add it and blocks
    // else
    // check if blocks exist in map
    // mark existed blocks as dirty
    // overwrite, them


    // write meta data to cache_data
    return 0;

}

int
BlockCache::read(string path, const uint8_t* buf, uint64_t size, uint64_t offset) {
    (void)path;
    (void)buf;
    (void)size;
    (void)offset;
    // check if it exists
    // find file in file cache
    // get blocks starting from offset
    // transfer those to buf
    // return buf
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

BlockCache::~BlockCache() {
    // do stuff
}
