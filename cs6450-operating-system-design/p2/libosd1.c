//Written by David Ells (and ONLY David Ells :)
//
// A simple example of a shared library that is
// used in an rpc project for CS6450 - Op Sys Design

#include <stdio.h>

int p1(void *args[]){
    printf("libosd1 procedure p1 called\n");
    return 1;
}

int p2(void *args[]){
    printf("libosd1 procedure p2 called\n");
    return 2;
}

int p3(void *args[]){
    printf("libosd1 procedure p1 called\n");
    return 3;
}
