#include "block_cache.h"
using namespace std;

BlockCache::BlockCache(string path_to_shdw)
    : path_to_shdw_(path_to_shdw) {

}

int
BlockCache::write(string path, const uint8_t* buf, uint64_t size, uint64_t offset) {
    // create blocks for buf

    // check if path has blocks at those indexes
    if (file_cache_.find(path) == file_cache_.end()) {

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
    // check if it exists
    // find file in file cache
    // get blocks starting from offset
    // transfer those to buf
    // return buf
    return 0;
}

bool
BlockCache::in_cache(string path) {
    return true;
}

int
flush_to_shdw() {
    return 0;
}

BlockCache::~BlockCache() {
    // do stuff
}
