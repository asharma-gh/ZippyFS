#include "block_cache.h"
using namespace std;

BlockCache::BlockCache(string path_to_shdw)
    : path_to_shdw_(path_to_shdw) {

}

int
BlockCache::write(string path, const uint8_t* buf, uint64_t size, uint64_t offset) {
    return 0;

}

int
BlockCache::read(string path, const uint8_t* buf, uint64_t size, uint64_t offset) {
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
