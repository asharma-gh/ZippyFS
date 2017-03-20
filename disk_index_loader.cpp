#include "disk_index_loader.h"
#include <cstring>
#include <sys/mman.h>
using namespace std;

DiskIndexLoader::DiskIndexLoader(string path) {
    path_ = path;
    load_trees();
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


        // load memory
        DiskIndex::node* fmem = (DiskIndex::node*)mmap(0, fsize, PROT_READ, MAP_SHARED, fd, 0);

        uint64_t* hmem = (uint64_t*)mmap(0, hsize, PROT_READ, MAP_SHARED, headfd, 0);

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
    // for each tree we got, find this path
    for (auto tree : file_to_mem_) {
        // traverse tree
        // if we found this path, record info in inode

        // if we have one and this one is later, update
        //
        // update blocks for each one if new
        //
    }
    return latest;
}


DiskIndexLoader::~DiskIndexLoader() {
    for (auto ent : file_to_mem_) {
        munmap(ent.second, file_to_size_[ent.first]);
        close(file_to_fd_[ent.first]);
    }
    for (auto ent : file_to_headmem_) {
        munmap(ent.second, file_to_size_[ent.first]);
        close(file_to_fd_[ent.first]);

    }
}
