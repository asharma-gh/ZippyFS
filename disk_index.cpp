#include "disk_index.h"
#include "util.h"
using namespace std;

DiskIndex::DiskIndex() {

    // generate name for this tree
    // generate path
    string name = "/home/arvin/FileSystem/zipfs/o/dir/root/TREE-"; //+ Util::generate_rand_hex_name();
    // initialize memory zone
    mem_ = TireFire(name);
}

void
DiskIndex::add_inode(Inode in, map<uint64_t, shared_ptr<Block>> dirty_blocks) {
    if (root_ptr_ == nullptr) {
        // construct root
        int64_t rootidx = mem_.get_tire(sizeof(node));
        root_ptr_ = (node*)mem_.get_memory(rootidx);
        root_ptr_->left = -1;
        root_ptr_->right = -1;
    }
    // construct inode to persist
    int64_t inodeidx = mem_.get_tire(sizeof(inode));
    inode* ist = (inode*)mem_.get_memory(inodeidx);
    ist->mode = in.get_mode();
    ist->nlink = in.get_link();
    ist->mtime = in.get_ull_mtime();
    ist->ctime = in.get_ull_ctime();
    ist->size = in.get_size();
    ist->deleted = in.is_deleted();

    // construct hash for inode
    int64_t hashidx = mem_.get_tire(sizeof(char) * 512);
    char* hash = (char*)mem_.get_memory(hashidx);
    memset(hash, '\0', 512 * sizeof(char));
    string shash = Util::crypto_hash(in.get_path());
    memcpy(hash,  shash.c_str(), shash.size());

    ist->hash = hashidx;

    // construct blocks dirty blocks for inode
    int64_t blocksidx = mem_.get_tire(sizeof(block_data) * dirty_blocks.size());

    block_data* loblocks = (block_data*)mem_.get_memory(blocksidx);

    for (auto db : dirty_blocks) {
        block_data* cur = loblocks + db.first;
        cur->block_id = db.first;
        cur->size = db.second->get_actual_size();

        // construct data in memory
        int64_t bdataidx = mem_.get_tire(cur->size * sizeof(uint8_t));
        uint8_t* bdata = (uint8_t*)mem_.get_memory(bdataidx);
        auto bytes = db.second->get_data();
        for (uint32_t ii = 0; ii < cur->size; ii++) {
            bdata[ii] = bytes[ii];
        }
        cur->block_id = bdataidx;
    }

    ist-> block_data = blocksidx;

    // find spot in tree for this inode, based on the hash
    node* cur_node = root_ptr_;
    // keep going until we found a spot
    while (true) {
        if (cur_node->left == -1 && cur_node->right == -1) {
            // then it goes in the root
            cur_node->ent = *ist;
            break;
        }
        // if this isn't the case, then the root has something in it.
        inode ent = cur_node->ent;
        // get ent hash to compare
        char* ent_hash = (char*)mem_.get_memory(ent.hash);
        // if this hash is greater than the current, it goes in the right, else left
        bool cmphash = strcmp(ent_hash, shash.c_str()) < 0;
        if (cmphash && cur_node->right == -1) {
            // then it goes here, make a new node
            int64_t nnode = mem_.get_tire(sizeof(node));
            node* nmem = (node*)mem_.get_memory(nnode);
            nmem->left = -1;
            nmem->right = -1;
            nmem->ent = *ist;
            cur_node->right = nnode;
            break;
        } else if (cmphash && cur_node->right != -1) {
            // then there is something else here to explore
            cur_node = (node*)mem_.get_memory(cur_node->right);
            continue;
        }
        // if we reached here, then the hash is less than the current.
        if (cur_node->left == -1) {
            // then it goes here, make a new node
            int64_t nnode = mem_.get_tire(sizeof(node));
            node* nmem = (node*)mem_.get_memory(nnode);
            nmem->left = -1;
            nmem->right = -1;
            nmem->ent = *ist;
            cur_node->left = nnode;
            break;
        } else {
            // we need to explore the left
            cur_node = (node*)mem_.get_memory(cur_node->left);
        }
    }
}
DiskIndex::~DiskIndex() {
    mem_.flush_head();
}
