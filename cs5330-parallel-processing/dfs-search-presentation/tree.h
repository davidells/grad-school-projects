//Written by David Ells
//
//A simple binary tree ADT.

#include <stdio.h>

typedef struct treenode {
    int id;
    void *data;
    struct treenode *left;
    struct treenode *right;
} treenode;

typedef struct {
    treenode *head;
    unsigned long node_count;
} tree;

typedef void (*treenode_func)(treenode *, void *);

void treenode_init(treenode *);
int treenode_isleaf(treenode *);
void treenode_print(treenode *, FILE *);
void tree_init(tree *);
void tree_print_r(treenode *, FILE *);
void tree_print(tree *, FILE *);
void tree_visit_r(treenode *, treenode_func, void *);
void tree_visit(tree *, treenode_func, void *);
