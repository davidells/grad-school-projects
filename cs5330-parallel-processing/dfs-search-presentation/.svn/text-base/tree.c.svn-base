//Written by David Ells
//
//A simple binary tree ADT.

#include "tree.h"

void treenode_init(treenode *n)
{
    n->data = NULL;
    n->left = NULL;
    n->right = NULL;
}

void treenode_print(treenode *n, FILE *f)
{
    fprintf(f, "node %d:", n->id);
    if(n->left != NULL)
        fprintf(f, " left:%d", n->left->id);
    if(n->right != NULL)
        fprintf(f, " right:%d", n->right->id);
    fprintf(f, "\n");
}

int treenode_isleaf(treenode *n)
{
    return (n->left == NULL && n->right == NULL);
}


void tree_init(tree *t)
{
    t->head = NULL;
    t->node_count = 0;
}

void tree_print_r(treenode *n, FILE *f)
{
    if(n == NULL) return;
    treenode_print(n, f);
    tree_print_r(n->left, f);
    tree_print_r(n->right, f);
}

void tree_print(tree *t, FILE *f)
{
    tree_print_r(t->head, f);
}

void tree_visit_r(treenode *n, treenode_func func, void *arg)
{
    if(n == NULL) return;
    (*func)(n, arg);
    tree_visit_r(n->left, func, arg);
    tree_visit_r(n->right, func, arg);
}

void tree_visit(tree *t, treenode_func fun, void *arg)
{
    tree_visit_r(t->head, fun, arg);
}
