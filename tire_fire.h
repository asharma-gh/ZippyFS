#ifndef TIREFIRE_H
#define TIREFIRE_H
#include "includes.h"

/**
 * Represents an allocator which returns memory allocated
 * within a file on disk
 *
 * @author Arvin Sharma
 */

class TireFire {

  public:

    /**
     * initializes the allocator for the given file
     * the given file must be empty!
     */
    TireFire(std::string path);

    TireFire();

    void set_path(std::string path);
    /**
     * @param size is the size of the requested memory
     * @returns index to a pointer to memory
     * allocation is consecutive in the file, there is no free
     * only way to free is to make a new file
     */
    int64_t get_tire(size_t size);

    /**
     * gets the memory associated with the given index
     */
    void* get_memory(int64_t index);
    /**
     * Closes the mmapped region, flushes the header for it
     */
    void flush_head();

    ~TireFire();
  private:
    std::string file_;
    int fd_;
    uint64_t cur_size_;
    void* cur_ptr_;
    uint64_t latest_index;
    std::unordered_map<int64_t, uint64_t> index_to_offset;
    std::unordered_map<int64_t, void*> index_to_ptr;


};

#endif
