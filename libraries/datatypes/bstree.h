#ifndef BSTREE_H
#define BSTREE_H

#include <stdbool.h>
#include "common.h"

typedef struct _bstree {
    struct _bstree *left, *right;
    ptr data;
} bstree;

void bst_free(bstree *bst);

// Tree mutation
bstree *bst_add(bstree *me, ptr data);
bstree *bst_remove(bstree *me, ptr data);

// Finding nodes
bstree *bst_find(bstree *bst, ptr data);
bstree *bst_find_lower_bound(bstree *me, ptr data, bstree *best);
bstree *bst_min_node(bstree *bst);
bstree *bst_max_node(bstree *bst);

// Info
size_t bst_size(bstree *bst);

// Dumping
void bst_print(bstree *me, int indent, bool print_null);

#endif