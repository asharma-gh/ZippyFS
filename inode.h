#ifndef INODE_H
#define INODE_H

#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include "block.h"
#include "util.h"
/**
 * Represents an inode. A structure containing
 * information about a file in the file system
 * @author Arvin Sharma
 */
class Inode {

public:
    Inode(std::string path);

    void set_mode(uint32_t mode);
    void inc_link();
    void set_mtime(unsigned long long mtime);
    void set_blocks(std::shared_ptr<std::vector<std::shared_ptr<Block>>> blocks);


private:
    std::string path_;
    uint32_t mode_;
    uint32_t nlink_;
    unsigned long long mtime_;
    unsigned long long ctime_;
    std::vector<std::shared_ptr<Block>> blocks_;
    uint64_t size_;



};
#endif
