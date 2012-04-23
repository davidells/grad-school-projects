//Written by David Ells
//
//Simple stack datatype implemented using kazlib's list datatype.

#include "stack.h"

void stack_init(stack_t *s, listcount_t max)
{
    /*if(s->list == NULL)
        s->list = list_create(max);*/

    list_init(s->list, max);
}

stack_t *stack_create(listcount_t max)
{
    stack_t *s = (stack_t*)malloc(sizeof(stack_t));
    if(s == NULL) return NULL;

    s->list = list_create(max);
    return s;
}

void stack_destroy(stack_t *s)
{
    list_destroy(s->list);
    free(s);
}


int stack_push(stack_t *s, void *data)
{
    lnode_t *n = lnode_create(data);
    if(n == NULL) return 0;
    list_append(s->list, n);
    return 1;
}

void *stack_pop(stack_t *s)
{
    lnode_t *n;

    if(stack_isempty(s)) return NULL;
    n = list_del_last(s->list);
    if(n == NULL) return NULL;
    return n->list_data;
}

void *stack_top(stack_t *s)
{
    lnode_t *n;

    if(stack_isempty(s)) return NULL;
    n = list_last(s->list);
    if(n == NULL) return NULL;
    return n->list_data;
}

void *stack_del_first(stack_t *s)
{
    lnode_t *n;
    
    if(stack_isempty(s)) return NULL;
    n = list_del_first(s->list);
    if(n == NULL) return NULL;
    return n->list_data;
}
    
int stack_size(stack_t *s)
{
    return list_count(s->list);
}

int stack_isempty(stack_t *s)
{
    return (stack_size(s) == 0);
}

