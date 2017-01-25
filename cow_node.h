#ifndef COW_NODE_H
#define COW_NODE_H

#include "inode.h"
#include <map>
#include <cstdint>
#include <string>
class CowNode {
  public:
    /**
     * constructs an empty cow node
     * @param archive_path is the location to flush to
     */
    CowNode(std::string archive_path);

    /**
     * flushes the meta-data of this node to a file
     * in archive_path
     * - file name generated using 64 byte rand int from urandom
     *
     * - flushes the map
     *
     * FORMAT:
     * key inode_record
     *
     * Format of
     */
    void flush_to_archive();

    void reconstruct();

  private:
    /** (key, inode) mapping containing data for this node */
    std::map<uint64_t, std::shared_ptr<Inode>> contents_;
    std::string archive_path_;

};
#endif
