#include <stdio.h>
#include <stdlib.h>

int randint(int max)
{
    return ((double)rand() / (double)RAND_MAX) * max;
}

int main(int argc, char *argv[])
{
    int randseed, num_ints, i;

    if(argc != 3){
        printf("usage: randint [rand seed] [number of ints]\n");
        exit(1);
    }
    randseed = atoi(argv[1]);
    num_ints = atoi(argv[2]);

    srand(randseed);
    for(i = 0; i < num_ints; i++){
        printf("%d\n", randint(num_ints));
    }

    return 0;
}
