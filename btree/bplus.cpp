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
    cout << "printing things in target..." << endl;
    for (int ii = 0; ii < target->num_elements; ii++) {
        cout << "|" << to_string(target->keys[ii]) << "|";
    }
    cout << endl;
    // check if node is full
    if (target->num_elements == ORDER - 1) {
        // it needs to be split and inserted
        split_insert_node(target, key, val);
        return;
    }
    insert_into_node(target, key, val, false, NULL);
    return;


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
        n->num_elements == 0 ? 0 : n->num_elements--;
    }
    // get last child
    right->children[ii] = n->children[ii];
    n->children[ii] = NULL;
    n->num_elements = med_idx - 1;

    // now we split the leaf, insert into the proper side
    if (k != -1 && v != -1) {
        if (is_left_part)
            insert_into_node(n, k, v, false, NULL);
        else
            insert_into_node(right, k, v, false, NULL);
    }

    // now we need to fix the parents if needed, and fix parent pointers
    node* pparent = n->parent;

    // make new root if parent is null
    if (pparent == NULL) {
        node* nroot = (node*)malloc(sizeof(node));
        nroot->num_elements = 1;
        nroot->is_leaf = false;
        // make median the only key in this new root
        nroot->keys[0] = right->keys[0];
        // add children to this new root
        nroot->children[0] = n;
        nroot->children[1] = right;
        // split is complete.
        cur_root = nroot;
        return nroot;
    }
    // if it isn't null, then we check if it has room
    if (pparent->num_elements == ORDER - 1) {
        // no room, need to split parent now
        split_insert_node(pparent, -1, -1);
    }
    // we can fit it in the parent
    insert_into_node(pparent, k, v, false, right);



    return pparent;
}

int
BPLUSTree::insert_into_node(node* n, int k, int v, bool isleft, node* child) {

    // find spot
    int cur_idx = 0;
    cout << "num elements: " << to_string(n->num_elements) << endl;
    cout << "Listing things: ";
    for (int ii = 0; ii < n->num_elements - 1; ii++) {
        cout << "|"<<to_string(n->keys[ii]) << "|";
        if (n->keys[ii] > k) {
            cur_idx = ii;
        }
    }
    cout << endl;
    cout << "cur idx:" << to_string(cur_idx) << " for k:" << k << endl;

    if (cur_idx == 0 && n->num_elements > 0) {
        cur_idx = n->num_elements;
    } else {
        // move everything from cur_idx + 1 up
        node temp = *n;
        for (int ii = cur_idx; ii < n->num_elements - 1; ii++) {
            n->keys[ii + 1] = temp.keys[ii];
            n->children[ii + 1] = temp.children[ii];
            n->values[ii + 1] = temp.values[ii];
        }


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
        if (!isleft)
            cur_idx++;
        n->children[cur_idx] = child;
    }
    return cur_idx;
}

BPLUSTree::node*
BPLUSTree::find_node_to_store(int k) {
    if (cur_root == NULL)
        throw domain_error ("tree is empty");

    node* cur = cur_root;
    for (;;) {
        if (cur->is_leaf) {
            cout << "cur is leaf" << endl;
            return cur;
        }

        // is this key before all in  this node?
        if (k < cur->keys[0]) {
            cur = before(cur, 0);
            cout << "cur is before" << endl;
            continue;

        }

        // is this key after all of the ones in this node?
        if (k >= cur->keys[cur->num_elements - 1]) {
            cur = after(cur, cur->num_elements - 1);
            cout << "cur is after" << endl;
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
void
BPLUSTree::print() {
    print(cur_root);
}
void
BPLUSTree::print(node* n) {
    if (n == NULL)
        return;
    // print keys
    for (int ii = 0; ii < n->num_elements; ii++) {
        cout << "---";
    }
    cout << endl;
    for (int ii = 0; ii < n->num_elements; ii++) {
        cout << to_string(n->keys[ii]) << " ";
    }
    cout << endl;
    for (int ii = 0; ii < n->num_elements; ii++) {
        cout << "---";
    }
    cout << endl;

    // print each sub tree

    for (int ii = 0; ii < n->num_elements + 1; ii++) {
        print(n->children[0]);
    }
    return;

}
