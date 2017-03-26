#include "bplus_index.h"
#include "util.h"
using namespace std;
BPLUSIndex::BPLUSIndex(uint64_t num_ents) {
    num_ents_ = num_ents;
    // generate path
    string name = "/home/arvin/FileSystem/zipfs/o/dir/root/TREE-"+ Util::generate_rand_hex_name();
    // initialize memory zone
    mem_ = TireFire(name);
}

void BPLUSIndex::add_inode(Inode in, map<uint64_t, shared_ptr<Block>> dirty_blocks,
                           map<uint64_t, unsigned long long> block_mtime) {
    cout << "INSERTING " << in.get_id() << endl;
    if (root_ptr_ == NULL) {
        rootidx_ = mem_.get_tire(sizeof(node));
        root_ptr_ = (node*)mem_.get_memory(rootidx_);
        root_ptr_->is_leaf = 1;
        root_ptr_->num_keys = 1;

        // allocate memory for all inodes
        inode_arr_idx_ = mem_.get_tire(sizeof(inode) * num_ents_);
        inode_arr_ptr_ = (inode*)mem_.get_memory(inode_arr_idx_);

        // insert this inode into the root
        root_ptr_->values_size = 1;
        return;
    }

}

BPLUSIndex::~BPLUSIndex() {
    mem_.end();
}
