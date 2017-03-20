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
        if (ent.substr(0,5).compare("TREE-") != 0
                || ent.substr(ent.size() - 5).compare(".head") == 0)
            continue;
        if (file_to_mem_.find(ent) != file_to_mem_.end())
            continue;

        // we have a new tree file to map
        string fpath = path_ + ent;
        string hepath = fpath + ".head";
        cout << "fpath: " << fpath << endl;
        cout << "hepath: " << hepath << endl;
        int fd = ::open(fpath.c_str(), O_RDONLY);
        int headfd = ::open(hepath.c_str(), O_RDONLY);
        file_to_fd_[fpath] = fd;
        file_to_fd_[hepath] = headfd;

        // get file size
        struct stat st;
        stat(fpath.c_str(), &st);
        uint64_t fsize = st.st_size;
        file_to_size_[fpath] = fsize;

        memset(&st, 0, sizeof(struct stat));
        stat(hepath.c_str(), &st);
        uint64_t hsize = st.st_size;
        file_to_size_[hepath] = hsize;
        cout << "fsize: " << to_string(fsize) << endl;
        cout << "hsize: " << to_string(hsize) << endl;


        // load memory
        DiskIndex::node* fmem = (DiskIndex::node*)mmap(0, fsize*sizeof(char), PROT_READ, MAP_SHARED, fd, 0);

        uint64_t* hmem = (uint64_t*)mmap(0, hsize*sizeof(char), PROT_READ, MAP_SHARED, headfd, 0);

        // add pointer to map
        file_to_mem_[fpath] = fmem;
        file_to_headmem_[hepath] = hmem;
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

    // for each tree we got, find this path
    for (auto tree : file_to_mem_) {
        // traverse tree
        DiskIndex::node* cur = tree.second;
        uint64_t* hemem = file_to_headmem_[(tree.first + ".head")];
        while (true) {

            // check this node's inode's hash
            DiskIndex::inode inode = cur->ent;
            int64_t hashidx = inode.hash;
            cout << "HASH IDX: " << to_string(hashidx) << endl;
            // find hash in memory
            uint64_t offset = hemem[hashidx];
            cout << "OFFSET: " << to_string(offset) << endl;
            // get hash
            char* hash = (char*)tree.second + offset;
            cout << "HASH: " << hash << endl;
            int res = memcmp(hash, phash, 512);
            cout << "RES: " << res << endl;
            // compare hashes, explore accordingly
            if (res == 0) {
                cout << "We found it!" << endl;
                break;
            }

            if (res > 0 && cur->right == -1) {
                cout << "not in tree, can't explore right" << endl;
                break;
            } else if (res == 1 && cur->right != -1) {
                uint64_t noffset = hemem[cur->right];
                cur = tree.second + noffset;
                // keep exploring
                continue;
            }

            // now we need to explore the left
            if (res < 0 && cur->left == -1) {
                cout << "not in tree, cant explore left" << endl;
                break;
            } else if (res == -1 && cur->left != -1) {
                uint64_t loffset = hemem[cur->left];
                cur = tree.second + loffset;
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
    for (auto ent : file_to_headmem_) {
        munmap(ent.second, file_to_size_[ent.first]);
        close(file_to_fd_[ent.first]);

    }
    cout << "6" << endl;
}
