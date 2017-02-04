#ifndef BLOCK_CACHE_H
#define BLOCK_CACHE_H

#include "includes.h"
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
    int make_file(std::string path, mode_t mode, bool dirty);

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
     * renames the from file to the one in to
     */
    int rename(std::string from, std::string to);

    /**
     * symlink support
     */
    int symlink(std::string from, std::string to);

    /**
     * read symlink
     */
    int readlink(std::string path,  uint8_t* buf, uint64_t size);


    // helper struct for storing entries for readdir
    typedef struct index_entry {
        std::string path;
        unsigned long long added_time;
        int deleted;
    } index_entry;

    /**
     * @return list of names of things in path
     */
    std::vector<index_entry> readdir(std::string path);

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
     * removes the dir in path
     */
    int rmdir(std::string path);

    /**
     * determines if the thing in path is in cache
     * @param 0 if the file exists, -1 otherwise
    */
    int in_cache(std::string path);

    /**
     * flushes the contents of this block cache to
     * the shadow directory.
     * - ONLY FLUSHES WHEN AT MAX CAPACITY
     * Also flushes meta data to index.idx file
     * @param on_close is whether this is being called on close
     * @return 0 for success or no flushing is needed,
     * -1 if no flushing occured
     */
    int flush_to_shdw(int on_close);

    /**
     * loads the thing to this cache
     * @param path is the path to the thing to load
     * @return 0 on success, -1 otherwise
     */
    int load_to_cache(std::string path);

    /**
     * updates the mtime for the file
     * @return 0 if the file exists and can be opened
     */
    int open(std::string path);

    /**
     * @returns a list of paths of things currently in cache
     */
    std::vector<std::string> get_refs(std::string path);

    /**
     * flushes normalized maps to zip directory
     */
    void flush_to_zip();

    //TODO: implement ability to add/remove items from flushed files
    // std::string get_id();
    // std::shared_ptr<Inode> get_item();


  private:

    /** absolute path of shadow directory */
    std::string path_to_shdw_;

    /** path to main zip directory */
    std::string path_to_zip_dir_;

    /** for multithreaded mode potentially */
    std::mutex mutex_;

    /** map of (path, inode idx) */
    std::map<std::string, std::string> inode_idx_;

    /** map of (inode idx, inode ptr) */
    std::map<std::string, std::shared_ptr<Inode>> inode_ptrs_;

    /** map of ((inode num, block num), block ptr) */
    std::map<std::string, std::map<uint64_t, std::shared_ptr<Block>>> blocks_;

    /** represents meta data for files in cache */
    std::map<std::string, std::shared_ptr<Inode>> meta_data_;

    /* size of this block cache */
    uint64_t size_;

    /* "big enough" size of this block cache */
    const uint64_t MAX_SIZE = 250;
};
#endif
