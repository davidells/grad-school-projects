//Written by David Ells
//
//Simple stack datatype implemented using kazlib's list datatype.

#include "stack.h"

void dsp_stack_init(dsp_stack_t *s, listcount_t max)
{
    /*if(s->list == NULL)
        s->list = list_create(max);*/

    list_init(s->list, max);
}

dsp_stack_t *dsp_stack_create(listcount_t max)
{
    dsp_stack_t *s = (dsp_stack_t*)malloc(sizeof(dsp_stack_t));
    if(s == NULL) return NULL;

    s->list = list_create(max);
    return s;
}

void dsp_stack_destroy(dsp_stack_t *s)
{
    list_destroy(s->list);
    free(s);
}


int dsp_stack_push(dsp_stack_t *s, void *data)
{
    lnode_t *n = lnode_create(data);
    if(n == NULL) return 0;
    list_append(s->list, n);
    return 1;
}

void *dsp_stack_pop(dsp_stack_t *s)
{
    lnode_t *n;

    if(dsp_stack_isempty(s)) return NULL;
    n = list_del_last(s->list);
    if(n == NULL) return NULL;
    return n->list_data;
}

void *dsp_stack_top(dsp_stack_t *s)
{
    lnode_t *n;

    if(dsp_stack_isempty(s)) return NULL;
    n = list_last(s->list);
    if(n == NULL) return NULL;
    return n->list_data;
}

void *dsp_stack_del_first(dsp_stack_t *s)
{
    lnode_t *n;
    
    if(dsp_stack_isempty(s)) return NULL;
    n = list_del_first(s->list);
    if(n == NULL) return NULL;
    return n->list_data;
}
    
int dsp_stack_size(dsp_stack_t *s)
{
    return list_count(s->list);
}

int dsp_stack_isempty(dsp_stack_t *s)
{
    return (dsp_stack_size(s) == 0);
}

