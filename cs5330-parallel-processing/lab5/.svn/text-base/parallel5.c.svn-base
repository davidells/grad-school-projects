#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "/usr/local/home/accts/pettey/mpich2-1.0.5p3/mpich2i/include/mpi.h"

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
 * This code is an MPI version. The original used Pthreads.
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

int totalPlayers = 0;
int globalQueueGetIndex = 0;
int globalQueuePutIndex = 0;
int epoch = 0;
int dragonContinent = 0;

//Function prototypes
void initWorld();
void printWorld();
void printPlayersInContinent(int continentId);

void printGlobalPlayerQueue(int);
player_t getNextPlayerFromGlobalQueue();
void putPlayerOnGlobalQueue(player_t*);

void printPlayer(player_t);
void printPlayers(player_t* players, int n);
int clearPlayer(player_t*);
int isAlivePlayer(player_t);
int isNullPlayer(player_t);

void moveDragon();
void conductPlayerBattle(player_t*);
void feedDragon(player_t*);
void compressContinentPlayerList(player_t*);
void refillPlayersWithGlobalQueue(player_t*);

void startContinent();


int rank, nprocs, myCid;
MPI_Status status;

int main(int argc, char** argv)
{
    int i, j;
    int msg_rank = 0;
    player_t deadTemp[CONTINENT_QUEUE_LENGTH];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    myCid = rank-1;

    if(rank == 0){
        //Initialize the game world.
        initWorld();

        //Print initial world set up.
        printf("Initial world :\n");
        printWorld();


        for(i = 0; i < NUM_OF_CONTINENTS; i++){

            //Send continent i player list to rank i+1
            MPI_Send((char*)continentPlayers[i], 
                      sizeof(player_t) * PLAYERS_PER_CONTINENT,
                      MPI_CHAR, i+1, 0, MPI_COMM_WORLD);

            //Send intial dragon position
            MPI_Send(&dragonContinent, 1, MPI_INT, i+1, 0, MPI_COMM_WORLD);
        }


        for(epoch = 0; epoch < TOTAL_EPOCHS; epoch++){

            for(i = 0; i < NUM_OF_CONTINENTS; i++){
                //Recieve dead
                MPI_Recv((char*)deadTemp,
                          sizeof(player_t) * CONTINENT_QUEUE_LENGTH,
                          MPI_CHAR, i+1, 0, MPI_COMM_WORLD, &status);

                //Put dead on global queue
                for(j = 0; j < CONTINENT_QUEUE_LENGTH; j++){
                    if(!isNullPlayer(deadTemp[j])){
                        putPlayerOnGlobalQueue(&deadTemp[j]);
                    }
                }
            }


            for(i = 0; i < NUM_OF_CONTINENTS; i++){
                //Recieve new player list after epoch from each continent
                MPI_Recv((char*)continentPlayers[i],
                          sizeof(player_t) * PLAYERS_PER_CONTINENT,
                          MPI_CHAR, i+1, 0, MPI_COMM_WORLD, &status);

                /*printf("Rank 0 got these players from continent %d\n", i);
                printPlayersInContinent(i);*/

                //Refill with players from global queue.
                refillPlayersWithGlobalQueue(continentPlayers[i]);

                //Send new list to each continent to start new epoch.
                MPI_Send((char*)continentPlayers[i], 
                          sizeof(player_t) * PLAYERS_PER_CONTINENT,
                          MPI_CHAR, i+1, 0, MPI_COMM_WORLD);
            }

            moveDragon();

            //Print new world that is now executing in the other ranks.
            printf("The World After %d Epochs\n", epoch+1);
            printWorld();


       }
    } else {
        //Recieve initial distrobution from rank 0.
        MPI_Recv((char*)continentPlayers[myCid], 
                  sizeof(player_t) * PLAYERS_PER_CONTINENT,
                  MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);

        //Recieve initial dragon position.
        MPI_Recv(&dragonContinent, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        startContinent(myCid);
    }

    MPI_Finalize();
    return 0;
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

void startContinent(int continentId)
{
    int i, j;
    int cid = continentId;
    int nextContinentId = (cid+1) % NUM_OF_CONTINENTS;
    int prevContinentId = (cid+NUM_OF_CONTINENTS-1) % NUM_OF_CONTINENTS;
    player_t myPlayers[PLAYERS_PER_CONTINENT];
    player_t myDead[CONTINENT_QUEUE_LENGTH];
    player_t myQueue[CONTINENT_QUEUE_LENGTH];
    player_t nextQueue[CONTINENT_QUEUE_LENGTH];


    //Initial set up
    memcpy(myPlayers, continentPlayers[cid], sizeof(player_t) * PLAYERS_PER_CONTINENT);

    //Epoch loop
    for(epoch = 0; epoch < TOTAL_EPOCHS; epoch++){

        //---- Execute Epoch ------
        

        //Player fighting
        conductPlayerBattle(myPlayers);

        //Feed the dragon
        if(dragonContinent == cid){
            feedDragon(myPlayers);
        }

        /*printf("%d: Printing players...\n", rank);
         *printPlayers(myPlayers, PLAYERS_PER_CONTINENT);
        }*/

        //Clear dead array
        for(i = 0; i < CONTINENT_QUEUE_LENGTH; i++){
            clearPlayer(&myDead[i]);
        }

        
        //Move dead players to dead array (revived)
        j = 0;
        for(i = 0; i < PLAYERS_PER_CONTINENT && j < CONTINENT_QUEUE_LENGTH; i++){
            if(!isNullPlayer(myPlayers[i]) && !isAlivePlayer(myPlayers[i])){
                    //printf("%d: Player %d is dead\n", rank, myPlayers[i].id);
                    myPlayers[i].alive = 1;
                    myDead[j++] = myPlayers[i];
                    clearPlayer(&myPlayers[i]);
            }
        }

        /*printf("%d: Sending the following dead to rank 0...\n", rank);
          printPlayers(myDead, CONTINENT_QUEUE_LENGTH);*/

        //Send dead back to rank 0 to be placed on global queue
        MPI_Send((char*)myDead, sizeof(player_t) * CONTINENT_QUEUE_LENGTH,
                  MPI_CHAR, 0, 0, MPI_COMM_WORLD);


        //Move (up to CONTINENT_QUEUE_LENGTH) alive players to nextQueue
        j = 0;
        for(i = 0; i < PLAYERS_PER_CONTINENT && j < CONTINENT_QUEUE_LENGTH; i++){
            if(isAlivePlayer(myPlayers[i])){
                nextQueue[j++] = myPlayers[i];
                clearPlayer(&myPlayers[i]);
            }
        }

        //Send nextQueue to the next continent's process.
        MPI_Send((char*)nextQueue, 
                  sizeof(player_t) * CONTINENT_QUEUE_LENGTH,
                  MPI_CHAR, nextContinentId+1, 0, MPI_COMM_WORLD);

        //Recieve new queue for next epoch.
        MPI_Recv((char*)myQueue, 
                  sizeof(player_t) * CONTINENT_QUEUE_LENGTH,
                  MPI_CHAR, prevContinentId+1, 0, MPI_COMM_WORLD, &status);



        //---- Prepare for next epoch ---------
        compressContinentPlayerList(myPlayers);

        //Retrieve players from my queue.
        j = 0;
        for(i = 0; i < PLAYERS_PER_CONTINENT && j < CONTINENT_QUEUE_LENGTH; i++){
            if(isNullPlayer(myPlayers[i])){
                if(!isNullPlayer(myQueue[j])){
                    myPlayers[i] = myQueue[j];
                    clearPlayer(&myQueue[j]);
                    j++;
                } 
                else break;
            }
        }

        //printf("Before refill\n"); printPlayersInContinent(cid);
        
        //Send player list off to be filled from global queue.
        MPI_Send((char*)myPlayers,
                  sizeof(player_t) * PLAYERS_PER_CONTINENT,
                  MPI_CHAR, 0, 0, MPI_COMM_WORLD);

        //Rank 0 fills up players in continent with players from global player queue

        //Recieve new filled up queue for the next epoch.
        MPI_Recv((char*)myPlayers,
                  sizeof(player_t) * PLAYERS_PER_CONTINENT,
                  MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);

        //printf("After refill\n"); printPlayersInContinent(cid);

        moveDragon();

    } //end epoch loop

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
            //printf("%d: Killing player %d in battle!\n", rank, myPlayers[i].id);
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
        for(i = lastNullPlayerIndex; i < PLAYERS_PER_CONTINENT; i++){
            myPlayers[i] = getNextPlayerFromGlobalQueue();
        }
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

void printPlayers(player_t* players, int n)
{
    int i;
    player_t p;

    for(i = 0; i < n; i++){
        p = players[i];
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
