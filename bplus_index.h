#ifndef BPLUS_INDEX_H
#define BPLUS_INDEX_H
#include "includes.h"
#include "inode.h"
#include "tire_fire.h"
#include "block.h"
#define ORDER 24
#define HASH_SIZE 128
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
    typedef struct header {
        uint64_t num_inodes;
        // offset into inode list
        int64_t inode_list;
        int64_t root;
    } header;
    /** represents an inode which is easily flushable to disk from memory */
    typedef struct inode {

        // stat info
        uint32_t mode;
        uint32_t nlink;
        uint64_t mtime;
        uint64_t ctime;
        uint64_t size;
        int deleted;
        // offset to the path in memory
        uint64_t path;
        uint64_t path_size;
        // offset to hash in memory
        uint64_t hash;
        // index to block data list in memory
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

    /** TODO: refactor node to internal_node */
    /** represents a tree structure containing inodes */
    typedef struct node {
        int64_t parent = -1;
        /** array of offsets to char arrays */
        int64_t keys[ORDER - 1] = {-1};
        int64_t num_keys = 0;
        bool is_leaf = 0;
        /** offset to child nodes */
        uint64_t children[ORDER] = {0};
        /** list of offsets for inodes if this node is a leaf */
        int64_t values[ORDER - 1] = {-1};
    } node;

    /** represents a lead node for a B+Tree */
    typedef struct leaf_node {
        int64_t parent = -1;
        /** array of offsets to char arrays */
        int64_t keys[ORDER - 1] = {-1};
        int64_t num_keys = 0;
        bool is_leaf = 1;
        /** list of offsets for inodes if this node is a leaf */
        int64_t values[ORDER - 1] = {-1};

    } leaf_node;

    /** adds the given inode to this B+Tree */
    void add_inode(Inode in, std::map<uint64_t, std::shared_ptr<Block>> dirty_blocks, std::map<uint64_t, unsigned long long> block_mtime);

    /**
     * constructs a B+Tree
     * @num_ents is the number of values that will exist in this B+tree
     */
    BPLUSIndex(uint64_t num_ents, uint64_t block_size, uint64_t num_blocks, uint64_t paths_size);
    bool find(std::string p);
    /**
     * Destructor, flushes everything to disk
     */
    ~BPLUSIndex();

  private:
    /** number of entries*/
    uint64_t num_ents_ = 0;
    uint64_t num_blocks_ = 0;
    uint64_t path_size_ = 0;
    int64_t path_arr_idx_ = 0;
    int64_t cur_path_idx_ = 0;
    int64_t inode_arr_idx_ = 0;
    int64_t cur_inode_arr_idx_ = 0;
    /** in memory structure */
    TireFire mem_;
    int64_t rootidx_ = 0;

    int64_t headeridx_ = 0;
    /** maintain pointer to root */
    node* root_ptr_ = nullptr;
    bool root_has_inode_ = false;

    /** contingeous list of blocks for inodes -- done to speed up allocation */
    int64_t block_size_ = 0;
    int64_t block_arr_idx_ = 0;
    int64_t cur_block_arr_idx_ = 0;

    int64_t bd_arr_idx_ = 0;
    int64_t cur_bd_arr_idx_ = 0;
    /** contingeous list of hashes for inodes
     * size is = hashsize * num_ents */
    int64_t hash_arr_idx_ = 0;
    int64_t cur_hash_arr_idx_ = 0;

    /** @returns the index of the node to store the given key */
    int64_t find_node_to_store(std::string key);

    /** helpers to find a child */
    int64_t before(node* n, int64_t idx);
    int64_t after(node* n, int64_t idx);

    /** inserts the k,v in the given NON-EMPTY leaf-node, assuming ALL k's are UNIQUE
     * @return the index that the k,v pair was inserted in
     * if n is not an internal node, the child is inserted instead
     */
    int insert_into_node(uint64_t nodeoffset, int64_t k, int64_t v, bool isleft, int64_t child);

    /** splits the given node, inserts the given k,v
     * MODIFIES cur_root
     * @return the new root of the tree with the k,v inserted
     */
    /** @params k,v == -1 then no k,v pair is inserted into the newly allocated node
     *  @params isparent changes how this functon splits, assuming that it is splitting a parent node.
     *
     */
    int64_t split_insert_node(uint64_t nodeoffset, int64_t k, int64_t v, bool isparent, int64_t targ);

    void print(int64_t n);
};
#endif
