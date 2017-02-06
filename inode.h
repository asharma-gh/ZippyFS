#ifndef INODE_H
#define INODE_H

#include "block.h"
#include "includes.h"
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
     * constructs an inode with the contents of that one
     * with the given path
     */
    Inode(std::string path, Inode that);

    /**
     * sets the mode for this inode
     * @param mode is the mode for this inode
     */
    void set_mode(uint32_t mode);

    /**
     * gets the mode for this inode
     * @return the mode
     */
    uint32_t get_mode();

    /**
     * increments the link count and adds ref to this inode
     * @param ref is the reference to this inode
     */
    void inc_link(std::string ref);

    /**
     * decrements link count
     * clears blocks if link is 0
     */
    void dec_link(std::string ref);

    /**
     * checks if ref links to this inode
     */
    int is_link(std::string ref);

    /**
     * gets the number of links to this inode
     */
    uint32_t get_link();

    /**
     * updates the modified time of this inode
     */
    void update_mtime();

    /**
     * updates access time
     */
    void update_atime();
    /**
     * @return the mtime as ull
     */
    unsigned long long get_ull_mtime();

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
     * gets the block at block_index
     */
    std::shared_ptr<Block> get_block(uint64_t block_index);

    /**
     * gets a list of block indexes for this inode
     */
    std::vector<uint64_t> get_block_indx();

    /**
     * @return the mapping of blocks for this inode
     */
    std::shared_ptr<std::unordered_map<uint64_t, std::shared_ptr<Block>>> get_blocks_with_id();

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
     * @return true if this inode has a block at block_index
     */
    bool has_block(uint64_t block_index);

    /**
     * fills in the following stat struct with info about this inode
     * @param st is the stat struct to fill
     * @return 0 on success, -1 otherwise
     */
    int stat(struct stat* st);

    /**
     * @return 1 if this is a dir, 0 otherwise
     */
    int is_dir();

    /**
     * sets the files times to the given ones
     */
    void set_st_time(struct timespec  mtim, struct timespec ctim);

    /**
     * fills the given timespec with current time
     * doing this because time is weird in c/c++
     */
    static
    void
    fill_time(struct timespec* ts) {
        clock_gettime(CLOCK_REALTIME, ts);
    }

    /**
     * read size data at the given offset into buf
     */
    int read(uint8_t* buf, uint64_t size, uint64_t offset);

    /**
     * writes data in this inode to thing in fd
     * @return 0 for success, -1 otherwise
     */
    int flush_to_fd(int fd);

    /**
     * Returns the record of this inode
     * Format: path - mode - nlink - ul_mtime - ul_ctime - ts_mtime - ts_ctime - size - links - block_data
     */
    std::string get_record();

    /** deletes this inode */
    void delete_inode();

    /** un-delete this inode */
    void remake_inode();

    /** is this inode deleted */
    int is_deleted();

    /** is this inode dirty? */
    int is_dirty();

    /**
     * sets this inode as dirty
     */
    void set_dirty();

    /**
     * if this inode is dirty, this undo's that
     * useful if write was used to load a file to cache
     */
    void undo_dirty();

    /**
     * returns the id for this inode
     */
    std::string get_id();

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

    /** creation time for this inode as time spec */
    struct timespec ts_ctime_;

    /** access time for this inode as time spec */
    struct timespec ts_atime_;

    /** size of this inode */
    uint64_t size_;

    /** is this inode deleted? */
    int deleted_;

    /** has this inode been written to? */
    int dirty_;

    /** id for this inode */
    std::string inode_id_;

    /** the paths which have a reference to this inode */
    std::unordered_set<std::string> links_;

    /** list of blocks for this inode */
    std::unordered_map<uint64_t, std::shared_ptr<Block>> blocks_;

};
endif
