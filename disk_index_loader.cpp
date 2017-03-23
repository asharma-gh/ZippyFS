#include "disk_index_loader.h"
#include "util.h"
#include <sys/mman.h>
using namespace std;

DiskIndexLoader::DiskIndexLoader(string path) {
    path_ = path;
    load_trees();
}
DiskIndexLoader::DiskIndexLoader() {

}
void
DiskIndexLoader::load_trees() {

    DIR* tree_dir = opendir(path_.c_str());
    struct dirent* entry;

    while ((entry = readdir(tree_dir))) {
        cout << "Looking at " << entry->d_name << endl;
        string ent = entry->d_name;
        if (ent.substr(0,5).compare("TREE-") != 0)
            continue;
        if (file_to_mem_.find(ent) != file_to_mem_.end())
            continue;

        // we have a new tree file to map
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
        DiskIndex::node* fmem = (DiskIndex::node*)mmap(0, fsize*sizeof(char), PROT_READ, MAP_SHARED, fd, 0);

        // add pointer to map
        file_to_mem_[fpath] = fmem;

    }
    closedir(tree_dir);
}

DiskIndexLoader::disk_inode_info
DiskIndexLoader::find_latest_inode(std::string path, bool get_data) {
    load_trees();
    disk_inode_info latest;
    latest.i_mtime = 0;
    if (path.size() == 0)
        return latest;
    // hash for comparisons
    char phash[512] = {'\0'};
    string sthash = Util::crypto_hash(path);
    memcpy(phash, sthash.c_str(), sthash.size());
    cout << "PHASH: " << phash << endl;
    uint64_t latest_time = 0;

    // for each tree we got, find this path
    for (auto tree : file_to_mem_) {
        cout << "TREE: " << tree.first << endl;

        // traverse tree
        DiskIndex::node* cur = tree.second;

        while (true) {
            cout << "Cur l: " << to_string(cur->left) << endl;
            cout << "Cur r: " << to_string(cur->right) << endl;
            // check this node's inode's hash
            DiskIndex::inode inode = cur->ent;
            int64_t hashoffset = inode.hash;

            // get hash
            char* hash = (char*)tree.second + hashoffset;
            cout << "THASH: " << hash << endl;
            int res = memcmp(phash, hash, 512);
            cout << "RES: " << res << endl;
            // compare hashes, explore accordingly
            if (res == 0) {
                cout << "We found it!" << endl;
                cout << "initializing inode..." << endl;

                // update inode info for newer version
                if (inode.mtime > latest_time) {
                    latest.i_mode = inode.mode;
                    latest.i_size = inode.size;
                    latest.i_nlink = inode.nlink;
                    latest.i_mtime = inode.mtime;
                    latest.i_ctime = inode.ctime;
                    latest.i_inode_id = hash;
                    latest.i_deleted = inode.deleted;
                    latest_time = inode.mtime;
                }

                // get blocks
                if (get_data) {
                    cout << "ADDDDING DATA!!!!!" << endl;
                    int64_t bd_off = inode.block_data;
                    if (bd_off == -1)
                        // no data
                        break;
                    uint64_t bd_size = inode.block_data_size;
                    cout << "BD OFF: " << to_string(bd_off) << " bd_size: " << to_string(bd_size) << endl;
                    for (uint64_t ii = bd_off; ii < bd_off + bd_size; ii += sizeof(DiskIndex::block_data)) {
                        // get block
                        DiskIndex::block_data b = *(DiskIndex::block_data*)((char*)cur + ii);
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

                break;
            }

            if (res > 0 && cur->right == -1) {
                cout << "not in tree, can't explore right" << endl;
                break;
            } else if (res > 0 && cur->right != -1) {
                int64_t noffset = cur->right;
                cout << "noffset: " << to_string(noffset) << endl;
                cur = (DiskIndex::node*)((char*)tree.second + noffset);
                // keep exploring
                continue;
            }

            // now we need to explore the left
            if (res < 0 && cur->left == -1) {
                cout << "not in tree, cant explore left" << endl;
                break;
            } else if (res < 0 && cur->left != -1) {
                int64_t loffset = cur->left;
                cout << "loffset: " << to_string(loffset) << endl;
                cur = (DiskIndex::node*)((char*)tree.second + loffset);
                continue;
            }

        }
        // if we found this path, record info in inode

        // if we have one and this one is later, update
        //
        // update blocks for each one if new
        //
    }
    return latest;
}


DiskIndexLoader::~DiskIndexLoader() {
    cout << "4" << endl;
    for (auto ent : file_to_mem_) {
        munmap(ent.second, file_to_size_[ent.first]);
        close(file_to_fd_[ent.first]);
    }
    cout << "5" << endl;

}