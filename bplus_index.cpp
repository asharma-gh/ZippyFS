#include "bplus_index.h"
#include "util.h"
using namespace std;
BPLUSIndex::BPLUSIndex(uint64_t num_ents) {
    num_ents_ = num_ents;
    // generate path
    string name = "/home/arvin/FileSystem/zipfs/o/dir/root/TREE-"+ Util::generate_rand_hex_name();
    // initialize memory zone
    mem_ = TireFire(name);
    inodes_idx_ = mem_.get_tire(sizeof(inode) * num_ents_);
}

void BPLUSIndex::add_inode(Inode in, map<uint64_t, shared_ptr<Block>> dirty_blocks,
                           map<uint64_t, unsigned long long> block_mtime) {
    (void)dirty_blocks;
    (void)block_mtime;
    cout << "INSERTING " << in.get_id() << endl;
    if (root_ptr_ == NULL) {
        rootidx_ = mem_.get_tire(sizeof(node));
        root_ptr_ = (node*)mem_.get_memory(rootidx_);
        root_ptr_->is_leaf = 1;
        root_ptr_->num_keys = 1;
        // construct key
        int64_t key_idx = mem_.get_tire(256 * sizeof(char));
        char* key = (char*)mem_.get_memory(key_idx);
        memset(key, '\0', 256 * sizeof(char));
        memcpy(key, in.get_id().c_str(), in.get_id().size());
        // allocate memory for all inodes
        inode_arr_idx_ = mem_.get_tire(sizeof(inode) * num_ents_);
        inode_arr_ptr_ = (inode*)mem_.get_memory(inode_arr_idx_);
        // refresh ptr for root
        root_ptr_ = (node*)mem_.get_memory(rootidx_);
        // insert this inode into the root
        cur_inode_arr_idx_ = 0;
        inode_arr_ptr_->mode = in.get_mode();
        inode_arr_ptr_->nlink = in.get_link();
        inode_arr_ptr_->mtime = in.get_ull_mtime();
        inode_arr_ptr_->ctime = in.get_ull_ctime();
        inode_arr_ptr_->size = in.get_size();
        inode_arr_ptr_->deleted = in.is_deleted();
        root_ptr_->values_size = 1;
        return;
    }

    // else find a node to insert it
    int64_t target_idx = find_node_to_store(in.get_id());

    // check if the node is full
    node* tnode = (node*)mem_.get_memory(target_idx);
    if (tnode->num_keys == ORDER - 1) {
        // needs to split the node to fit this item
        //split_insert_node(target, key, val, false, NULL);
        return;
    }
    // or else we can just insert it
    // insert_into_node(target, key, val, false, NULL);

}

int64_t
BPLUSIndex::find_node_to_store(string key) {
    if (root_ptr_ == nullptr)
        throw domain_error("tree is empty");

    node* cur = (node*)mem_.get_memory(rootidx_);
    int64_t cur_idx = rootidx_;

    for (;;) {
        cur = (node*)mem_.get_memory(cur_idx);
        if (cur->is_leaf)
            return cur_idx;

        // is this key before all of the ones in this node?
        char* cur_key = (char*)mem_.get_memory(cur->keys[0]);
        if (key.c_str() < cur_key) {
            cur_idx = before(cur, 0);
            continue;
        }

        // is this key after the ones in this node?
        cur_key = (char*)mem_.get_memory(cur->keys[cur->num_keys - 1]);
        if (key.c_str() >= cur_key) {
            cur_idx = after(cur, cur->num_keys - 1);
            continue;
        }

        // else it must be somewhere in between
        uint64_t inneridx = 0;
        for (int ii = 1; ii < cur->num_keys - 1; ii++) {
            char* prev = (char*)mem_.get_memory(cur->keys[ii]);
            char* post = (char*)mem_.get_memory(cur->keys[ii + 1]);
            if (prev <= key.c_str()
                    && key.c_str() < post)
                inneridx = ii;
        }
        cur_idx = after(cur, inneridx);
    }
}

int64_t
BPLUSIndex::before(node* n, int64_t idx) {
    return n->children[idx];
}

int64_t
BPLUSIndex::after(node* n, int64_t idx) {
    return n->children[idx];
}
int
BPLUSIndex::insert_into_node(int64_t nodeidx, int k, inode v, bool isleft, int64_t child) {
    // find spot
    int cur_idx = 0;
    node * n = (node*)mem_.get_memory(nodeidx);
    for (int ii = 0; ii < n->num_keys - 1; ii++) {

        if (n->keys[ii] > k) {
            cur_idx = ii;
        }
    }

    if (cur_idx == 0 && n->num_keys > 0) {
        cur_idx = n->num_keys;
    } else {
        // move everything from cur_idx + 1 up
        node temp = *n;
        for (int ii = cur_idx; ii < n->num_keys - 1; ii++) {
            n->keys[ii + 1] = temp.keys[ii];
            n->children[ii + 1] = temp.children[ii];
            // get value ptr
            inode* lo_inode = (inode*)mem_.get_memory(n->inodes);

            lo_inode[ii + 1] = lo_inode[ii];
            n = (node*)mem_.get_memory(nodeidx);
        }
    }
    n->keys[cur_idx] = k;
    n->num_keys++;
    n->values_size++;

    if (n->is_leaf) {
        // insert value into fixed node
        inode* lo_inode = (inode*)mem_.get_memory(n->inodes);
        n = (node*)mem_.get_memory(nodeidx);
        lo_inode[cur_idx] = v;
        n->children[cur_idx] = -1;

    } else {
        // insert child
        if (!isleft)
            cur_idx++;
        n->children[cur_idx] = child;
    }
    return cur_idx;
}
BPLUSIndex::~BPLUSIndex() {
    mem_.end();
}
