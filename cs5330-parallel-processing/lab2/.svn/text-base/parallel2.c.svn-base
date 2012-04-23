#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* 
 * Written by David Ells 
 *
 * Program written for Dr. Pettey's Parallel Processing course (CS5330).
 * This program simulates a game involving "players" who are "teleported"
 * from "continent" to "continent", fighting each other and avoiding the
 * "dragon", who will eat the players. As the players are shifted and killed,
 * they are added to the global queue for reentry into the world. Detailed
 * rules for the game are not specified here.
 *
 */

//Game constants
#define NUM_OF_CONTINENTS 4
#define PLAYERS_PER_CONTINENT 5
#define MAX_PLAYERS 30
#define CONTINENT_QUEUE_LENGTH 2
#define TOTAL_EPOCHS 10
#define LAB2_CONFIG_FILE "players.dat"


//Game types
typedef struct {
    int id;
    char type;
    int alive;
} player_t;


//Game variables
player_t globalPlayerQueue[MAX_PLAYERS];
player_t continentPlayers[NUM_OF_CONTINENTS][PLAYERS_PER_CONTINENT];
player_t continentQueuedPlayers[NUM_OF_CONTINENTS][CONTINENT_QUEUE_LENGTH];

int totalPlayers = 0;
int globalQueueGetIndex = 0;
int globalQueuePutIndex = 0;
int epoch = 0;
int continentsDone = 0;
int dragonContinent = 0;

pthread_mutex_t barrierMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t globalQueueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t barrierCondition = PTHREAD_COND_INITIALIZER;
pthread_cond_t printWorldCond = PTHREAD_COND_INITIALIZER;


//Function prototypes
void initWorld();
void printWorld();
void printPlayersInContinent(int continentId);
void printPlayersInContinentQueue(int continentId);

void printGlobalPlayerQueue(int);
player_t getNextPlayerFromGlobalQueue();
void putPlayerOnGlobalQueue(player_t*);

void printPlayer(player_t);
int clearPlayer(player_t*);
int isAlivePlayer(player_t);
int isNullPlayer(player_t);

void moveDragon();
void conductPlayerBattle(player_t*);
void feedDragon(player_t*);
void reapDeadPlayers(player_t*);
void compressContinentPlayerList(player_t*);
void refillPlayersWithGlobalQueue(player_t*);

void* startContinent(void*);


int main (int argc, char** argv)
{
    int i;

    int con_id[NUM_OF_CONTINENTS];
    pthread_t continents[NUM_OF_CONTINENTS];


    //Initialize the game world.
    initWorld();

    //Print initial world set up.
    printf("Initial world :\n");
    printWorld();

    //Create Threads
    for(i = 0; i < NUM_OF_CONTINENTS; i++){
        con_id[i] = i;
        if(pthread_create(&continents[i], 
                          NULL, 
                          startContinent, 
                          (void*)&con_id[i]) != 0)
            perror("Thread creation failed.");
    }

    //Main prints the world after each epoch.
    while (epoch < TOTAL_EPOCHS){
        pthread_mutex_lock(&barrierMutex);
            pthread_cond_wait(&printWorldCond, &barrierMutex);
            printf("The World After %d Epochs\n", epoch);
            printWorld();
            pthread_cond_broadcast(&barrierCondition);
        pthread_mutex_unlock(&barrierMutex);
    }

    //Join Threads
    for(i = 0; i < NUM_OF_CONTINENTS; i++){
        if(pthread_join(continents[i], NULL) != 0)
            perror("Thread joining failed.");
    }
    return 0;
}


void* startContinent(void *continentId)
{
    int i, j;

    int cid = *((int*) continentId);
    int nextContinentId = (cid+1) % NUM_OF_CONTINENTS;
    player_t *myPlayers = continentPlayers[cid];


    //Epoch loop
    while(epoch < TOTAL_EPOCHS){

        //---- Execute Epoch ------
    
        //Player fighting
        conductPlayerBattle(myPlayers);

        //Feed the dragon
        if(dragonContinent == cid){
            feedDragon(myPlayers);
        }

        //Send dead players to global queue (revived)
        reapDeadPlayers(myPlayers);


        //Move (up to CONTINENT_QUEUE_LENGTH) alive players 
        //to the next continent's queue.
        j = 0;
        for(i = 0; i < PLAYERS_PER_CONTINENT && j < CONTINENT_QUEUE_LENGTH; i++){
            if(isAlivePlayer(myPlayers[i])){
                continentQueuedPlayers[nextContinentId][j++] = myPlayers[i];
                clearPlayer(&myPlayers[i]);
            }
        }


        //-------Thread Barrier----------------
        //Make sure all threads have finished the
        //current epoch before preparing for the
        //next epoch.
        pthread_mutex_lock(&barrierMutex);
            continentsDone++;
            if(continentsDone < NUM_OF_CONTINENTS){
                pthread_cond_wait(&barrierCondition, &barrierMutex);
            } else {
                moveDragon();
                epoch++;
                continentsDone = 0;
                pthread_cond_broadcast(&barrierCondition);
            }
        pthread_mutex_unlock(&barrierMutex);
        //-------------------------------------
	

        //---- Prepare for next epoch ---------
        
        compressContinentPlayerList(myPlayers);

        //Retrieve players from my queue.
        j = 0;
        for(i = 0; i < PLAYERS_PER_CONTINENT && j < CONTINENT_QUEUE_LENGTH; i++){
            if(isNullPlayer(myPlayers[i])){
                if(!isNullPlayer(continentQueuedPlayers[cid][j])){
                    myPlayers[i] = continentQueuedPlayers[cid][j];
                    clearPlayer(&continentQueuedPlayers[cid][j]);
                    j++;
                } 
                else break;
            }
        }

        //printf("Before refill\n"); printPlayersInContinent(cid);

        //Fill up players in continent with players from global player queue
        refillPlayersWithGlobalQueue(myPlayers);

        //printf("After refill\n"); printPlayersInContinent(cid);


        //-------Thread Barrier----------------
        //One thread prints the world when all the threads
        //have prepared to execute the next epoch. This is 
        //done using a barrier.
        pthread_mutex_lock(&barrierMutex);

            continentsDone++;
            if(continentsDone == NUM_OF_CONTINENTS){
                continentsDone = 0;
                pthread_cond_signal(&printWorldCond);
            }
            pthread_cond_wait(&barrierCondition, &barrierMutex);

        pthread_mutex_unlock(&barrierMutex);
        //-------------------------------------

    } //end epoch loop

    pthread_exit(0);
}


void initWorld()
{
    int i, j;
    FILE* configFile;
    char c;
    player_t temp;


    //Init players to default values.
    for(i = 0; i < MAX_PLAYERS; i++){
        clearPlayer(&globalPlayerQueue[i]);
    }


    //Open player data file.
    if((configFile = fopen(LAB2_CONFIG_FILE, "r")) == NULL)
        perror("Error opening config file");

    //Read players types (characters) from config file.
    while((c = getc(configFile)) != EOF) {
        if(c == 'A' || c == 'H'){
            if(totalPlayers > MAX_PLAYERS){
                fprintf(stderr, 
                        "Error! Number of players in %s exceeds max number of players! Exiting...\n", 
                        LAB2_CONFIG_FILE);
                exit(-1);
            }
            globalPlayerQueue[totalPlayers].id = totalPlayers;
            globalPlayerQueue[totalPlayers].type = c;
            globalPlayerQueue[totalPlayers].alive = 1;
            totalPlayers++;
        }
        else if (c == 'D' && (c = getc(configFile)) != EOF)
            dragonContinent = (c & 0x0F);
    }
    fclose(configFile);


    //Spread the inital queue across the continents
    for(i = 0; i < NUM_OF_CONTINENTS; i++){
        for(j = 0; j < PLAYERS_PER_CONTINENT; j++){
            continentPlayers[i][j] = getNextPlayerFromGlobalQueue();
        }
    }

    //Set globalQueuePutIndex
    for(i = globalQueueGetIndex; i < MAX_PLAYERS; i++){
        if(isNullPlayer(globalPlayerQueue[i])){
            break;
        }
    }
    globalQueuePutIndex = i % MAX_PLAYERS;

}

void printGlobalPlayerQueue(int printEmpties)
{
    int i, j;

    printf("\tPlayer Queue:\n");

    //We start printing at the index of the next fetch...
    for(i = globalQueueGetIndex; i < MAX_PLAYERS; i++){
        if(isNullPlayer(globalPlayerQueue[i])){
            if(printEmpties){
                //printf("\t\t(%d) ",i); 
                printf("\t\t");
                printf("*EMPTY*");
                printf("\n");
            }
        } else {
            //printf("\t\t(%d) ",i); 
            printf("\t\t");
            printPlayer(globalPlayerQueue[i]);
            printf("\n");
        }
    }

    //...and wrap to the beginning of the queue to print any others.
    for(i = 0; i < globalQueueGetIndex; i++){
        if(isNullPlayer(globalPlayerQueue[i])){
            if(printEmpties){
                //printf("\t\t(%d) ",i); 
                printf("\t\t");
                printf("*EMPTY*");
                printf("\n");
            }
        } else {
            //printf("\t\t(%d) ",i); 
            printf("\t\t");
            printPlayer(globalPlayerQueue[i]);
            printf("\n");
        }
    }
}

player_t getNextPlayerFromGlobalQueue()
{
    player_t p = globalPlayerQueue[globalQueueGetIndex];
    //printf("Getting player %d from index %d\n", p.id, globalQueueGetIndex);
    if(!isNullPlayer(p)){
        clearPlayer(&globalPlayerQueue[globalQueueGetIndex]);
        globalQueueGetIndex = (globalQueueGetIndex + 1) % MAX_PLAYERS;
    }
    return p;
}

void putPlayerOnGlobalQueue(player_t *p)
{
    //printf("Putting player %d at index %d\n", p->id, globalQueuePutIndex);
    globalPlayerQueue[globalQueuePutIndex] = *p;
    globalQueuePutIndex = (globalQueuePutIndex + 1) % MAX_PLAYERS;
    clearPlayer(p);
}

void moveDragon()
{
    dragonContinent += (NUM_OF_CONTINENTS-1);
    dragonContinent %= NUM_OF_CONTINENTS;
}

void conductPlayerBattle(player_t *continentPlayerList)
{
    int i = 0;
    int aPlayers = 0;
    int hPlayers = 0;
    char minorityPlayerType;
    player_t *myPlayers = continentPlayerList;

    //Determine minority player type
    aPlayers = 0;
    hPlayers = 0;
    for(i = 0; i < PLAYERS_PER_CONTINENT; i++){
        if(!isNullPlayer(myPlayers[i])){
            if(myPlayers[i].type == 'A')
                aPlayers++;
            else if(myPlayers[i].type == 'H')
                hPlayers++;
        }
    }
    minorityPlayerType = (aPlayers > hPlayers) ? 'H' : 'A';


    //Kill first minority type player
    for(i = 0; i < PLAYERS_PER_CONTINENT; i++){
        if(isAlivePlayer(myPlayers[i]) && myPlayers[i].type == minorityPlayerType){
            myPlayers[i].alive = 0;
            break;
        }
    }

}

void feedDragon(player_t *continentPlayerList)
{
    int i = 0;
    int dragonFull = 0;
    player_t *myPlayers = continentPlayerList;

    //If dragon present, dragon eats first A type player.
    for(i = 0; i < PLAYERS_PER_CONTINENT && !dragonFull; i++){
        if(isAlivePlayer(myPlayers[i]) && myPlayers[i].type == 'A'){
            myPlayers[i].alive = 0;
            dragonFull = 1;
        }
    }

    //If no A player found, eat first H player.
    if(i == PLAYERS_PER_CONTINENT){
        for(i = 0; i < PLAYERS_PER_CONTINENT && !dragonFull; i++){
            if(isAlivePlayer(myPlayers[i]) && myPlayers[i].type == 'H'){
                myPlayers[i].alive = 0;
                dragonFull = 1;
            }
        }
    }

}

void reapDeadPlayers(player_t *myPlayers)
{
    int i;

    //Send dead players to global queue (revived)
    for(i = 0; i < PLAYERS_PER_CONTINENT; i++){
        if(!isNullPlayer(myPlayers[i]) && !isAlivePlayer(myPlayers[i])){
            pthread_mutex_lock(&globalQueueMutex);
                myPlayers[i].alive = 1;
                putPlayerOnGlobalQueue(&myPlayers[i]);
            pthread_mutex_unlock(&globalQueueMutex);
        }
    }
}

void compressContinentPlayerList(player_t *myPlayers)
{
    int i, j;

    //Compress continent player list.
    for(i = 0; i < PLAYERS_PER_CONTINENT; i++){
        if(isNullPlayer(myPlayers[i])){
            //Find next alive player and move up.
            for(j = i; j < PLAYERS_PER_CONTINENT; j++){
                if(!isNullPlayer(myPlayers[j])){
                    myPlayers[i] = myPlayers[j];
                    clearPlayer(&myPlayers[j]);
                    break;
                }
            }
        }
    }
}

void refillPlayersWithGlobalQueue(player_t *myPlayers)
{
    int i, lastNullPlayerIndex;

    //Find last index with null player
    for(i = 0; i < PLAYERS_PER_CONTINENT; i++){
        if(isNullPlayer(myPlayers[i]))
            break;
    }
    lastNullPlayerIndex = i;


    //printf("cid:%d, lastNullPlayerIndex=%d\n", cid, lastNullPlayerIndex);
    //Fill up players in continent with players from global player queue
    if(lastNullPlayerIndex < PLAYERS_PER_CONTINENT){
        pthread_mutex_lock(&globalQueueMutex);
            for(i = lastNullPlayerIndex; i < PLAYERS_PER_CONTINENT; i++){
                myPlayers[i] = getNextPlayerFromGlobalQueue();
            }
        pthread_mutex_unlock(&globalQueueMutex);
    }

}

int clearPlayer(player_t *player)
{
    player->id = -1;
    player->alive = 0;
}

int isNullPlayer(player_t player)
{
    return (player.id == -1);
}

int isAlivePlayer(player_t player)
{
    return (!isNullPlayer(player) && player.alive == 1);
}

void printWorld()
{
    int i;
    for(i = 0; i < NUM_OF_CONTINENTS; i++){
        printPlayersInContinent(i);
    }
    printGlobalPlayerQueue(0);
    printf("\n");
    //printf("Dragon continent : %d\n", dragonContinent);
}


void printPlayersInContinent(int continentId)
{
    int i;
    int cid = continentId;
    player_t p;

    printf("\tPlayers in continent %d\n", cid);
    for(i = 0; i < PLAYERS_PER_CONTINENT; i++){
        p = continentPlayers[cid][i];
        if(!isNullPlayer(p)){
            //printf("\t\t(%d) ",i); 
            printf("\t\t");
            printPlayer(p); 
            printf("\n");
        }
    }
    if(dragonContinent == continentId)
        printf("\t\t*Dragon!\n");
}

void printPlayersInContinentQueue(int continentId)
{
    int i;
    int cid = continentId;
    player_t p;

    printf("\tPlayers in continent %d's queue\n", cid);
    for(i = 0; i < CONTINENT_QUEUE_LENGTH; i++){
        p = continentQueuedPlayers[cid][i];
        if(!isNullPlayer(p)){
            //printf("\t\t(%d) ",i); 
            printf("\t\t");
            printPlayer(p); 
            printf("\n");
        }
    }
}

void printPlayer (player_t player)
{
    printf("Player %d : %c", player.id, player.type);
}
