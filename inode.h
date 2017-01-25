#ifndef INODE_H
#define INODE_H

#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_set>
#include <map>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctime>
#include <iostream>
#include <cassert>
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
     * decrements link count
     */
    void dec_link();

    /**
     * updates the modified time of this inode
     */
    void update_mtime();

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

    /**
     * adds the given block to this inode
     * @param block_index is the index of the block
     * @param block is the block of data
     * - Invalidates old block if it exists
     */
    void add_block(uint64_t block_index, std::shared_ptr<Block> block);

    /** removes the block at the given index from this inode */
    void remove_block(uint64_t block_index);

    /**
     * fills in the following stat struct with info about this inode
     * @param st is the stat struct to fill
     * @return 0 on success, -1 otherwise
     */
    int stat(struct stat* st);

    /**
     * fills the given timespec with current time
     * doing this because time is weird in c/c++
     */
    static
    void
    fill_time(struct timespec* ts) {
        clock_gettime(CLOCK_REALTIME, ts);
    }
    int read(uint8_t* buf, uint64_t size, uint64_t offset);

    /**
     * writes data in this inode to thing in fd
     * @return 0 for success, -1 otherwise
     */
    int flush_to_fd(int fd);


  private:
    /** the path of this inode **/
    std::string path_;

    /** the mode for this inode, based on unix spec */
    uint32_t mode_;

    /** number of links for this inode */
    uint32_t nlink_;

    /** modified time for this inode as ull*/
    unsigned long long  ul_mtime_;

    /** creation time for this inode as ull*/
    unsigned long long ul_ctime_;

    /** modified time for this inode as time spec */
    struct timespec ts_mtime_;

    /** creatio ntime for this inode as time spec */
    struct timespec ts_ctime_;

    /** size of this inode */
    uint64_t size_;

    /** the paths which have a reference to this inode */
    std::unordered_set<std::string> links_;

    /** list of blocks for this inode */
    std::map<uint64_t, std::shared_ptr<Block>> blocks_;



};
#endif
