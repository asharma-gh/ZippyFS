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
    cout << "finding inode for " << path << "getdata? " << to_string(get_data) << endl;
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
        for(;;) {
            // check if this node contains this key
            for (int64_t ii = 0; ii < cur->num_keys; ii++) {
                // find this key
                int64_t keyof = cur->keys[ii];
                char* kmem = (char*)tree.second + keyof;

                if (strcmp(kmem, phash) == 0) {
                    // we found it
                    if (cur->is_leaf)
                        break;
                    else {
                        // its in a child
                        int64_t childof = cur->children[ii + 1];
                        cur = (BPLUSIndex::node*)((char*)tree.second + childof);
                        continue;

                    }
                }
            }
        }
    }
    return latest;
}
