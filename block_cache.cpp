#include "block_cache.h"
#include "util.h"
using namespace std;

BlockCache::BlockCache(string path_to_shdw)
    : path_to_shdw_(path_to_shdw) {

}

int
BlockCache::write(string path, const uint8_t* buf, uint64_t size, uint64_t offset) {
    // create blocks for buf. Number of blocks = ceil(size / block_size)
    uint64_t num_blocks = Util::ulong_ceil(size, Block::get_logical_size());
    bool in_cache = file_cache_.find(path) == file_cache_.end();
    // check if path has blocks at those indexes
    // for each block, add to cache for file
    uint64_t cached_bytes = 0;
    uint64_t block_size = 0;
    for (unsigned int block_count = 0; block_count < num_blocks; block_count++) {

        // first jump to latest byte
        cached_bytes += block_size;

        // then determine how much space we have to add
        if (cached_bytes + Block::get_logical_size() > size)
            block_size = size - cached_bytes;
        else
            block_size = Block::get_logical_size();

        if (in_cache) {
            // invalidate old block
            file_cache_[path][offset + cached_bytes]->set_dirty();
        }

        // finally create block with that much space at the current byte
        shared_ptr<Block> ptr(new Block(buf + cached_bytes, block_size));

        // add newly formed block to file cache
        file_cache_[path][offset + cached_bytes] = ptr;
    }


    // write meta data to cache_data
    return 0;

}

int
BlockCache::read(string path, uint8_t* buf, uint64_t size, uint64_t offset) {
    (void)path;
    (void)buf;
    (void)size;
    (void)offset;
    uint64_t read_bytes = 0;
    auto data = file_cache_.find(path)->second;
    for (unsigned int ii = offset; ii < offset + size; ii += read_bytes) {
        if (data.find(ii) == data.end())
            printf("COULD NOT LOCATE BLOCK\n");
        auto block = (data.find(ii))->second;
        auto block_data = block->get_data();
        for (auto byte : block_data) {
            buf[read_bytes++] = byte;
        }

    }
    assert(read_bytes == size);

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
