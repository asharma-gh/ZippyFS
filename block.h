#ifndef BLOCK_H
#define BLOCK_H
#include <string>
#include <cstring>
#include <array>
/**
 * Represents a block of data for a file
 * @author Arvin Sharma
 */

class Block {

public:

    /**
     * constructs a block at default size
     * and no data
     */
    Block();

    /**
     * constructs a block with the given data
     * @param data is the data for this block
     */
    Block(std::string data);

    /**
     * adds the given data to this empty block
     * @param data is the data to add
     * @return 0 for success, nonzero otherwise.
     * @return -1 if this block is not empty
     */
    int insert(std::string data);

    /**
     * @return the data of this block as a string
     */
    std::string get_data();

    /**
     * set this block as dirty
     */
    void set_dirty();

    /**
     * @return is this block dirty?
     */
    bool is_dirty();

private:
    static const int64_t SIZE_ = 4096;
    std::array<uint8_t, SIZE_> data_;
    bool dirty_;
    bool has_data = false;
    void insert_data(std::string data);

};

#endif
