#ifndef DISK_INDEX_H
#define DISK_INDEX_H
#include "includes.h"
#include "inode.h"
/**
 * Represents an in-memory structure of flushed files
 * @author Arvin Sharma
 */
class DiskIndex {

  public:

    DiskIndex();

    void add_inode(Inode in);

    void find_inode(std::string path);



  private:

    typedef struct node {

    } node;

    typedef struct inode {

    } inode;

    typedef struct block_data {

    } block_data;
};
#endif
