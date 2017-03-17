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

    /**
     * @param size is the size of the requested memory
     * @returns pointer to memory
     * allocation is consecutive in the file, there is no free
     * only way to free is to make a new file
     */
    void* get_tire(size_t size);

    /**
     * Closes the mmapped region, flushes it
     */
    ~TireFire();

  private:

    std::string file_;
    uint64_t cur_size_;
    void* cur_ptr_;


};

#endif
