#ifndef META_DATA_CACHE_H
#define META_DATA_CACHE_H
#include "includes.h"

/**
 * Represents an in-memory cache for meta-data files: .root right now
 * - contains either a path entry in the .root, or an entire .root file's contents.
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
     * @return empty vector if the path and root do not have an entry
     */
    std::vector<std::tuple<std::string, std::string, uint64_t, uint64_t>> get_entry(std::string path, std::string root);

    /**
     * @return true if this path and root have an entry
     */
    bool in_cache(std::string path, std::string root);

    /**
     * Adds the given root file and it's contents to this cache
     * Evicts the LRU root file if full
     */
    void add_root_file(std::string root_file, std::string contents);

    /**
     * Retrieves the given root file's contents
     * @return the given root file's contents, empty string
     * if dne
     */
    std::string get_root_file_contents(std::string root_file);

    /**
     * @return true if this root file's contents are in cache
     */
    bool root_content_in_cache(std::string root_name);


  private:
    /** number of files this cache can store */
    const uint64_t SIZE_ = 512;

    /**
     * map (path,map(root, path entries))
     */
    std::unordered_map<std::string, std::unordered_map<std::string,
        std::vector<std::tuple<std::string, std::string, uint64_t, uint64_t>>>> root_entry_cache_;

    /**
     * map (path, latest time)
     * on eviction, the path with the min(latest_time) is deleted
     */
    std::unordered_map <std::string, unsigned long long> lru_times_;

    /**
     * map (root, latest_time)
     * on eviction, the root with the min(latest_time) is deleted
     */
    std::unordered_map <std::string, unsigned long long> root_lru_times_;


    /**
     * map (root name, root contents)
     */
    std::unordered_map<std::string, std::string> root_content_cache_;



};
#endif
