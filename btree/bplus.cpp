#include "bplus.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
using namespace std;
void
BPLUSTree::insert(int key, int val) {
    cout << "INSERTING " << to_string(key) << endl;
    if (cur_root == NULL) {
        // initialize root with this element
        cur_root = (node*)malloc(sizeof(node));
        // make leaf and put this ent in it
        node nroot;
        nroot.num_elements = 0;
        nroot.is_leaf = true;
        nroot.keys[0] = key;
        nroot.num_elements = 1;
        record* v = (record*)malloc(sizeof(record));
        v->value = val;
        nroot.values[0] = v;
        *cur_root = nroot;
        return;
    }
    // else find node
    node* target = find_node_to_store(key);

    // check if node is full
    if (target->num_elements == ORDER - 1) {
        // it needs to be split and inserted
        split_insert_node(target, key, val, false, NULL);
        cout << "TREE" << endl;
        print(cur_root);
        return;
    }
    insert_into_node(target, key, val, false, NULL);
    cout << "TREE" << endl;
    print(cur_root);
    return;
}

BPLUSTree::node*
BPLUSTree::split_insert_node(node* n, int k, int v, bool isparent, node* targ) {
    if (n->num_elements != ORDER - 1)
        return n;
    cout << "inserting " << to_string(k) << endl;
    cout << "splitting the following node" << endl;

    for (int ii = 0; ii < n->num_elements; ii++) {
        cout << "idx: " << to_string(ii) << " k " << n->keys[ii] << " ";
        cout << "with children" << endl;
        print(n->children[ii]);
    }
    cout << "with children" << endl;
    print(n->children[n->num_elements]);
    cout << endl;
    print(n);
    // get median idx, assuming order is odd
    int med_idx = ((ORDER-1) / 2);

    // make new leaves
    node* right = (node*)malloc(sizeof(node));
    right->is_leaf = true;

    // copy everything from [med, end] to new leaf
    int ii;
    int cur_idx = 0;
    int med_ke = n->keys[med_idx];
    if (isparent) {
        //node* rleaf = (node*)malloc(sizeof(node));
        // right child[0] == median + 1 child
        cout << "Contents of right..." << endl;
        for (ii = med_idx + 1; ii < ORDER - 1; ii++, cur_idx++) {
            right->num_elements++;
            right->keys[cur_idx] = n->keys[ii];
            right->children[cur_idx] = n->children[ii];
            //rleaf->keys[cur_idx] = n->keys[ii];
            //rleaf->values[cur_idx] = n->values[ii];
            cout << "idx: " << to_string(ii) << " key: " << to_string(n->keys[ii]);

            n->keys[ii] = -1;
            n->values[ii] = NULL;
            n->num_elements--;
        }
        right->children[cur_idx] = n->children[ii];
        cout << endl;
        // remove med from left
        n->keys[med_idx] = -1;
        n->values[med_idx] = NULL;
        n->num_elements--;
        right->is_leaf = false;
        insert_into_node(right, k, v, false, targ);
        cout << "printing stuff in targ" << endl;
        for (int jj = 0; jj < targ->num_elements; jj++) {
            cout << "idx: " << to_string(jj) << " key: " << to_string(targ->keys[jj]);

        }
        cout << endl;
        targ->parent = right;
        cout << "printing right's left child" << endl;
        print(right->children[0]);
        cout << "print right's right child" << endl;
        print(right->children[1]);
    } else {
        for (ii = med_idx; ii < ORDER - 1; ii++, cur_idx++) {
            right->num_elements++;
            right->keys[cur_idx] = n->keys[ii];
            right->children[cur_idx] = n->children[ii];
            right->values[cur_idx] = n->values[ii];
            cout << "idx: " << to_string(ii) << " key: " << to_string(n->keys[ii]);

            // delete now duplicate entry
            n->keys[ii] = -1;
            n->values[ii] = NULL;
        }
        cout << endl;
        n->num_elements = med_idx;

        // now we split the leaf, insert into the proper side
        if (k != -1 && v != -1) {
            insert_into_node(right, k, v, false, NULL);
        }
    }

    // now we need to fix the parents if needed, and fix parent pointers
    node* pparent = n->parent;
    int nkey = right->keys[0];
    if (isparent)
        nkey = med_ke;
    // make new root if parent is null
    if (pparent == NULL) {
        node* nroot = (node*)malloc(sizeof(node));
        nroot->num_elements = 1;
        nroot->is_leaf = false;
        // make median the only key in this new root
        nroot->keys[0] = nkey;
        // add children to this new root
        nroot->children[0] = n;
        nroot->children[1] = right;
        right->parent = nroot;
        n->parent = nroot;
        cur_root = nroot;
        return nroot;
    }
    // if it isn't null, then we check if it has room
    if (pparent->num_elements == ORDER - 1) {
        // no room, need to split parent now
        // THE RULES FOR SPLITTING ARE DIFFERENT TODO!!!
        pparent = split_insert_node(pparent, right->keys[0], -1, true, right);
    } else {

        // we can fit it in the parent
        insert_into_node(pparent, nkey, v, false, right);
        right->parent = pparent;
    }

    return pparent;
}

int
BPLUSTree::insert_into_node(node* n, int k, int v, bool isleft, node* child) {

    // find spot
    int cur_idx = 0;

    for (int ii = 0; ii < n->num_elements - 1; ii++) {

        if (n->keys[ii] > k) {
            cur_idx = ii;
        }
    }

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
        record* r = (record*)malloc(sizeof(record));
        r->value = v;
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
            return cur;
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
    cout << "NODE " << to_string(n->keys[0]) << endl;

    // print keys
    for (int ii = 0; ii < n->num_elements; ii++) {
        cout << "-";
    }
    for (int ii = 0; ii < n->num_elements; ii++) {
        cout << to_string(n->keys[ii]) << " ";
    }
    cout << endl;
    for (int ii = 0; ii < n->num_elements; ii++) {
        cout << "-";
    }
    cout << endl;
    //cout <<" num elements? " << to_string(n->num_elements);
    // print each sub tree
    if (n->is_leaf)
        return;

    for (int ii = 0; ii < n->num_elements + 1; ii++) {
        print(n->children[ii]);
    }

    return;
}
int
BPLUSTree::find(int key) {
    node* l = find_node_to_store(key);
    int idx = 0;
    for (int ii = 0; ii < l->num_elements; ii++) {
        if (l->keys[ii] == key) {
            idx = ii;
            break;
        }
    }
    return l->values[idx]->value;
}
