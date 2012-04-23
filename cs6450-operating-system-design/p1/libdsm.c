//Written by David Ells (and ONLY David Ells :)
//
// A simple library for implementing shared memory across
// processors using MPI and Pthreads.

#include "dsm.h"

int libdsm_rank = -1;
int libdsm_nprocs = -1;
int libdsm_pgsz = -1;
int libdsm_reg_index = 0;
libdsm_memblock* libdsm_registered_mem_blocks[MAXIMUM_SHARED_PAGES];
int libdsm_current_requested_block_index = -1;
int libdsm_num_in_barrier = 0;

pthread_t libdsm_comm_thread;
pthread_t** mutex_holder_thread;

int libdsm_mutex_ints[16];
pthread_mutex_t libdsm_mutexes[16];
pthread_cond_t libdsm_mutexes_cond[16];

MPI_Comm libdsm_comm1, libdsm_comm2;

void segv_handler(int signum, siginfo_t *si, void *ctx)
{
    int i;
    char pg[libdsm_pgsz];
    libdsm_memblock* mb;
    MPI_Status status;
    int valid_addr = 0;
    void* segv_addr = si->si_addr;

    debug("%d: HANDLING SEGV for address %p\n", dsm_rank(), segv_addr);

    for(i = 0; i < libdsm_reg_index; i++){
        mb = libdsm_registered_mem_blocks[i];
        if((segv_addr >= mb->memptr) && (segv_addr < (mb->memptr + libdsm_pgsz))){
            //Address in a protected block
            valid_addr = 1;
            mb = libdsm_registered_mem_blocks[i];

            debug("%d: Address %p has following permissions: ", dsm_rank(), segv_addr);
            if(mb->protection & PROT_READ) debug("PROT_READ ");
            if(mb->protection & PROT_WRITE) debug("PROT_WRITE ");
            debug("\n");

            if(mb->protection & PROT_READ){
                //Must be a write access. Set dirty bit and allow.
                debug("%d: Address %p set to dirty\n", dsm_rank(), segv_addr);
                mb->dirty = 1;
                if (mprotect(mb->memptr, libdsm_pgsz, (PROT_READ | PROT_WRITE)) < 0)
                    err_die("mprotect failed in handler");
                mb->protection = (PROT_READ | PROT_WRITE);

            } else {
                //Must be a first access, get master copy from rank 0.
                if (mprotect(mb->memptr, libdsm_pgsz, (PROT_READ | PROT_WRITE)) < 0)
	                err_die("mprotect failed in handler");
                mb->protection = (PROT_READ | PROT_WRITE);

                //Only get from rank 0 if not rank 0.
                if(dsm_rank() != 0){
                    debug("%d: Sending request to rank 0 for block %d\n", dsm_rank(), i);
                    //Send request to rank 0 for this block
                    MPI_Send(&i, 1, MPI_INT, 0, 1, libdsm_comm2);
                    MPI_Recv(pg, libdsm_pgsz, MPI_CHAR, 0, 1, libdsm_comm1, &status);
                    memcpy(mb->memptr, pg, libdsm_pgsz);
                    debug("%d: Recieved memory for block %d\n", dsm_rank(), i);
                }

                if (mprotect(mb->memptr, libdsm_pgsz, PROT_READ) < 0)
	                err_die("mprotect failed in handler");
                mb->protection = PROT_READ;
            }
        }
    }

    if(!valid_addr){
        err_die("Unhandled SIGSEGV");
    }

}

int dsm_init()
{
    int i, provided;
    struct sigaction sa;
    MPI_Group libdsm_group;

    MPI_Init_thread(0,0,MPI_THREAD_MULTIPLE,&provided);

    //Get rank and size of MPI ring.
    MPI_Comm_rank(MPI_COMM_WORLD, &libdsm_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &libdsm_nprocs);

    //Create communicators for main thread and service thread
    MPI_Comm_group(MPI_COMM_WORLD, &libdsm_group);
    MPI_Comm_create(MPI_COMM_WORLD, libdsm_group, &libdsm_comm1);
    MPI_Comm_dup(libdsm_comm1, &libdsm_comm2);

    //Get page size.
    libdsm_pgsz = getpagesize();

    //Set up signal handler
    sa.sa_sigaction = segv_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGSEGV);
    sigaction(SIGSEGV,&sa,NULL);

    debug("%d: Init completed.\n", dsm_rank());

    if(dsm_rank() == 0){
        //Create communication thread
        pthread_create(&libdsm_comm_thread, NULL, libdsm_rank0_comm_loop, NULL);
        //Initialize mutexes
        for(i = 0; i < 16; i++){
            pthread_mutex_init(&libdsm_mutexes[i], NULL);
            pthread_cond_init(&libdsm_mutexes_cond[i], NULL);
        }

        mutex_holder_thread = (pthread_t**)malloc(sizeof(pthread_t*) * dsm_nprocs());
        for(i = 0; i < dsm_nprocs(); i++){
            mutex_holder_thread[i] = (pthread_t*)malloc(sizeof(pthread_t) * 16);
        }
    } 

    return 0;
}

int dsm_rank()
{
    return libdsm_rank;
}

int dsm_nprocs()
{
    return libdsm_nprocs;
}

void* dsm_malloc()
{
    //void* newmem = malloc(libdsm_pgsz);
    void* memptr = mmap(0,libdsm_pgsz,PROT_READ | PROT_WRITE,MAP_ANONYMOUS | MAP_PRIVATE,-1,0);

    //Create and register new memory block
    libdsm_memblock* mb = libdsm_createSharedBlock(memptr, libdsm_pgsz, 0, PROT_NONE);
    if(libdsm_registerSharedBlock(mb) < 0){
        fprintf(stderr, "Register of shared block failed in dsm_malloc.\n");
        exit(-1);
    }

    return memptr;
}

int dsm_finalize()
{
    int i = 0;

    dsm_barrier();
    //If Rank 0, cut off wait by sending dummy message.
    if(dsm_rank() == 0){
        MPI_Send(&i, 1, MPI_INT, 0, 99, libdsm_comm2);
    }

    debug("%d: dsm_finalize called\n", dsm_rank());
    pthread_join(libdsm_comm_thread, NULL);
    MPI_Finalize();
    return 1;
}


void* libdsm_rank0_comm_loop(void* ignore)
{
    //Rank 0's communication thread that continually services requests
    //from the other ranks. An MPI tag of 1 means a request for a page,
    //a tag of 2 means an update for a page, a tag of 3 means a request
    //for a mutex, and a tag of 4 means a return of a mutex.
    //Tag 99 ends communication loop.

    int i, source, tag;
    int* mutex_hold_args;
    char pg[libdsm_pgsz];
    libdsm_memblock* mb;
    MPI_Status status, temp;

    debug("0: Comm thread: Starting communication loop...\n");
    while(1){

            debug("0: Comm thread: Waiting for a message\n");
            //Recieve message
            MPI_Recv(&i, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, libdsm_comm2, &status);

            source = status.MPI_SOURCE;
            tag = status.MPI_TAG;

            debug("0: Comm thread: Recieved message from rank %d with tag %d and index %d\n", source, tag, i);

            if(tag == 1){
                mb = libdsm_registered_mem_blocks[i];
                memcpy(pg, mb->memptr, libdsm_pgsz);
                MPI_Send(pg, libdsm_pgsz, MPI_CHAR, source, tag, libdsm_comm1);
                debug("0: Comm thread: Sent back memory block %d to rank %d\n", i, source);
            }
            else if (tag == 2) {
                mb = libdsm_registered_mem_blocks[i];
                MPI_Recv(pg, libdsm_pgsz, MPI_CHAR, source, tag, libdsm_comm2, &temp);
                memcpy(mb->memptr, pg, libdsm_pgsz);
                debug("0: Comm thread: Recieved updated block with index %d from rank %d\n", i, source);
            } 
            else if (tag == 3) {
                pthread_mutex_lock(&libdsm_mutexes[i]);
                MPI_Send(&libdsm_mutex_ints[i], 1, MPI_INT, source, 3, libdsm_comm1);
                if(libdsm_mutex_ints[i] != 1){
                    libdsm_mutex_ints[i] = 1;
                    pthread_mutex_unlock(&libdsm_mutexes[i]);
                } else {
                    pthread_mutex_unlock(&libdsm_mutexes[i]);
                    mutex_hold_args = (int*)malloc(sizeof(int)*2);
                    mutex_hold_args[0] = source;
                    mutex_hold_args[1] = i;
                    pthread_create(&mutex_holder_thread[source][i], NULL, 
                                    libdsm_master_rank_mutex_hold, (void*)mutex_hold_args);
                    debug("%d: Created mutex holding thread\n", dsm_rank());
                }
            }
            else if (tag == 4) {
                pthread_mutex_lock(&libdsm_mutexes[i]);
                libdsm_mutex_ints[i] = 0;

                debug("%d: Comm thread: Mutex %d unlocked by rank %d\n", dsm_rank(), i, source);
                pthread_cond_signal(&libdsm_mutexes_cond[i]);
                pthread_mutex_unlock(&libdsm_mutexes[i]);
            }
            else if(tag == 99){
                break;
            }

    }
    pthread_exit(0);
}

void* libdsm_master_rank_mutex_hold(void* arg)
{
    int* int_args = (int*)arg;
    int i = 0;
    int requesting_rank = int_args[0];
    int mutex_number = int_args[1];

    free(arg);

    pthread_mutex_lock(&libdsm_mutexes[mutex_number]);
    debug("%d: Mutex hold thread: Suspending on condition %d: requesting rank %d\n", 
                   dsm_rank(), mutex_number, requesting_rank);
    pthread_cond_wait(&libdsm_mutexes_cond[mutex_number], &libdsm_mutexes[mutex_number]);
    MPI_Send(&i, 1, MPI_INT, requesting_rank, 3, libdsm_comm1);
    libdsm_mutex_ints[mutex_number] = 1;
    pthread_mutex_unlock(&libdsm_mutexes[mutex_number]);

    pthread_exit(0);
}

libdsm_memblock* libdsm_createSharedBlock(void* memptr, size_t size, int dirty, int protect)
{
    libdsm_memblock* mb = (libdsm_memblock*)malloc(sizeof(libdsm_memblock));
    mb->memptr = memptr;
    mb->size = libdsm_pgsz;
    mb->dirty = 0;
    mb->protection = (PROT_NONE);

    if (mprotect(mb->memptr, libdsm_pgsz, mb->protection) < 0)
        err_die("mprotect failed in libdsm_createShareBlock");

    return mb;
}

int libdsm_registerSharedBlock(libdsm_memblock* mb)
{
    if(libdsm_reg_index > MAXIMUM_SHARED_PAGES) 
        return -1;
    libdsm_registered_mem_blocks[libdsm_reg_index++] = mb;
    return 0;
}


int dsm_sync()
{
    int i;
    char pg[libdsm_pgsz];
    libdsm_memblock* mb;

    //If not rank 0, send all dirty registered pages to 0.
    if(dsm_rank() != 0){

        for(i = 0; i < libdsm_reg_index; i++){
            mb = libdsm_registered_mem_blocks[i];

            if(mb->dirty){
                //Send block number first, then send memory contents.
                MPI_Send(&i, 1, MPI_INT, 0, 2, libdsm_comm2);
                memcpy(pg, mb->memptr, libdsm_pgsz);
                MPI_Send(pg, libdsm_pgsz, MPI_CHAR, 0, 2, libdsm_comm2);

                debug("%d: Sent update for block %d\n", dsm_rank(), i);

                //Page no longer "dirty"
                mb->dirty = 0;
            }

            //Pages may be updated, so we invalidate ours
            if(mprotect(mb->memptr, libdsm_pgsz, PROT_NONE) < 0)
                err_die("mprotect failed in dsm_sync");
            mb->protection = PROT_NONE;
        }
    }
    return 0;
}

int dsm_barrier()
{
    debug("%d: Joining barrier...\n", dsm_rank());
    return MPI_Barrier(libdsm_comm1);
}

int dsm_mutex_lock(int mutex_number)
{
    int i = mutex_number;
    MPI_Status status;

    debug("%d: Attempting to lock mutex %d\n", dsm_rank(), mutex_number);

    if(dsm_rank() == 0){
        pthread_mutex_lock(&libdsm_mutexes[mutex_number]);
        libdsm_mutex_ints[mutex_number] = 1;
    } else {
        MPI_Send(&i, 1, MPI_INT, 0, 3, libdsm_comm2);
        MPI_Recv(&i, 1, MPI_INT, 0, 3, libdsm_comm1, &status);
        debug("%d: Recieved %d about mutex %d when requesting\n", dsm_rank(), i, mutex_number);
        if(i == 1) {
            MPI_Recv(&i, 1, MPI_INT, 0, 3, libdsm_comm1, &status);
        }
    }

    debug("%d: Obtained lock on mutex %d\n", dsm_rank(), mutex_number);
    return 0;
}

int dsm_mutex_unlock(int mutex_number)
{
    if(dsm_rank() == 0){
        libdsm_mutex_ints[mutex_number] = 0;
        pthread_mutex_unlock(&libdsm_mutexes[mutex_number]);
        pthread_cond_signal(&libdsm_mutexes_cond[mutex_number]);
        debug("%d: Mutex %d unlocked \n", dsm_rank(), mutex_number);
    }  else {
        debug("%d: Sending message to release mutex %d\n", dsm_rank(), mutex_number);
        MPI_Send(&mutex_number, 1, MPI_INT, 0, 4, libdsm_comm2);
    }
    return 0;
}
            
void err_die(const char* message)
{
    perror(message);
    exit(-1);
}

void debug(const char* message, ...)
{
    #if DEBUG == 1
        va_list printf_args;
        va_start(printf_args, message);
        vfprintf(stderr, message, printf_args);
        va_end(printf_args);
    #endif
}

