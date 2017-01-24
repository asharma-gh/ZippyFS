#ifndef BLOCK_H
#define BLOCK_H
#include <string>
#include <cstring>
#include <array>
#include <stdexcept>
#include <cstdint>
#include <vector>
/**
 * Represents a block of data for a file
 * @author Arvin Sharma
 */

class Block {

  public:

    /**
     * constructs an empty block
     */
    Block();

    /**
     * constructs a block with size stuff
     * @throw domain_error if size > logical size
     */
    Block(const uint8_t* data, uint64_t size);


    /**
     * adds the given data to this empty block
     * @param data is the data to add
     * @return 0 for success, nonzero otherwise.
     * @return -1 if this block is not empty
     */
    int insert(const uint8_t* data, uint64_t size);

    /**
     * @return the data of this block as a string
     */
    std::vector<uint8_t> get_data();

    /**
     * set this block as dirty
     */
    void set_dirty();

    /**
     * @return is this block dirty?
     */
    bool is_dirty();

    /**
     * @returns the actual size of this block
     *  - actual size is defined as the size of bytes this block
     *  contains
     */
    uint64_t get_actual_size();


    /**
     * @return the logical size of this block
     * - logical size is defined as the total size of this block
     */
    static uint64_t get_logical_size() {
        return SIZE_;
    }

  private:
    static const uint64_t SIZE_ = 4096;
    uint64_t actual_size_;
    std::array<uint8_t, SIZE_> data_;
    bool dirty_;
    bool has_data_ = false;
    void insert_data(const uint8_t* data, uint64_t size);



};

#endif
