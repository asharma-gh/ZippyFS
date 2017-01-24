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
    void inc_link(std::string ref);
    void set_mtime(unsigned long long mtime);

    /** return the info of this inode as a string
     * with the format of records
     */
    std::string get_record()


  private:
    std::string path_;
    uint32_t mode_;
    uint32_t nlink_;
    unsigned long long mtime_;
    unsigned long long ctime_;
    uint64_t size_;
    std::vector<std::string> links_;



};
#endif
