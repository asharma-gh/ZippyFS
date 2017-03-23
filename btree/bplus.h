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

    void print();

  private:
    /** value that for each key */
    typedef struct record {
        int value;
    } record;

    /** node of a b+ tree */
    // node* means childen is a node***!
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

    /** inserts the k,v in the given NON-EMPTY leaf-node, assuming ALL k's are UNIQUE
     * @return the index that the k,v pair was inserted in*/
    /** if n is not a leaf, the child is inserted instead */
    int insert_into_node(node* n, int k, int v, bool isleft, node* child);

    /** splits the given node, inserts the given k,v
     * MODIFIES cur_root
     * @return the new root of the tree with the k,v inserted
     */
    /** if k,v == -1 then the node is split and the new root is returned */
    node* split_insert_node(node* n, int k, int v);
    /** prints the b+tree */
    void print(node* n);
};
#endif
