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
     */
    DiskIndexLoader();

    /**
     * loads all new trees from disk to this struct
     */
    void load_trees();

    /**
     * finds the latest inode for the given path
     */
    Inode find_latest_inode(std::string path);

  private:

    /** cache of mmap'd trees */
    std::unordered_map<std::string, DiskIndex::node*> file_to_tree_;

    void find_new_trees();

};
#endif
