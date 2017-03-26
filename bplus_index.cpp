#include "bplus_index.h"
#include "util.h"
using namespace std;
BPLUSIndex::BPLUSIndex() {
    // generate path
    string name = "/home/arvin/FileSystem/zipfs/o/dir/root/TREE-"+ Util::generate_rand_hex_name();
    // initialize memory zone
    mem_ = TireFire(name);
}

void BPLUSIndex::add_inode(Inode in, map<uint64_t, shared_ptr<Block>> dirty_blocks,
                           map<uint64_t, unsigned long long> block_mtime) {

}

BPLUSIndex::~BPLUSIndex() {
    mem_.end();
}
