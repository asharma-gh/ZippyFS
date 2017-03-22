#ifndef B_PLUS_H
#define B_PLUS_H
#include <cstdint>
#include <iostream>
#define ORDER 100

class BPLUSTree {

  public:

    void insert(int ent);

    int find(int key);

  private:
    /** value that for each key */
    typedef struct record {
        int value;
    } record;

    /** node of a b+ tree */
    typedef struct node {
        int num_elements;
        int keys [ORDER - 1];
        /**children at key index gets u the node with keys < key, key index + 1 gets u the node with keys >= key */
        node* children[ORDER - 1];
        record** values;

    } node;

    node* cur_root = NULL;


};
#endif
