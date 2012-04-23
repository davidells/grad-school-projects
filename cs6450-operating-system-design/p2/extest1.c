#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "osdrpc.h"

void err_die(const char*);

int main(void)
{
    int rc, rank, nprocs;


    osdrpc_init();

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if(rank != 0){
        rc = osdrpc_call(0, "libosd1.so", "p1", "iis", 3, 4, "howdy");
        if(rc < 0) err_die("error in osdrpc_call");
        printf("%d: libosd1 proc p1 returned %d\n", rank, rc);

        rc = osdrpc_call(0, "libosd1.so", "p2", "iissisi", 3, 4, "howdy", "there", 567, "buddy", 890);
        if(rc < 0) err_die("error in osdrpc_call");
        printf("%d: libosd1 proc p2 returned %d\n", rank, rc);

        rc = osdrpc_call(0, "libosd1.so", "p3", "");
        if(rc < 0) err_die("error in osdrpc_call");
        printf("%d: libosd1 proc p3 returned %d\n", rank, rc);

        rc = osdrpc_call(0, "libosd2.so", "p1", "");
        if(rc < 0) err_die("error in osdrpc_call");
        printf("%d: libosd2 proc p1 returned %d\n", rank, rc);

        rc = osdrpc_call(0, "libosd2.so", "p4", "isi", 3, "howdy", 4);
        if(rc < 0) err_die("error in osdrpc_call");
        printf("%d: libosd2 proc p4 returned %d\n", rank, rc);
    } else {
        rc = osdrpc_call(1, "libosd2.so", "p4", "isi", 3, "howdy", 4);
        if(rc < 1) err_die("error in osdrpc_call");
        printf("%d: libosd2 proc p4 returned %d\n", rank, rc);
    }

    osdrpc_finalize();

    return 0;
}

void err_die(const char* message)
{
    perror(message);
    exit(-1);
}
