#include "dsm.h"

int main(int argc, char *argv[])
{
    int i,rc,nprocs,myrank;
    int *p[4];

    rc = dsm_init();
    nprocs = dsm_nprocs();
    myrank = dsm_rank();
    if (nprocs < 1  ||  nprocs > 4)
    {
	printf("BUMMER: only work 1-4 processes\n");
	exit(-1);
    }
    p[0] = dsm_malloc();
    p[1] = dsm_malloc();
    p[2] = dsm_malloc();
    p[3] = dsm_malloc();
    p[myrank][100] = myrank;
    rc = dsm_sync();
    rc = dsm_barrier();
    if (myrank == 0)
    {
	for (i=0; i < nprocs; i++)
	    printf("%d\n",p[i][100]);
    }

    rc = dsm_finalize();
}
