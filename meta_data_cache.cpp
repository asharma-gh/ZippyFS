#include "meta_data_cache.h"
#include "util.h"
using namespace std;

MetadataCache::MetadataCache() {

}

void
MetadataCache::add_entry(string path, string root, vector<tuple<string, string, uint64_t, uint64_t>> entry) {
    if (root_entry_cache_.size() == SIZE_) {
        // evict the LRU entry
        string min;
        unsigned long long cur_min = ULLONG_MAX;
        for (auto ent : lru_times_) {
            if (ent.second <= cur_min)
                min = ent.first;
        }
        root_entry_cache_.erase(min);
        lru_times_.erase(min);
    }
    root_lru_times_[path] = ULLONG_MAX;
    root_entry_cache_[path][root] = entry;
}


vector<tuple<string, string, uint64_t, uint64_t>>
MetadataCache::get_entry(string path, string root) {
    if (in_cache(path, root)) {
        lru_times_[path] = Util::get_time();
        return root_entry_cache_[path][root];
    }
    return vector<tuple<string, string, uint64_t, uint64_t>>();
}

bool
MetadataCache::in_cache(string path, string root) {
    return root_entry_cache_.find(path) != root_entry_cache_.end()
           && root_entry_cache_[path].find(root) != root_entry_cache_[path].end();
}

bool
MetadataCache::root_content_in_cache(string root_name) {
    return root_content_cache_.find(root_name) != root_content_cache_.end();
}

void
MetadataCache::add_root_file(string root_file, string contents) {
    if (root_content_cache_.size() == SIZE_) {
        // evict the LRU entry
        string min;
        unsigned long long cur_min = ULLONG_MAX;
        for (auto ent : root_lru_times_) {
            if (ent.second <= cur_min)
                min = ent.first;
        }
        root_content_cache_.erase(min);
        root_lru_times_.erase(min);
    }
    root_lru_times_[root_file] = ULLONG_MAX;
    root_content_cache_[root_file] = contents;
}

string
MetadataCache::get_root_file_contents(string root_file) {
    if (root_content_in_cache(root_file)) {
        root_lru_times_[root_file] = Util::get_time();
        return root_content_cache_[root_file];
    }
    return "";
}
