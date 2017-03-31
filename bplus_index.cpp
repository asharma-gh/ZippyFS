#include "bplus_index.h"
#include "util.h"
using namespace std;
BPLUSIndex::BPLUSIndex(uint64_t num_ents) {
    num_ents_ = num_ents;
    cout << "NUM ENTS: " << to_string(num_ents) << endl;
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
        cout << "ROOT OFFSET: " << to_string(mem_.get_offset(rootidx_));
        cout << "SIZEOF NODE: " << to_string(sizeof(node)) << endl;
        first_root_ = rootidx_;
        root_ptr_->is_leaf = 1;
        root_ptr_->num_keys = 1;
        cur_inode_arr_idx_ = 0;
        // allocate memory for all inodes
        inode_arr_idx_ = mem_.get_tire(sizeof(inode) * num_ents_);
        inode_arr_ptr_ = (inode*)mem_.get_memory(inode_arr_idx_);
    }

    // construct inode
    // construct key
    int64_t key_idx = mem_.get_tire(256 * sizeof(char));
    char* key = (char*)mem_.get_memory(key_idx);
    memset(key, '\0', 256 * sizeof(char));
    memcpy(key, in.get_id().c_str(), in.get_id().size());
    // insert this inode
    inode_arr_ptr_ = (inode*)mem_.get_memory(inode_arr_idx_);
    inode* cur_inode_ptr = (inode*)mem_.get_memory(inode_arr_idx_) + cur_inode_arr_idx_;
    cur_inode_ptr->mode = in.get_mode();
    cur_inode_ptr->nlink = in.get_link();
    cur_inode_ptr->mtime = in.get_ull_mtime();
    cur_inode_ptr->ctime = in.get_ull_ctime();
    cur_inode_ptr->size = in.get_size();
    cur_inode_ptr->deleted = in.is_deleted();
    cur_inode_ptr->hash = mem_.get_offset(key_idx);
    cur_inode_arr_idx_++;
    // initialize blocks
    uint64_t bd_size = sizeof(block_data) *  dirty_blocks.size();
    // construct blocks dirty blocks for inode
    int64_t blocksidx = mem_.get_tire(bd_size);


    for (auto db : dirty_blocks) {
        block_data* loblocks = (block_data*)mem_.get_memory(blocksidx);
        block_data* cur = loblocks + db.first;
        cur->block_id = db.first;
        cur->size = db.second->get_actual_size();

        // construct data in memory
        int64_t bdataidx = mem_.get_tire(cur->size * sizeof(uint8_t));
        uint8_t* bdata = (uint8_t*)mem_.get_memory(bdataidx);
        auto bytes = db.second->get_data();
        loblocks = (block_data*)mem_.get_memory(blocksidx);
        cur = loblocks + db.first;

        for (uint32_t ii = 0; ii < cur->size; ii++) {
            bdata[ii] = bytes[ii];

        }
        cur->data_offset = mem_.get_offset(bdataidx);
        cur->mtime = block_mtime[db.first];
    }
    // add block to inode
    inode_arr_ptr_ = (inode*)mem_.get_memory(inode_arr_idx_);
    cur_inode_ptr = inode_arr_ptr_ + cur_inode_arr_idx_;
    if (bd_size > 0) {
        cur_inode_ptr->block_data = mem_.get_offset(blocksidx);
        cur_inode_ptr->block_data_size = bd_size;
    } else
        cur_inode_ptr->block_data = -1;

    // insert into node w/ blocks info
    // else find a node to insert it
    uint64_t target_offset = find_node_to_store(in.get_id());


    // check if the node is full
    node* tnode = (node*)((char*)mem_.get_memory(first_root_) + target_offset);
    uint64_t inode_of = ((cur_inode_arr_idx_ - 1) * sizeof(inode)) + mem_.get_offset(inodes_idx_);
    if (tnode->num_keys == ORDER - 1) {
        // needs to split the node to fit this item
        split_insert_node(target_offset, key_idx, inode_of, false, -1);
        return;
    }
    // or else we can just insert it
    insert_into_node(target_offset, key_idx, inode_of, false, -1);

}

int64_t
BPLUSIndex::find_node_to_store(string key) {
    if (root_ptr_ == nullptr)
        throw domain_error("tree is empty");

    node* cur = nullptr;
    int64_t cur_offset = mem_.get_offset(first_root_);
    for (;;) {
        cur = (node*)((char*)mem_.get_memory(first_root_) + cur_offset);
        if (cur->is_leaf)
            return cur_offset;

        // is this key before all of the ones in this node?
        char* cur_key = (char*)mem_.get_memory(cur->keys[0]);
        if (strcmp(key.c_str(), cur_key) < 0) {
            cur_offset = before(cur, 0);
            continue;
        }

        // is this key after the ones in this node?
        cur_key = (char*)mem_.get_memory(cur->keys[cur->num_keys - 1]);
        if (strcmp(key.c_str(), cur_key) >= 0) {
            cur_offset = after(cur, cur->num_keys - 1);
            continue;
        }

        // else it must be somewhere in between
        uint64_t inneridx = 0;
        for (int ii = 1; ii < cur->num_keys - 1; ii++) {
            char* prev = (char*)mem_.get_memory(cur->keys[ii]);
            char* post = (char*)mem_.get_memory(cur->keys[ii + 1]);
            if (strcmp(prev, key.c_str()) <= 0
                    && strcmp(key.c_str(), post) < 0)
                inneridx = ii;
        }
        cur_offset = after(cur, inneridx);
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
BPLUSIndex::insert_into_node(uint64_t nodeoffset, int64_t k, int64_t v, bool isleft, int64_t child) {
    // find spot
    int cur_idx = 0;
    node * n = (node*)((char*)mem_.get_memory(first_root_) + nodeoffset);
    for (int ii = 0; ii < n->num_keys - 1; ii++) {
        // compare keys by string
        // TODO:
        char* oldk = (char*)mem_.get_memory(n->keys[ii]);
        char* newk = (char*)mem_.get_memory(k);
        if (strcmp(oldk, newk) > 0)
            cur_idx = ii;
    }

    if (cur_idx == 0 && n->num_keys > 0)
        cur_idx = n->num_keys;
    else {
        // move everything from cur_idx + 1 up
        node temp = *n;
        for (int ii = cur_idx; ii < n->num_keys - 1; ii++) {
            n->keys[ii + 1] = temp.keys[ii];
            n->children[ii + 1] = temp.children[ii];
            n->values[ii + 1] = temp.values[ii];

        }
    }
    n->keys[cur_idx] = k;
    n->num_keys++;
    n->values_size++;

    if (n->is_leaf) {
        // insert value into fixed node
        n->values[cur_idx] = v;
        n->children[cur_idx] = 0;

    } else {
        // insert child
        if (!isleft)
            cur_idx++;
        n->children[cur_idx] = mem_.get_offset(child);
    }
    return cur_idx;
}

int64_t
BPLUSIndex::split_insert_node(uint64_t nodeoffset, int64_t k, int64_t v, bool isparent, int64_t targ) {
    // get memory for node
    node* nnode = (node*)((char*)mem_.get_memory(first_root_) + nodeoffset);

    // do nothing if the node is not full
    if (nnode->num_keys != ORDER - 1)
        return nodeoffset;

    // get median key index
    uint64_t med_idx = ((ORDER - 1) / 2);

    // make new leaves
    int64_t rightidx = mem_.get_tire(sizeof(node));
    node* right = (node*)mem_.get_memory(rightidx);
    right->is_leaf = true;
    // copy stuff from median to end over to new node
    uint64_t ii;
    uint64_t cur_idx = 0;
    // refresh pointer
    nnode = (node*)((char*)mem_.get_memory(first_root_) + nodeoffset);
    int64_t med_key_idx = nnode->keys[med_idx];
    // if we are splitting a parent, don't include median on right
    if (isparent) {
        for (ii = med_idx + 1; ii < ORDER - 1; ii++, cur_idx++) {
            right->num_keys++;
            right->keys[cur_idx] = nnode->keys[ii];
            right->children[cur_idx] = nnode->children[ii];
            // delete now redundant entries
            nnode->keys[ii] = -1;
            nnode->num_keys--;
        }
        // remove med from left
        nnode->keys[med_idx] = -1;
        nnode->num_keys--;
        // right side is also a parent, insert the target (carried up) node
        right->is_leaf = false;
        insert_into_node(rightidx, k, v, false, targ);
        // set parent pointer in targ
        node* target = (node*)mem_.get_memory(targ);
        target->parent = mem_.get_offset(rightidx);

    } else {
        // we are splitting a child
        for (ii = med_idx; ii < ORDER - 1; ii++, cur_idx++) {
            right->num_keys++;
            right->keys[cur_idx] = nnode->keys[ii];
            right->children[cur_idx] = nnode->children[ii];
            // copy values over
            right->values[cur_idx] = nnode->values[ii];

            // delete redundant entries
            nnode->keys[ii] = -1;
        }
        nnode->num_keys = med_idx;

        // now that the leaf is split, insert into new side
        insert_into_node(rightidx, k, v, false, -1);
    }
    // fix parent pointers
    int64_t paridx = nnode->parent;

    int nkey = right->keys[0];
    if (isparent)
        nkey = med_key_idx;
    // make new root if parent is null
    if (paridx == -1) {
        // make new root
        int64_t nrootidx = mem_.get_tire(sizeof(node));
        node* nroot = (node*)mem_.get_memory(nrootidx);
        nroot->num_keys = 1;
        nroot->is_leaf = false;
        nroot->keys[0] = nkey;
        nroot->children[0] = nodeoffset;
        nroot->children[1] = mem_.get_offset(rightidx);
        right = (node*)mem_.get_memory(rightidx);
        right->parent = nrootidx;
        nnode = (node*)((char*)mem_.get_memory(first_root_) + nodeoffset);
        nnode->parent = nrootidx;
        rootidx_ = nrootidx;
        return nrootidx;
    }
    // parent exists
    node* pparent = (node*)mem_.get_memory(paridx);
    right = (node*)mem_.get_memory(rightidx);

    if (pparent->num_keys == ORDER - 1) {
        // no room, need to split
        paridx = split_insert_node(paridx, right->keys[0], v, true, rightidx);
    } else {
        insert_into_node(paridx, nkey, v, false, rightidx);
        right->parent = paridx;
    }
    return -1;
}

BPLUSIndex::~BPLUSIndex() {
    mem_.end();
}
