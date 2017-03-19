#ifndef DISK_INDEX_H
#define DISK_INDEX_H
#include "includes.h"
#include "inode.h"
/**
 * Represents a persistent  in-memory structure of flushed files / inodes
 * (eventually will replace block_cache internal structure)
 * USAGE:
 * - on a flush, move inodes from block_cache to disk_index
 * - flush disk_index to disk thru TireFire
 * - on a load, load up all mmap'd disk_indeces into the block_cache's disk_index
 *   - find inode thru find_latet_inode.
 * @author Arvin Sharma
 */
class DiskIndex {

  public:

    /**
     * Constructs a disk index structure
     */
    DiskIndex();

    /**
     * Adds the given inode to this disk index
     */
    void add_inode(Inode in);

    /**
     * finds the given inode from this disk structure
     */
    void find_latest_inode(std::string path);

    /**
     * loads all new trees from disk to this struct
     */
    void load_trees();

  private:
    /** represents an inode which is easily flushable to disk from memory */
    typedef struct inode {

        // stat info
        //
        // block data memory index
        int block_data;
    } inode;

    / ** represents [block id, data] for inodes */
    typedef struct block_data {
        uint64_t block_id;
        uint8_t* data;
    } block_data;


/** represents a tree structure containing inodes */
typedef struct node {

    int left;
    int right;
    inode ent;
} node;

/** cache of mmap'd trees */
std::unordered_map<std::string, node*> file_to_tree_;


// checks if there are any new trees on disk, globs
void find_new_trees();
};
#endif
