#include "bplus.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
using namespace std;
void
BPLUSTree::insert(int key, int val) {
    if (cur_root == NULL) {
        // initialize root with this element
        cur_root = (node*)malloc(sizeof(node));
        // make leaf and put this ent in it
        node nroot;
        nroot.num_elements = 0;
        nroot.is_leaf = true;
        nroot.keys[0] = key;
        nroot.num_elements = 1;
        record v;
        v.value = val;
        nroot.values[0] = v;
        *cur_root = nroot;
        return;
    }
    // else find node
    // insert to node
}


BPLUSTree::node*
BPLUSTree::find_node(int k) {
    if (cur_root == NULL) {
        return NULL;
    }

    node* cur = cur_root;
    for (;;) {
        if (cur->is_leaf) {
            // then the key must be in here
            for (int key : cur->keys) {
                if (key == k)
                    return cur;
            }
            return NULL;
        }

        // is this key before all in  this node?
        if (k < cur->keys[0]) {
            cur = before(cur, 0);
            continue;

        }

        // is this key after all of the ones in this node?
        if (k >= cur->keys[cur->num_elements - 1]) {
            cur = after(cur, cur->num_elements - 1);
            continue;
        }
        // else it must be somewhere in between
        int cur_idx = 0;
        for (int ii = 1; ii < cur->num_elements - 1; ii++) {
            if (cur->keys[ii] <= k
                    && k < cur->keys[ii + 1])
                cur_idx = ii;
        }
        cur = after(cur, cur_idx);
    }


    return cur;
}

void
BPLUSTree::insert_to_node(int k, int v) {
    (void)k;
    (void)v;
}


BPLUSTree::node*
BPLUSTree::before(node* n, int k) {
    return n->children[k];
}

BPLUSTree::node*
BPLUSTree::after(node* n, int k) {
    return n->children[k + 1];
}
