#define BLOCK_H
#ifndef BLOCK_H

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
    Block(string data);

    /**
     * adds the given data to this empty block
     * @param data is the data to add
     * @return 0 for success, nonzero otherwise.
     * @return -1 if this block is not empty
     */
    int insert(string data);

    /**
     * @return the data of this block as a string
     */
    string get_data();

    /**
     * set this block as dirty
     */
    void set_dirty() noexcept;

    /**
     * @return is this block dirty?
     */
    boolean is_dirty() const noexcept;

private:
    static const int64_t SIZE_ = 4000;
    array<uint8_t, SIZE_> data_;
    bool dirty_;
};

#endif
