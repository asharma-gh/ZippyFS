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
     */
    TireFire(std::string path);

    /**
     * @param size is the size of the requested memory
     * @returns pointer to memory
     */
    void* get_tire(size_t size);

    /**
     * Closes the mmapped region, flushes it
     */
    ~TireFire();

  private:

    std::string file_;


};

#endif
