#include "bplus_index_loader.h"
#include "util.h"
#include <sys/mman.h>
using namespace std;

BPLUSIndexLoader::BPLUSIndexLoader(string path) {
    path_ = path;
    load_trees();
}

void
BPLUSIndexLoader::load_trees() {
    DIR* tree_dir = opendir(path_.c_str());
    struct dirent* entry;
    while ((entry = readdir(tree_dir))) {
        // check if valid tree file
        //
        //
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
