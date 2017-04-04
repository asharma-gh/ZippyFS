#ifndef BPLUS_INDEX_LOADER_H
#define BPLUS_INDEX_LOADER_H
#include "includes.h"
#include "inode.h"
#include "block.h"
#include "bplus_index.h"

/**
 * Represents a loader for a B+Tree indexing structure
 * @author Arvin Sharma
 */
class BPLUSIndexLoader {
  public:

    /**
     * @param path is the path of the tree dir
     */
    BPLUSIndexLoader(std::string path);
    BPLUSIndexLoader();
    /** loads all the new trees from disk to this struct */
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

    /** finds the latest inode for the given path */
    disk_inode_info find_latest_inode(std::string path, bool get_data);

    /** gets all the direct child inodes for the given path */
    std::unordered_set<std::string> get_children(std::string path);

    ~BPLUSIndexLoader();

  private:
    std::string path_;
    std::unordered_map<std::string, BPLUSIndex::node*> file_to_mem_;

    std::unordered_map<std::string, int> file_to_fd_;

    std::unordered_map<std::string, uint64_t> file_to_size_;
};
#endif
