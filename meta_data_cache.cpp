#include "meta_data_cache.h"
#include "util.h"
using namespace std;

MetadataCache::MetadataCache() {

}



bool
MetadataCache::node_content_in_cache(string node_name) {
    return node_content_cache_.find(node_name) != node_content_cache_.end();
}

void
MetadataCache::add_node_file(string node_file, string const& contents) {
    node_content_cache_[node_file] =  contents;
}


string&
MetadataCache::get_node_file(string node_file) {
    return node_content_cache_[node_file];
}
