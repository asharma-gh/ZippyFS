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

Inode
DiskIndexLoader::find_latest_inode(std::string path) {
    load_trees();
    Inode latest(path);

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
            // find hash in memory

            // get hash
            char* hash = (char*)tree.second + hashoffset;
            cout << "THASH: " << hash << endl;
            int res = memcmp(phash, hash, 512);
            cout << "RES: " << res << endl;
            // compare hashes, explore accordingly
            if (res == 0) {
                cout << "We found it!" << endl;
                cout << "initializing inode..." << endl;
                if (inode.mtime > latest_time) {
                    latest.set_mode(inode.mode);
                    latest.set_size(inode.size);
                    latest.set_nlink(inode.nlink);
                    latest.set_mtime(inode.mtime);
                    latest.set_ctime(inode.ctime);
                    if (inode.deleted)
                        latest.delete_inode();

                    break;
                }
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
