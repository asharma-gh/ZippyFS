#include "bplus_index_loader.h"
#include "util.h"
#include <sys/mman.h>
using namespace std;

BPLUSIndexLoader::BPLUSIndexLoader(string path) {
    path_ = path;
    load_trees();
}

BPLUSIndexLoader::BPLUSIndexLoader() {

}

void
BPLUSIndexLoader::load_trees() {
    DIR* tree_dir = opendir(path_.c_str());
    struct dirent* entry;
    while ((entry = readdir(tree_dir))) {
        // check if valid tree file
        // mmap it to the unordered map
        string ent = entry->d_name;
        if (ent.substr(0,5).compare("TREE-") != 0)
            continue;
        if (file_to_mem_.find(ent) != file_to_mem_.end())
            continue;

        string fpath = path_ + ent;

        cout << "fpath: " << fpath << endl;
        int fd = ::open(fpath.c_str(), O_RDONLY);

        file_to_fd_[fpath] = fd;
        // get file size
        struct stat st;
        stat(fpath.c_str(), &st);
        uint64_t fsize = st.st_size;
        file_to_size_[fpath] = fsize;

        // load memory
        BPLUSIndex::node* fmem = (BPLUSIndex::node*)mmap(0, fsize*sizeof(char), PROT_READ, MAP_SHARED, fd, 0);

        // add pointer to map
        file_to_mem_[fpath] = fmem;
    }
}

BPLUSIndexLoader::disk_inode_info
BPLUSIndexLoader::find_latest_inode(string path, bool get_data) {
    load_trees();
    cout << "finding inode for " << path << " getdata? " << to_string(get_data) << endl;
    disk_inode_info latest;
    latest.i_mtime = 0;
    if (path.size() == 0)
        return latest;

    // create hash for path
    char phash[128] = {'\0'};
    string sthash = Util::crypto_hash(path);
    memcpy(phash, sthash.c_str(), sthash.size());
    cout << "PHASH: " << phash << endl;
    uint64_t latest_time = 0;
    for (auto tree : file_to_mem_) {
        cout << "TREE: " << tree.first << endl;

        // traverse thru tree
        BPLUSIndex::node* cur = (BPLUSIndex::node*)tree.second;
        int64_t inodeoff = 0;

        for(;;) {
            // check if this node contains this key
            for (int64_t ii = 0; ii < cur->num_keys; ii++) {
                // find this key
                int64_t keyof = cur->keys[ii];
                char* kmem = (char*)tree.second + keyof;

                if (strcmp(kmem, phash) == 0) {
                    // we found it
                    if (cur->is_leaf) {
                        cout << "FOUND IT " << endl;
                        inodeoff = cur->values[ii];
                        goto make_inode;
                    } else {
                        // its in a child
                        int64_t childof = cur->children[ii + 1];
                        cur = (BPLUSIndex::node*)((char*)tree.second + childof);
                        continue;

                    }
                }
            }
            if (cur->is_leaf)
                // then we will never get to it
                return latest;
            // this node does not have the key, find the correct child
            char* prev = (char*)tree.second + cur->keys[0];
            if (strcmp(phash, prev) < 0) {
                cur = (BPLUSIndex::node*)((char*)tree.second + cur->children[0]);
                continue;
            }
            char* post = (char*)tree.second + cur->keys[cur->num_keys - 1];
            if (strcmp(phash, post) > 0) {
                cur = (BPLUSIndex::node*)((char*)tree.second + cur->children[cur->num_keys - 1]);
                continue;
            }
            // then it must be somewhere in the middle
            for (int ii = 0; ii < cur->num_keys - 1; ii++ ) {
                char* pprev = (char*)tree.second + cur->keys[ii];
                char* ppost = (char*)tree.second + cur->keys[ii + 1];
                if (strcmp(pprev, phash) < 0
                        && strcmp(phash, ppost) < 0) {
                    cur = (BPLUSIndex::node*)((char*)tree.second + cur->children[ii + 1]);
                    continue;
                }
            }
            throw domain_error("we will never terminate");
        }
make_inode:
        // we found the entry
        // create inode
        BPLUSIndex::inode inode = *(BPLUSIndex::inode*)((char*)tree.second + inodeoff);
        if (inode.mtime > latest_time) {
            latest.i_mode = inode.mode;
            latest.i_size = inode.size;
            latest.i_nlink = inode.nlink;
            latest.i_mtime = inode.mtime;
            latest.i_ctime = inode.ctime;
            latest.i_inode_id = phash;
            latest.i_deleted = inode.deleted;
            latest_time = inode.mtime;
        }
        if (get_data) {
            cout << "Getting data..." << endl;
            int64_t bd_off = inode.block_data;
            if (bd_off == -1)
                // no data
                break;
            uint64_t bd_size = inode.block_data_size;
            cout << "BD OFF: " << to_string(bd_off) << " bd_size: " << to_string(bd_size) << endl;
            for (uint64_t ii = bd_off; ii < bd_off + bd_size; ii += sizeof(BPLUSIndex::block_data)) {
                // get block
                BPLUSIndex::block_data b = *(BPLUSIndex::block_data*)((char*)tree.second + ii);
                cout << "GETTING BLOCK!!!" <<endl;
                cout << "boff: " << to_string(b.data_offset) << " bsize: " << to_string (b.size);

                // add block if its a later version
                if (latest.i_block_time.find(b.block_id)
                        == latest.i_block_time.end()
                        || (latest.i_block_time.find(b.block_id) != latest.i_block_time.end()
                            && latest.i_block_time[b.block_id] <= b.mtime)) {
                    // add it, its a new block!
                    latest.i_block_time[b.block_id] = b.mtime;
                    shared_ptr<Block> bp = make_shared<Block>((uint8_t*)((char*)cur + b.data_offset), b.size);

                    latest.i_block_data[b.block_id] = bp;
                }
            }
        }
    }
    return latest;
}

BPLUSIndexLoader::~BPLUSIndexLoader() {
    cout << "destroying loader.." << endl;
    for (auto ent : file_to_mem_) {
        munmap(ent.second, file_to_size_[ent.first]);
        close(file_to_fd_[ent.first]);
    }

}
