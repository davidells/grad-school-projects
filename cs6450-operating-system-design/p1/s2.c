#include "dsm.h"

int main(int argc, char *argv[])
{
    int rc, nprocs, myrank;

    rc = dsm_init();
    nprocs = dsm_nprocs();
    myrank = dsm_rank();
    if (nprocs != 2)
    {
	printf("BUMMER: only work with 2 processes\n");
	exit(-1);
    }
    rc = dsm_mutex_lock(5);
    printf("GOT IT\n");
    sleep(4);
    rc = dsm_mutex_unlock(5);
    printf("DONE\n");
    rc = dsm_finalize();
}
