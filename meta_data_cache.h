#ifndef META_DATA_CACHE_H
#define META_DATA_CACHE_H
#include "includes.h"

/**
 * Represents an in-memory cache for meta-data files: .root, .node, .data
 * Eviction Scheme: LRU
 * @author Arvin Sharma
 */
class MetadataCache {

  public:
    /**
     * Constructs an empty Metadata cache
     */
    MetadataCache();

    /**
     * Adds the given entry to this path
     * Evicts the LRU entry if the cache is full
     * the tuple is (node name, inode id, offset, size)
     */
    void add_entry(std::string path, std::string root, std::vector<std::tuple<std::string, std::string, uint64_t, uint64_t>> entry);

    /**
     * @return the vector of entries for the given path and root
     * @return an empty vector if it isn't in the cache
     */
    std::vector<std::tuple<std::string, std::string, uint64_t, uint64_t>> get_entry(std::string path, std::string root);

  private:
    /** number of files this cache can store */
    const uint64_t SIZE_ = 64;

    /**
     * map (path,map(root, path entries))
     */
    std::unordered_map<std::string, std::map<std::string,
        std::vector<std::tuple<std::string, std::string, uint64_t, uint64_t>>>> root_cache_;

    /**
     * map (path, latest time)
     * on eviction, the path with the min(latest_time) is deleted
     */
    std::unordered_map <std::string, unsigned long long> lru_times_;


};
#endif
