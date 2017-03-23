#ifndef B_PLUS_H
#define B_PLUS_H
#include <cstdint>
#include <iostream>
#define ORDER 100

class BPLUSTree {

  public:

    void insert(int key, int val);


  private:
    /** value that for each key */
    typedef struct record {
        int value;
    } record;

    /** node of a b+ tree */
    typedef struct node {
        int num_elements;
        bool is_leaf = false;
        int keys [ORDER - 1] = {-1};
        node* children[ORDER] = {NULL};
        record values[ORDER];

    } node;

    node* cur_root = NULL;

    /** returns the child for the given node and key */
    node* before(node* n, int k);
    node* after(node* n, int k);

    node* find_node(int k);

    void insert_to_node(int k, int v);
};
#endif
