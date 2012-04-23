//Written by David Ells (and ONLY David Ells :)
//
// A simple library for implementing an rpc mechanism
// across multiple processors using MPI and Pthreads.

#include <dlfcn.h>
#include <errno.h>
#include <mpi.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "osdrpc.h"

#define DEBUG 0

typedef int (*osdlib_function)(void *);

int osdrpc_process_data(void *);
void* osdrpc_service_loop(void *);
int osdrpc_barrier();
void debug(const char*, ...);

int osdrpc_rank;
int osdrpc_nprocs;

pthread_t osdrpc_service_thread;
MPI_Comm osdrpc_comm1, osdrpc_comm2;

int osdrpc_init(void)
{
    int provided;
    MPI_Group osdrpc_group;

    //Use init for threaded programs
    MPI_Init_thread(0, 0, MPI_THREAD_MULTIPLE, &provided);

    //Get rank and size of MPI ring.
    MPI_Comm_rank(MPI_COMM_WORLD, &osdrpc_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &osdrpc_nprocs);

    //Create communicators for main thread and service thread
    MPI_Comm_group(MPI_COMM_WORLD, &osdrpc_group);
    MPI_Comm_create(MPI_COMM_WORLD, osdrpc_group, &osdrpc_comm1);
    MPI_Comm_dup(osdrpc_comm1, &osdrpc_comm2);

    //Create service thread
    pthread_create(&osdrpc_service_thread, NULL, osdrpc_service_loop, NULL);

    return 0;
}

int osdrpc_finalize(void)
{
    int i = 0;

    debug("%d: osdrpc_finalize called\n", osdrpc_rank);

    //Make sure that service thread is not terminated prematurely 
    //by blocking main thread here with a barrier call.
    osdrpc_barrier();

    debug("%d: sending terminating message to service thread\n", osdrpc_rank);
    MPI_Send(&i, 1, MPI_INT, osdrpc_rank, 99, osdrpc_comm2);
    pthread_join(osdrpc_service_thread, NULL);

    return MPI_Finalize();
}

int osdrpc_call(int rank, char *lib, char *procname, char *fmt, ...)
{
    int i, tempint;
    int numargs, remote_rc;
    int libname_size, procname_size, tempstr_size, total_data_size;
    char *tempstr;
    void *p, *packed_data_ptr;
    va_list ap;


    libname_size = strlen(lib)+1;
    procname_size = strlen(procname)+1;
    numargs = strlen(fmt);

    total_data_size = 0;
    total_data_size += libname_size;
    total_data_size += procname_size;
    total_data_size += numargs+1;

    //Pass through variable argument list, calculating size for data to send
    va_start(ap, fmt);
    for(i = 0; i < numargs; i++){
        switch(fmt[i]){
            case 'i': 
                tempint = va_arg(ap, int);
                total_data_size += sizeof(int);
                break;
            case 's':
                tempstr = va_arg(ap, char*);
                total_data_size += strlen(tempstr)+1;
                break;
        }
    }
    va_end(ap);

    //Allocated space for packed up data
    packed_data_ptr = malloc(total_data_size);
    if(packed_data_ptr == NULL)
        return -1;
    p = packed_data_ptr;

    //Start copying data into allocated space
    memcpy(p, lib, libname_size);
    p += libname_size;
    memcpy(p, procname, procname_size);
    p += procname_size;
    memcpy(p, fmt, numargs+1);
    p += (numargs+1);

    //Run through variable list, copying arguments
    va_start(ap, fmt);
    for(i = 0; i < numargs; i++){
        switch(fmt[i]){
            case 'i': 
                tempint = va_arg(ap, int);
                memcpy(p, &tempint, sizeof(int));
                p += sizeof(int);
                break;
            case 's':
                tempstr = va_arg(ap, char*);
                tempstr_size = strlen(tempstr)+1;
                memcpy(p, tempstr, tempstr_size);
                p += tempstr_size;
                break;
        }
    }
    va_end(ap);


    //Send data to remote process on the service communicator
    debug("%d: sending data size to process %d\n", osdrpc_rank, rank);
    MPI_Send(&total_data_size, 1, MPI_INT, rank, 1, osdrpc_comm2);
    debug("%d: sending data to process %d\n", osdrpc_rank, rank);
    MPI_Send((char*)packed_data_ptr, total_data_size, MPI_CHAR, rank, 1, osdrpc_comm2);

    //Recieve return code from remote process on the main communicator
    debug("%d: *** waiting for return code from process %d\n", osdrpc_rank, rank);
    MPI_Recv(&remote_rc, 1, MPI_INT, rank, 1, osdrpc_comm1, MPI_STATUS_IGNORE);
    debug("%d: returning return code to client\n", osdrpc_rank);

    free(packed_data_ptr);
    return remote_rc;
}

int osdrpc_process_data(void *rpc_data)
{
    void *p, *library;
    int i, rc, numargs;
    char *libname, *procname, *fmt, *error;
    void **args;
    osdlib_function osdlibfunc;

    p = rpc_data;

    //Extract libname, procname, and fmt strings
    libname = (char*)p;
    p += strlen(p)+1;
    procname = (char*)p;
    p += strlen(p)+1;
    fmt = (char*)p;
    p += strlen(p)+1;

    numargs = strlen(fmt);
    args = (void**)malloc(sizeof(void*) * numargs);

    //Parse out variable list of args
    for(i = 0; i < numargs; i++){
        args[i] = p;
        switch(fmt[i]){
            case 'i': 
                p += sizeof(int);
                break;
            case 's':
                p += strlen(p)+1;
                break;
        }
    }

    //Debug args recieved
    /*debug("Processed args:\n");
    debug("\tlibname=%s\n", libname);
    debug("\tprocname=%s\n", procname);
    debug("\tfmt=%s\n", fmt);
    for(i = 0; i < numargs; i++){
        debug("\targ[%d]=", i);
        switch(fmt[i]){
            case 'i': debug("%d\n", *((int*)args[i])); break;
            case 's': debug("%s\n", (char*)args[i]); break;
        }
    }*/

    if(strncmp(libname,"libosd1.so",strlen(libname)) != 0 && 
       strncmp(libname,"libosd2.so",strlen(libname)) != 0){
        fprintf(stderr, "osdrpc_call must call function from libosd libraries!\n");
        errno = EINVAL;
        return -1;
    }

     library = dlopen(libname, RTLD_LAZY);
     if (library == NULL) {
          fprintf(stderr, "Could not open library %s: %s\n", libname, dlerror());
          errno = EINVAL;
          return -1;
     }
  
     dlerror();    /* clear errors */
     osdlibfunc = dlsym(library, procname);
     error = dlerror();
     if (error) {
          fprintf(stderr, "Could not find function %s: %s\n", procname, error);
          errno = EINVAL;
          return -1;
     }
  
     if(procname == "p4" && numargs < 3){
         errno = EINVAL;
         return -1;
     }

     rc = (*osdlibfunc)(args);
     if(rc < 0){
         errno = 0;
         return -1;
     }

     dlclose(library);

     return rc;
}

void* osdrpc_service_loop(void *ignore)
{
    /* The service loop which accepts requests for rpc calls, recieves data,
     * makes call, and sends back result. Tag 1 is used for requesting, tag 99
     * ends the service loop and terminates the thread. */

    int i;
    int source, tag, rc;
    void* databuf;
    MPI_Status status;

    debug("%d: service thread: Starting service loop\n", osdrpc_rank);

    while(1){
        debug("%d: service thread: Waiting to recieve another message\n", osdrpc_rank);
        MPI_Recv(&i, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, osdrpc_comm2, &status);

        source = status.MPI_SOURCE;
        tag = status.MPI_TAG;

        debug("%d: service thread: Recieved i = %d on tag %d from process %d\n", osdrpc_rank, i, tag, source);

        if(tag == 1){
            databuf = malloc(i);

            debug("%d: service thread: Recieving data from process %d\n", osdrpc_rank, source);
            MPI_Recv((char*)databuf, i, MPI_CHAR, source, 1, osdrpc_comm2, &status);
            rc = osdrpc_process_data(databuf);
            debug("%d: service thread: Sending return code %d to process %d\n", osdrpc_rank, rc, source);

            MPI_Send(&rc, 1, MPI_INT, source, 1, osdrpc_comm1);
            free(databuf);
        }

        else if(tag == 99){
            break;
        }
    }
    debug("%d: service thread: Terminating service thread\n", osdrpc_rank);
    pthread_exit(0);
}

int osdrpc_barrier()
{
    debug("%d: Joining barrier...\n", osdrpc_rank);
    return MPI_Barrier(osdrpc_comm1);
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

