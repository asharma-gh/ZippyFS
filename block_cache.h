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
     * @param path_to_disk is the path to flush stuff to
     */
    BlockCache(std::string path_to_disk, bool f);

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
     * @return 0 on success, -1 otherwise
     */
    int flush_to_disk();

    /**
     * loads the given file from disk to this cache
     */
    int load_from_disk(std::string path);

    /**
     * @return a pointer to the inode at the given path
     */
    std::shared_ptr<Inode> get_inode_by_path(std::string path);

  private:

    /** absolute path of shadow directory */
    std::string path_to_shdw_;

    /** path to main disk dir */
    std::string path_to_disk_;

    /** for multithreaded mode potentially */
    std::mutex mutex_;

    /** map of (path, inode idx) */
    std::map<std::string, std::string> inode_idx_;

    /** map of (inode idx, inode ptr) */
    std::map<std::string, std::shared_ptr<Inode>> inode_ptrs_;

    /** map of ((inode idx, block idx), dirty block) for flushing */
    std::map<std::string, std::map<uint64_t, std::shared_ptr<Block>>> dirty_block_;

    /* size of this block cache */
    uint64_t size_;

    /* "big enough" size of this block cache */
    const uint64_t MAX_SIZE = 250;

    /** finds the latest set of nodes for the given file with blocks indexes not in the given vector*/
    std::unordered_set<std::string> find_latest_node(std::string inode_idx, std::vector<uint64_t> block_idx);

    /**
     * @param root_name is the name of the root file
     * @param path is the path of the file to find
     * @returns [list of (node name, inode id, offset ,size)]
     * If this list is empty, then the path entry did not exist
     */
    std::vector<std::tuple<std::string, std::string, uint64_t, uint64_t>> find_entry_in_root(std::string root_name, std::string path);

    /**
     * finds the given entry in the given index file
     * @param index_name is the name of the index file
     * @param path is the file to find
     * @return the entry for the given file in the given index
     */
    std::string find_entry_in_index(std::string index_name, std::string path);

    /**
     * Generates the table of dirty blocks and offset positions for use in .data files
     * this is because inodes do not keep track of which blocks are dirty
     * @param inode_idx is the id of the inode to do this on
     * map(total block size, map(block#, (offset#, block size)))
     */
    std::pair<uint64_t, std::map<uint64_t, std::pair<uint64_t, uint64_t>>> get_offsets(std::string inode_idx);
};
#endif
