#include <stdio.h>
#include <pthread.h>

#define MAX_PLAYERS 10
#define CONCURRENT_PLAYERS 3

int playersDone = 0;
int playTimes[MAX_PLAYERS];
pthread_mutex_t entryMutex = PTHREAD_MUTEX_INITIALIZER;

void* runClient(void*);


int main(int argc, char** argv)
{
    int i;
    char playTimeStr[80];
    pthread_t clientThreads[CONCURRENT_PLAYERS];


    for(i = 0; i < MAX_PLAYERS; i++){
	scanf("%d", &playTimes[i]);
    }

    for(i = 0; i < CONCURRENT_PLAYERS; i++){
        if(pthread_create(&clientThreads[i], NULL, runClient, NULL) != 0)
	        perror("Thread creation failed.");
    }

    for(i = 0; i < CONCURRENT_PLAYERS; i++){
        if(pthread_join(clientThreads[i], NULL) != 0)
            perror("Thread joining failed.");
    }

    pthread_mutex_destroy(&entryMutex);

    printf("Main thread done.\n");
    return 0;
}


void* runClient(void *ignore)
{
    int curPlayerId, curPlayTime;

    while(playersDone < MAX_PLAYERS){

        pthread_mutex_lock(&entryMutex);
            if(playersDone < MAX_PLAYERS){
                curPlayerId = playersDone;
                curPlayTime = playTimes[playersDone];
                playersDone++;
            } else {
                pthread_mutex_unlock(&entryMutex);
                pthread_exit(0);
            }
        pthread_mutex_unlock(&entryMutex);
	    
        printf("Player with ID %d playing for %d seconds...\n", 
             curPlayerId, 
             curPlayTime);

        sleep(curPlayTime);
    }
}

