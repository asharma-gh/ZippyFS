#ifndef B_PLUS_H
#define B_PLUS_H
#include <cstdint>
#include <iostream>
#define ORDER 100
/** this is prolly a memory leaking nightmare */
// TODO: fix comments and function names
class BPLUSTree {

  public:

    /** inserts the given key,val pair to this b+tree */
    void insert(int key, int val);

    /** prints the b+tree */
    void print();

  private:
    /** value that for each key */
    typedef struct record {
        int value;
    } record;

    /** node of a b+ tree */
    typedef struct node {
        node* parent = NULL;
        int num_elements = 0;
        bool is_leaf = false;
        int keys [ORDER - 1] = {-1};
        node* children[ORDER] = {NULL};
        record values[ORDER];

    } node;

    /** current root for this b+tree */
    node* cur_root = NULL;

    /** returns the child for the given node and key */
    node* before(node* n, int k);
    node* after(node* n, int k);

    /** finds the node to store the given key */
    node* find_node_to_store(int k);

    /** inserts the k,v in the given NON-EMPTY leaf-node, assuming ALL k's are UNIQUE */
    void insert_into_node(node* n, int k, int v);

    /** splits the given node, inserts the given k,v
     * MODIFIES cur_root
     * @return the node to insert the given key
     */
    node* split_insert_node(node* n, int k, int v);
};
#endif
