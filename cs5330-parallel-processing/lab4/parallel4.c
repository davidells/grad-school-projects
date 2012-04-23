/* Written by David Ells
 *
 * This program was written as an assignment in Dr. Pettey's parallel
 * processing course. It is intended to show a simple example of using
 * MPI to pass messages among processes. The code approximates the integral
 * of function func. It reduces the answer to rank 0 in two ways, first
 * by sending the partial sum around the ring, adding each answer as it
 * is passed, secondly by using an MPI operation to reduce.
 *
 */

#include <math.h>
#include <stdio.h>
#include "/usr/local/home/accts/pettey/mpich2-1.0.5p3/mpich2i/include/mpi.h"

#define PI 3.1415926535897932384626433

double func (double x);

int main(int argc, char** argv)
{
    int i, next, prev;
    double width, height, myanswer;
    double sum1, sum2;
    int rank, nprocs;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    next = (rank+1)%nprocs;
    prev = (rank+nprocs-1)%nprocs;

    width = PI / (nprocs*1.0);
    height = (func(rank*width) + func((rank+1)*width)) / 2.0;
    myanswer = width * height;


    if(rank == 0){
        //Send off token to sum partial sums. Wait until all the way around ring.
        sum1 = myanswer;
        MPI_Send(&sum1, 1, MPI_DOUBLE, 1, 1, MPI_COMM_WORLD);
        MPI_Recv(&sum1, 1, MPI_DOUBLE, (nprocs-1), 1, MPI_COMM_WORLD, &status);
        printf("Ring sum answer is %f\n", sum1);

    } else {
        //Wait for partial sum, add my answer to the sum, send to next process.
        MPI_Recv(&sum1, 1, MPI_DOUBLE, prev, 1, MPI_COMM_WORLD, &status);
        sum1 += myanswer;
        MPI_Send(&sum1, 1, MPI_DOUBLE, next, 1, MPI_COMM_WORLD);
    }


    //Use MPI_Reduce to reduce using addition.
    MPI_Reduce(&myanswer, &sum2, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    if(rank == 0){
        printf("MPI_Reduce answer is %f\n", sum2);
    }


    MPI_Finalize();
    return 0;
}

double func(double x)
{
    return sqrt(3.0 * pow( sin(x/2.0), 3.0 ));
}
