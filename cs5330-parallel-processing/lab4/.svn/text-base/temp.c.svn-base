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

#define PI 3.1415926535897932384626433

double func (double x);

int main(int argc, char** argv)
{
    int i, numintervals;
    double width, height;
    double approx = 0;

    if(argc != 2){
        printf("Usage: %s [number of intervals]\n", argv[0]);
        exit(-1);
    }
    numintervals = atoi(argv[1]);

    width = PI / (numintervals*1.0);
    for(i = 0; i < numintervals; i++){
        height = (func(i*width) + func((i+1)*width)) / 2.0;
        approx += (width * height);
    }
    printf("Approximation is %f\n", approx);

    return 0;
}

double func(double x)
{
    return sqrt(3.0 * pow( sin(x/2.0), 3.0 ));
}
