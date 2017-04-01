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
    }
}
