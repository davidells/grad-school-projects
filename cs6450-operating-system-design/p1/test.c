#include "dsm.h"

int main(int argc, char *argv[])
{
    int i,rc,nprocs,myrank;
    int *p;

    rc = dsm_init();
    nprocs = dsm_nprocs();
    myrank = dsm_rank();
    p = dsm_malloc();
    if (myrank == 0)
	p[nprocs] = 0;
    rc = dsm_barrier();
    dsm_mutex_lock(0);
    p[myrank] = myrank;
    p[nprocs]++;
    rc = dsm_sync();
    dsm_mutex_unlock(0);
    rc = dsm_barrier();
    for (i=0; i < nprocs; i++)
	printf("%d\n",p[i]);
    printf("%d\n",p[nprocs]);

    rc = dsm_finalize();
}
