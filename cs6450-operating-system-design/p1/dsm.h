//Written by David Ells (and ONLY David Ells :)
//
// A simple library for implementing shared memory across
// processors using MPI and Pthreads.

#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "mpi.h"

#define MAXIMUM_SHARED_PAGES 100
#define DEBUG 0

typedef struct {
    void* memptr;
    size_t size;
    int dirty;
    int protection;
} libdsm_memblock;


int dsm_init();
int dsm_rank();
int dsm_nprocs();
void* dsm_malloc();
int dsm_finalize();
void* libdsm_rank0_comm_loop(void*);
void* libdsm_master_rank_mutex_hold(void*);
int libdsm_registerSharedBlock(libdsm_memblock*);
libdsm_memblock* libdsm_createSharedBlock(void* memptr, size_t size, int dirty, int protect);
int dsm_sync();
int dsm_barrier();
int dsm_mutex_lock(int mutex_number);
int dsm_mutex_unlock(int mutex_number);
void err_die(const char*);
void debug(const char*, ...);
