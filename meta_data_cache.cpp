#include "meta_data_cache.h"
#include "util.h"
using namespace std;

MetadataCache::MetadataCache() {

}

void
MetadataCache::add_entry(string path, string root, vector<tuple<string, string, uint64_t, uint64_t>> entry) {
    if (root_cache_.size() == SIZE_) {
        // evict the LRU entry
        string min;
        unsigned long long cur_min = ULLONG_MAX;
        for (auto ent : lru_times_) {
            if (ent.second < cur_min)
                min = ent.first;
        }
        root_cache_.erase(min);
        lru_times_.erase(min);
    }
    root_cache_[path][root] = entry;
}


vector<tuple<string, string, uint64_t, uint64_t>>
MetadataCache::get_entry(string path, string root) {
    if (root_cache_.find(path) != root_cache_.end()) {
        if (root_cache_[path].find(root) != root_cache_[path].end()) {
            lru_times_[path] = Util::get_time();
            return root_cache_[path][root];
        }
    }
    return vector<tuple<string, string, uint64_t, uint64_t>>();
}
