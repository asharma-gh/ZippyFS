#define BLOCK_CACHE_H
#ifndef BLOCK_CACHE_H

#include <mutex>
#include <map>
#include <string>
#include "block.h"
/**
 * represents an in-memory Block Cache
 * @author Arvin Sharma
 */
class BlockCache {
public:
    /**
     * constructs a BlockCache that flushes to the given dir
     * @param path_to_shdw is the path of the shadow dir
     */
    BlockCache(std::string path_to_shdw);

    /**
     * writes to the given file in cache
     * @return number of bytes written
     */
    int write(std::string path, const uint8_t* buf, uint64_t size, uint64_t offset);

    /**
     * reads from the given file in cache
     * @return the number of bytes read
     */
    int read(std::string path,  const uint8_t* buf, uint64_t size, uint64_t offset);

    /**
     * determines if the thing in path is in cache
     * @param true if the file exists, false otherwise
     */
    bool in_cache(std::string path);

    /**
     * flushes the contents of this block cache to
     * the shadow directory.
     * Also flushes cache_data_ to an index file in the directory
     * @return 0 for success, nonzero otherwise
     */
    int flush_to_shdw();

    /**
     * destructor for BlockCache
     */
    ~BlockCache();



private:

    /** absolute path of shadow directory */
    std::string path_to_shdw_;

    /** file cache **/
    std::map<string, std::vector<std::shared_ptr<Block>> file_cache_;

    /** for multithreaded mode potentially */
    std::mutex mutex_;

    /** represents entries in an index file for files in cache */
    std::string cache_data_;
};
#endif
