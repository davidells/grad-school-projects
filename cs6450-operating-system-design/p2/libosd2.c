//Written by David Ells (and ONLY David Ells :)
//
// A simple example of a shared library that is
// used in an rpc project for CS6450 - Op Sys Design

#include <stdio.h>

int p1(void *args[]){
    printf("libosd2 procedure p1 called\n");
    return 101;
}

int p4(void *args[]){
    int *i1 = args[0];
    char *c1 = args[1];
    int *i2 = args[2];
    printf("libosd2 procedure p4 called\n");
    printf("libosd2 procedure p4 args: %d, %s, %d\n", *i1, c1, *i2);
    return 104;
}
