#ifndef DISK_INDEX_LOADER_H
#define DISK_INDEX_LOADER_H
#include "includes.h"
#include "inode.h"
#include "block.h"
#include "disk_index.h"
/**
 * Represents a loader for a disk indexing structure,
 * loading it to memory and parsing inodes from it
 * @author Arvin Sharma
 */
class DiskIndexLoader {
  public:

    /**
     * Constructs a new Disk Index Loader
     * @param path is the path to the tree dir
     */
    DiskIndexLoader(std::string path);
    DiskIndexLoader();
    /**
     * loads all new trees from disk to this struct
     */
    void load_trees();
    /**
     * Struct for retrieving inode info from disk
     */
    typedef struct {
        std::string i_path;
        std::string i_inode_id;
        uint32_t i_mode;
        uint32_t i_nlink;
        unsigned long long i_mtime;
        unsigned long long i_ctime;
        uint64_t i_size;
        int i_deleted;
        /** map(block_id, block time) */
        std::unordered_map<uint64_t, unsigned long long> i_block_time;
        /** map(block_id, block ptr) */
        std::unordered_map<uint64_t, std::shared_ptr<Block>> i_block_data;
    } disk_inode_info;
    /**
     * finds the latest inode for the given path
     */
    disk_inode_info find_latest_inode(std::string path);

    ~DiskIndexLoader();


  private:

    /** cache of mmap'd trees */
    /** map(path, ptr), map(path, size), and map(path, file descriptor) */
    std::unordered_map<std::string, DiskIndex::node*> file_to_mem_;

    std::unordered_map<std::string, uint64_t> file_to_size_;

    std::unordered_map<std::string, int> file_to_fd_;

    /** finds all the new trees */
    void find_new_trees();

    /** path of the tree dir */
    std::string path_;

};
#endif
