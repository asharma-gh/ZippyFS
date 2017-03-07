#ifndef META_DATA_CACHE_H
#define META_DATA_CACHE_H
#include "includes.h"

/**
 * Represents an in-memory cache for meta-data files: .root right now
 * - contains either a path entry in the .root, or an entire .root file's contents.
 * Eviction Scheme: clearing cache when full
 * @author Arvin Sharma
 */
class MetadataCache {

  public:
    /**
     * Constructs an empty Metadata cache
     */
    MetadataCache();


    void add_node_file(std::string node_file, std::string const& contents);

    /**
     * @return the content for the given node file
     * @return empty string if dne
     */
    std::string& get_node_file(std::string node_file);

    /**
     * @return true if the given nodes content is in cache
     */
    bool node_content_in_cache(std::string node_file);


  private:
    /** number of files this cache can store */
    const uint64_t SIZE_ = 1024;
    /**
     * map (node name, node contents)
     */
    std::unordered_map<std::string, std::string> node_content_cache_;


};
#endif
