#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define MAXPLAYERS 3

pthread_mutex_t entryMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t canPlayCond = PTHREAD_COND_INITIALIZER;
int numInGame = 0;

struct playArgs
{
    int playerId;
    int playTime;
};

void* playGame(void*);

int main(int argc, char** argv)
{
    int i = 0, j = 0;
    char playTimeStr[80];
    pthread_t players[10];
    struct playArgs pa[10];

    while((fgets(playTimeStr, 80, stdin) != NULL) && (i < 10)){
        pa[i].playerId = i;
        pa[i].playTime = atoi(playTimeStr);
        if(pthread_create(&players[i], NULL, playGame, (void*)&pa[i]) != 0)
            perror("Thread creation failed.");
        i++;
    }

    for(j = 0; j < i; j++){
        if(pthread_join(players[j], NULL) != 0)
            perror("Thread joining failed.");
    }

    pthread_mutex_destroy(&entryMutex);
    pthread_cond_destroy(&canPlayCond);

    printf("Main thread done.\n");
    return 0;
}


void* playGame(void *playArgs)
{
    struct playArgs pa = *((struct playArgs*)playArgs);

    pthread_mutex_lock(&entryMutex);
        while(numInGame >= MAXPLAYERS){
            pthread_cond_wait(&canPlayCond, &entryMutex);
        }
        printf("Player with ID %d playing for %d seconds...\n", 
                 pa.playerId, 
                 pa.playTime);
        numInGame++;
    pthread_mutex_unlock(&entryMutex);

    sleep(pa.playTime);

    pthread_mutex_lock(&entryMutex);
        numInGame--;
        if(numInGame < MAXPLAYERS) 
            pthread_cond_signal(&canPlayCond);
    pthread_mutex_unlock(&entryMutex);
}

