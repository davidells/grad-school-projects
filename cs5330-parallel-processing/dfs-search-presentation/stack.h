//Written by David Ells
//
//Simple stack datatype implemented using kazlib's list datatype.

#include <stdio.h>
#include <stdlib.h>
#include "list.h"

typedef struct {
    list_t *list;
} stack_t;

void stack_init(stack_t *, listcount_t);
stack_t *stack_create(listcount_t);
void stack_destroy(stack_t *);
void stack_destroy_nodes(stack_t *);

int stack_push(stack_t *, void *);
void *stack_pop(stack_t *);
void *stack_top(stack_t *);
void *stack_del_first(stack_t *);
int stack_size(stack_t *);
int stack_isempty(stack_t *);

