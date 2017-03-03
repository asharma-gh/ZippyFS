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

    /**
     * Adds the given entry to this path
     * clears the cache if its full
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
     * clears the cache if its full
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
    bool root_content_in_cache(std::string root_file);

    /**
     * adds the given node file and content to this cache
     */
    void add_node_file(std::string node_file, std::string contents);

    /**
     * @return the content for the given node file
     * @return empty string if dne
     */
    std::string get_node_file(std::string node_file);

    /**
     * @return true if the given nodes content is in cache
     */
    bool node_content_in_cache(std::string node_file);

    /**
     * @return true if the given .data content is in cache
     */
    bool data_content_in_cache(std::string data_file);

    /**
     * adds the given data file and content to cache
     */
    void add_data_file(std::string data_file, std::string contents);

    /**
     * @return the content for the given .data file
     * @return empty string if dne
     */
    std::string get_data_file(std::string data_file);


    bool root_has_entries(std::string root);


  private:
    /** number of files this cache can store */
    const uint64_t SIZE_ = 512;

    /**
     * map (path,map(root, path entries))
     * is never cleared, only gc'd
     */
    std::unordered_map<std::string, std::unordered_map<std::string,
        std::vector<std::tuple<std::string, std::string, uint64_t, uint64_t>>>> root_entry_cache_;

    /**
     * map (root name, root contents)
     */
    std::unordered_map<std::string, std::string> root_content_cache_;

    /**
     * map (node name, node contents)
     */
    std::unordered_map<std::string, std::string> node_content_cache_;

    /**
     * map (data name, data contents)
     */
    std::unordered_map<std::string, std::string> data_content_cache_;

    /** adds the given key and value to the map */
    void add_content(std::unordered_map<std::string, std::string> cache, std::string key, std::string value);

    /** gets the content for the given key and map */
    std::string get_content(std::unordered_map<std::string, std::string> cache, std::string key);

    /** checks if the given key has an entry in this cache */
    bool in_given_cache(std::unordered_map<std::string, std::string> cache, std::string key);


};
#endif
