//Written by David Ells
//
//Simple stack datatype implemented using kazlib's list datatype.

#include <stdio.h>
#include <stdlib.h>
#include "list.h"

typedef struct {
    list_t *list;
} dsp_stack_t;

void dsp_stack_init(dsp_stack_t *, listcount_t);
dsp_stack_t *dsp_stack_create(listcount_t);
void dsp_stack_destroy(dsp_stack_t *);
void dsp_stack_destroy_nodes(dsp_stack_t *);

int dsp_stack_push(dsp_stack_t *, void *);
void *dsp_stack_pop(dsp_stack_t *);
void *dsp_stack_top(dsp_stack_t *);
void *dsp_stack_del_first(dsp_stack_t *);
int dsp_stack_size(dsp_stack_t *);
int dsp_stack_isempty(dsp_stack_t *);

