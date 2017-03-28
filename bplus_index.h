#ifndef BPLUS_INDEX_H
#define BPLUS_INDEX_H
#include "includes.h"
#include "inode.h"
#include "tire_fire.h"
#include "block.h"
#define ORDER 1000
/**
 * This class represents a memory-backed B+Tree used for indexing
 * inodes flushed from the in-memory file-system.
 *  - It will only support the limited functionality required by the file system, which is only insertion.
 * It utilizes the memory allocated dubbed TireFire, which also is implemented using bump-allocation, and thus free-ing is not available, and so deletions are not possible.
 *
 * @author Arvin Sharma
 */
class BPLUSIndex {

  public:
    /** represents an inode which is easily flushable to disk from memory */
    typedef struct inode {

        // stat info
        uint32_t mode;
        uint32_t nlink;
        uint64_t mtime;
        uint64_t ctime;
        uint64_t size;
        int deleted;
        // index to hash memory, always 512 size char*
        int64_t hash;
        // index to block data list memory
        int64_t block_data;
        // total size of block data
        uint64_t block_data_size;
    } inode;

    /** represents [block id, data] for inodes */
    typedef struct block_data {
        uint64_t block_id;
        uint64_t mtime;
        uint64_t size;
        // index to data memory
        int64_t data_offset;
    } block_data;

    /** represents a tree structure containing inodes */
    typedef struct node {
        int64_t parent = -1;
        /** array of pointers to char arrays */
        int64_t keys[ORDER - 1] = {-1};
        int64_t num_keys = 0;
        int is_leaf = 0;
        /** child nodes */
        int64_t children[ORDER] = {-1};
        /** list of inodes if this node is a leaf */
        int64_t inodes;
        int64_t values_size = 0;
    } node;

    /** adds the given inode to this B+Tree */
    void add_inode(Inode in, std::map<uint64_t, std::shared_ptr<Block>> dirty_blocks,
                   std::map<uint64_t, unsigned long long> block_mtime);

    /**
     * constructs a B+Tree
     * @num_ents is the number of values that will exist in this B+tree
     */
    BPLUSIndex(uint64_t num_ents);

    /**
     * Destructor, flushes everything to disk
     */
    ~BPLUSIndex();

  private:
    /** number of entries*/
    uint64_t num_ents_ = 0;
    inode* inode_arr_ptr_ = nullptr;
    int64_t inode_arr_idx_ = 0;
    int64_t cur_inode_arr_idx_ = 0;
    /** in memory structure */
    TireFire mem_;
    int64_t rootidx_ = 0;
    /** maintain pointer to root */
    node* root_ptr_ = nullptr;
    bool root_has_inode_ = false;

    /** contingeous list of inodes for this B+Tree */
    int64_t inodes_idx_ = 0;

    /** @returns the index of the node to store the given key */
    int64_t find_node_to_store(std::string key);

    /** helpers to find a child */
    int64_t before(node* n, int64_t idx);
    int64_t after(node* n, int64_t idx);

    /** inserts the k,v in the given NON-EMPTY leaf-node, assuming ALL k's are UNIQUE
     * @return the index that the k,v pair was inserted in
     * if n is not an internal node, the child is inserted instead
     */
    int insert_into_node(int64_t nodeidx, int k, inode v, bool isleft, int64_t child);

    /** splits the given node, inserts the given k,v
     * MODIFIES cur_root
     * @return the new root of the tree with the k,v inserted
     */
    /** @params k,v == -1 then no k,v pair is inserted into the newly allocated node
     *  @params isparent changes how this functon splits, assuming that it is splitting a parent node.
     *
     */
    int64_t split_insert_node(int64_t n, int k, inode v, bool isparent, int64_t targ);
};
#endif
