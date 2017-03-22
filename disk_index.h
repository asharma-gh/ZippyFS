#ifndef DISK_INDEX_H
#define DISK_INDEX_H
#include "includes.h"
#include "inode.h"
#include "tire_fire.h"
#include "block.h"
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

    /** represents an inode which is easily flushable to disk from memory */
    typedef struct inode {

        // stat info
        uint32_t mode;
        uint32_t nlink;
        uint64_t mtime;
        uint64_t ctime;
        uint64_t size;
        int deleted;
        // index to hash memory, always 512 size char*
        int64_t hash;
        // index to block data list memory
        int64_t block_data;
        // total size of block data
        uint64_t block_data_size;
    } inode;

    /** represents [block id, data] for inodes */
    typedef struct block_data {
        uint64_t block_id;
        uint64_t mtime;
        uint64_t size;
        // index to data memory
        int64_t data_offset;
    } block_data;


    /** represents a tree structure containing inodes */
    typedef struct node {
        int64_t left;
        int64_t right;
        inode ent;
    } node;
    /**
     * Constructs a disk index structure that flushes to the given path
     */
    DiskIndex();

    /**
     * Adds the given inode to this disk index
     * with the dirty blocks (block idx, block)
     */
    void add_inode(Inode in, std::map<uint64_t, std::shared_ptr<Block>> dirty_blocks, std::map<uint64_t, unsigned long long> block_mtime);

    /**
     * finds the given inode from this disk structure
     */
    void find_latest_inode(std::string path);

    ~DiskIndex();

  private:

    /** in memory structure */
    TireFire mem_;
    uint64_t rootidx_ = 0;
    /** maintain pointer to root */
    node* root_ptr_ = nullptr;
    bool root_has_inode_ = false;
};
#endif
