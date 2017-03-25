#ifndef BPLUS_INDEX_H
#define BPLUS_INDEX_H
#include "includes.h"
#include "inode.h"
#include "tire_fire.h"
#include "block.h"
/**
 * This class represents a memory-backed B+Tree used for indexing
 * inodes flushed from the in-memory file-system.
 *  - It will only support the limited functionality required by the file system, which is only insertion.
 * It utilizes the memory allocated dubbed TireFire, which also is implemented using bump-allocation, and thus free-ing is not available, and so deletions are not possible.
 *
 * @author Arvin Sharma
 */
class BPLUSIndex {

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



  private:

};
#endif
