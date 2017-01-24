#ifndef INODE_H
#define INODE_H

#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include "block.h"
#include "util.h"

/**
 * Represents an inode. A structure containing
 * information about a file in the file system
 * @author Arvin Sharma
 */
class Inode {

  public:

    /**
     * constructs an inode for the thing in path
     * @path is the path of the thing for this inode
     */
    Inode(std::string path);


    /**
     * sets the mode for this inode
     * @param mode is the mode for this inode
     */
    void set_mode(uint32_t mode);

    /**
     * increments the link count and adds ref to this inode
     * @param ref is the reference to this inode
     */
    void inc_link(std::string ref);

    /**
     * sets the modified time of this inode
     * @param mtime the new modified time of this inode
     */
    void set_mtime(unsigned long long mtime);

    /**
     * sets the size of this inode
     * @param size is the new size for this inode
     */
    void set_size(unsigned long long size);

    /**
     * @returns the size of this inode;
     */
    uint64_t get_size();

    /**
     * @return the info of this inode as a string
     * in the format of records
     */
    std::string get_record();

    /**
     * @return a list of paths that refer to this inode
     */
    std::vector<std::string> get_refs();

    /** adds the given block to this inode */
    void add_block(uint64_t block_index, std::shared_ptr<Block> block);

    /** removes the block at the given index from this inode */
    void remove_block(uint64_t block_index);


  private:
    /** the path of this inode **/
    std::string path_;

    /** the mode for this inode, based on unix spec */
    uint32_t mode_;

    /** number of links for this inode */
    uint32_t nlink_;

    /** modified time for this inode */
    unsigned long long  mtime_;

    /** creation time for this inode */
    unsigned long long ctime_;

    /** size of this inode */
    uint64_t size_;

    /** the paths which have a reference to this inode */
    std::vector<std::string> links_;

    /** list of blocks for this inode */
    std::map<uint64_t, std::shared_ptr<Block>> blocks_;



};
#endif
