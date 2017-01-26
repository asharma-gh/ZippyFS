#ifndef COW_TREE_H
#define COW_TREE_H

#include <map>
#include <vector>
#include "inode.h"


/**
 * Represents a Copy-On-Write Persistent B-Tree
 * @author Arvin Sharma
 */
class CowTree {
    /**
     * This class represents the persistent part
     * - persistent B-tree containing Inodes
     * - each node is flushed to disk when modified
     * - can be reconstructed using disk files
     */
  public:
    /**
     * initializes an empty
     * cow tree
     */
    CowTree();

    /**
     * inserts the given inode into this tree
     * @param inode is the inode to add
     */
    void insert(Inode inode);



  private:

};
#endif
