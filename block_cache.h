#ifndef BLOCK_CACHE_H
#define BLOCK_CACHE_H

#include <mutex>
#include <map>
#include <vector>
#include <memory>
#include <string>
#include <utility>
#include <cstdint>
#include <cassert>
#include <sys/stat.h>
#include "block.h"
#include "inode.h"
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
     * creates a file with the given path and mode
     * @return 0 on success, -1 on failure
     */
    int make_file(std::string path, mode_t mode);

    /**
     * loads the thing in path from the shadow director to cache
     * @param path is the path of the file
     * @return 0 on success, -1 on failure
     */
    int load_from_shdw(std::string path);

    /**
     * writes to the given file in cache
     * @return number of bytes written
     */
    int write(std::string path, const uint8_t* buf, uint64_t size, uint64_t offset);

    /**
     * reads from the given file in cache
     * @return the number of bytes read
     */
    int read(std::string path, uint8_t* buf, uint64_t size, uint64_t offset);

    /**
     * @return list of names of things in path
     */
    std::vector<std::string> readdir(std::string path);
    /**
     * returns the attributes of the thing in path
     * into st
     * @return 0 on success, -1 otherwise
     */
    int getattr(std::string path, struct stat* st);

    /**
     * truncates the file to the given size
     * @param path is the path of the file
     * @param size is the new size of the file
     * @return 0 on success, -1 otherwise
     */
    int truncate(std::string path, uint64_t size);

    /**
     * removes the thing in path
     * @param path is the path of the thing
     */
    int remove(std::string path);

    /**
     * determines if the thing in path is in cache
     * @param 0 if the file exists, -1 otherwise
    */
    int in_cache(std::string path);

    /**
     * flushes the contents of this block cache to
     * the shadow directory.
     * Also flushes cache_data_ to an index file in the directory
     * @return 0 for success, nonzero otherwise
     */
    int flush_to_shdw();

    /**
     * loads the thing to this cache
     * @param path is the path to the thing to load
     * @return 0 on success, -1 otherwise
     */
    int load_to_cache(std::string path);

    /**
     * @returns a list of paths of things currently in cache
     */
    std::vector<std::string> get_refs(std::string path);



  private:

    /** absolute path of shadow directory */
    std::string path_to_shdw_;

    /** for multithreaded mode potentially */
    std::mutex mutex_;

    /** represents meta data for files in cache */
    std::map<std::string, std::shared_ptr<Inode>> meta_data_;

};
#endif
