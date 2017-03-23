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
    node* target = find_node_to_store(key);

    // check if node is full
    if (target->num_elements == ORDER - 1) {
        // it needs to be split
        //node* fixed_node = split_insert_node(target, key, val);
    }
}

BPLUSTree::node*
BPLUSTree::split_insert_node(node* n, int k, int v) {
    if (n->num_elements != ORDER - 1)
        return n;

    // get median idx, assuming order is odd
    int med_idx = (ORDER / 2) + 2;

    // check if this key belongs in the new left
    bool is_left_part = n->keys[med_idx] < k;

    // make new leaves
    node* right = (node*)malloc(sizeof(node));
    right->is_leaf = true;
    // copy everything from [med, end] to new leaf
    int ii;
    int cur_idx = 0;
    for (ii = med_idx; ii < ORDER - 1; ii++, cur_idx++) {
        right->num_elements++;
        right->keys[cur_idx] = n->keys[ii];
        right->children[cur_idx] = n->children[ii];
        right->values[cur_idx] = n->values[ii];
        // delete now duplicate entry
        n->keys[ii] = -1;
        n->children[ii] = NULL;
    }
    // get last child
    right->children[ii] = n->children[ii];
    n->children[ii] = NULL;
    n->num_elements = med_idx - 1;

    // now we split the leaf, insert into the proper side
    if (is_left_part)
        insert_into_node(n, k, v, NULL);
    else
        insert_into_node(right, k, v, NULL);

    // now we need to fix the parents if needed, and fix parent pointers
    return right;
}
void
BPLUSTree::insert_into_node(node* n, int k, int v, node* child) {

    // find spot
    int cur_idx = 0;
    for (int ii = 0; ii < n->num_elements - 1; ii++) {
        if (n->keys[ii] < k) {
            cur_idx = ii;
            break;
        }
    }

    // move everything from cur_idx + 1 up
    node temp = *n;
    for (int ii = cur_idx; ii < n->num_elements - 1; ii++) {
        n->keys[ii + 1] = temp.keys[ii];
        n->children[ii + 1] = temp.children[ii];
        n->values[ii + 1] = temp.values[ii];
    }

    n->keys[cur_idx] = k;
    n->num_elements++;

    if (n->is_leaf) {
        // insert value into fixed node
        record r;
        r.value = v;
        n->values[cur_idx] = r;
        n->children[cur_idx] = NULL;
    } else {
        // insert child
        n->children[cur_idx] = child;
    }
}

BPLUSTree::node*
BPLUSTree::find_node_to_store(int k) {
    if (cur_root == NULL)
        throw domain_error ("tree is empty");

    node* cur = cur_root;
    for (;;) {
        if (cur->is_leaf)
            return cur;


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

BPLUSTree::node*
BPLUSTree::before(node* n, int k) {
    return n->children[k];
}

BPLUSTree::node*
BPLUSTree::after(node* n, int k) {
    return n->children[k + 1];
}
