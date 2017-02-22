#include "meta_data_cache.h"
#include "util.h"
using namespace std;

MetadataCache::MetadataCache() {

}

void
MetadataCache::add_entry(string path, string root, vector<tuple<string, string, uint64_t, uint64_t>> entry) {
    if (root_entry_cache_.size() == SIZE_) {
        root_entry_cache_.clear();
    }
    //  root_lru_times_[path] = ULLONG_MAX;
    root_entry_cache_[path][root] = entry;
}


vector<tuple<string, string, uint64_t, uint64_t>>
MetadataCache::get_entry(string path, string root) {
    if (in_cache(path, root)) {
        //     lru_times_[path] = Util::get_time();
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
    add_content(shared_ptr<unordered_map<string, string>>(&root_content_cache_), root_file, contents);
}

string
MetadataCache::get_root_file_contents(string root_file) {
    if (root_content_in_cache(root_file)) {
        return root_content_cache_[root_file];
    }
    return "";
}

void
MetadataCache::add_content(std::shared_ptr<unordered_map<string, string>> cache, string key, string value) {
    if (cache->size() == SIZE_) {
        cache->clear();
    }

    (*cache)[key] = value;
}

string
MetadataCache::get_content(std::shared_ptr<unordered_map<string, string>> cache, string key) {
    if (in_given_cache(cache, key)) {
        return (*cache)[key];
    }
    return "";
}

bool
MetadataCache::in_given_cache(std::shared_ptr<unordered_map<string, string>> cache, string key) {
    return cache->find(key) != cache->end();
}

string
MetadataCache::get_node_file(string node_file) {
    return get_content(shared_ptr<unordered_map<string, string>>(&node_content_cache_), node_file);
}

string
MetadataCache::get_data_file(string data_file) {
    return get_content(shared_ptr<unordered_map<string, string>>(&data_content_cache_), data_file);

}

void
MetadataCache::add_node_file(string node_file, string content) {
    add_content(shared_ptr<unordered_map<string, string>>(&node_content_cache_), node_file, content);
}

void
MetadataCache::add_data_file(string data_file, string content) {
    add_content(shared_ptr<unordered_map<string, string>>(&data_content_cache_), data_file, content);
}

bool
MetadataCache::node_content_in_cache(string node_file) {
    return in_given_cache(shared_ptr<unordered_map<string, string>>(&node_content_cache_), node_file);
}

bool
MetadataCache::data_content_in_cache(string data_file) {
    return in_given_cache(shared_ptr<unordered_map<string, string>>(&data_content_cache_), data_file);
}
