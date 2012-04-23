/* Written by David Ells
 *
 * A program to demonstrate the parallel INDEX search of a binary tree.
 * Run with -h flag to see usage and help. */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define PROGNAME "index-search"
#define INDEX_THREAD_MAX 128 
#define INDEX_DEBUG_THREADS 0
#define INDEX_DEBUG_TIME 0
#define INDEX_DEBUG_PROGRESS 0

typedef struct {
    int id;
    int *array;
    int start;
    int end;
} index_thread_args;

//Global variables
int search_val, val_found;
int INDEX_ARRAY_SIZE;

pthread_t *threads;
pthread_mutex_t val_found_mutex = PTHREAD_MUTEX_INITIALIZER;

//Function prototypes
int search_array_for_val(int *array, int array_size, int num_threads, int val);
void *thread_search_array(void *args);
int randint(int);

//Some function declarations
void printArgError()
{
    printf(PROGNAME ": error: wrong number of arguments\n");
}

void printUsage()
{
    printf("\t" PROGNAME " [-h] [filename] [searchvalue] [number of threads]\n");
}

void printHelp()
{
    printUsage();
    printf("\n\t" PROGNAME "  will process the file named, reading consecutive\n"
           "\tintegers from the file specified into an internal array. It will\n"
           "\tcreate the number of concurrent threads specified to\n"
           "\tperform a simple sequential search of the array in parallel.\n"
           "\tNote that just one processor may also be specified.\n");
    printf("\tOptions:\n"
           "\t\t-h : show this help\n");
}

int main(int argc, char **argv)
{
    int i, next_int;
    int num_threads;
    int *int_arr;
    FILE* f;
    char *fname;
    int arr_space = 1024;
    int keyword_start_index = 1;

    //Check args for -h flag, print help and exit if found.
    for(i = 1; i < argc; i++){
        if( strcmp(argv[i], "-h") == 0){
            printf("\n");
            printHelp(); 
            exit(0);
        }
    }

    //Debug args
    /*printf("keyword index = %d\n", keyword_start_index);
    printf("argc - keyword index = %d\n", argc - keyword_start_index);
    for(i = 0; i < argc; i++){
        printf("argv[%d] = %s\n", i, argv[i]);
    }*/

    if((argc - keyword_start_index) != 3){
        printArgError();
        printUsage();
        exit(1);
    }

    //Set search value
    fname = argv[keyword_start_index];
    search_val = atoi(argv[keyword_start_index+1]);
    num_threads = atoi(argv[keyword_start_index+2]);

    if(num_threads < 0 || num_threads > INDEX_THREAD_MAX){
        printArgError();
        printf(PROGNAME ": error: number of threads must nonnegative"
                        " and less than or equal to %d\n", INDEX_THREAD_MAX);
        printUsage();
        exit(1);
    }



    //------------- Read in data file ----------------

    //Open file.
    if((f = fopen(fname, "r")) == NULL){
        perror(PROGNAME ": error: problem opening file");
        exit(1);
    }


    //Allocate array for the values.
    int_arr = (int*)malloc(sizeof(int) * arr_space);
    if(int_arr == NULL){
        perror(PROGNAME ": error: error allocating memory");
        exit(1);
    }

#if INDEX_DEBUG_PROGRESS > 0
    printf(PROGNAME ": scanning in values from %s...\n", fname);
#endif
    //Scan to get values, growing array if needed.
    fseek(f, 0, SEEK_SET);
    for(i = 0; (fscanf(f,"%d", &next_int) != EOF); i++){
        if(i == arr_space){
            arr_space *= 2;
            int_arr = (int*)realloc(int_arr, sizeof(int) * arr_space);
            if(int_arr == NULL){
                perror(PROGNAME ": error: error allocating memory");
                exit(1);
            }
        }
        int_arr[i] = next_int;
    }
    fclose(f);

    if(i == 0){
        fprintf(stderr, PROGNAME ": error: no values to process!\n");
        exit(1);
    }

    

    //--------------- Search The Array -----------------

    //Call the threaded search algorithm.
    if(num_threads == 0){
        for(num_threads = 1; num_threads <= INDEX_THREAD_MAX; num_threads *= 2){
#if INDEX_DEBUG_PROGRESS > 0
            printf(PROGNAME ": starting search_tree_for_val...\n");
#endif
            search_array_for_val(int_arr, i, num_threads, search_val);
        }
    } else {
#if INDEX_DEBUG_PROGRESS > 0
            printf(PROGNAME ": starting search_tree_for_val...\n");
#endif
            search_array_for_val(int_arr, i, num_threads, search_val);
    }


    return 0;
}

int search_array_for_val(int *array, int array_size, int num_threads, int val)
{
    int i, work_size;
    index_thread_args *ta;
    index_thread_args *args;

    search_val = val;
    val_found = 0;
    work_size = array_size / num_threads;

    //Allocate threads and thread arg structs
    threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
    args = (index_thread_args *)malloc(sizeof(index_thread_args) * num_threads);
    if(threads == NULL || args == NULL){
        perror(PROGNAME "error: error allocating threads");
        exit(1);
    }

    //Initialize threads
    for(i = 0; i < num_threads; i++){
        ta = &args[i];
        ta->id = i;
        ta->array = array;
        ta->start = (i * work_size);
        ta->end = ta->start + work_size;
        if(i == num_threads-1)
            ta->end = ta->end + (array_size % num_threads);
    }

    //Timing vars
    float search_time;
    struct timeval t0, t1;
    gettimeofday(&t0, NULL);

    //Create threads
    for(i = 0; i < num_threads; i++){
        pthread_create(&threads[i], NULL, thread_search_array, &args[i]);
    }

    //Join threads 
    for(i = 0; i < num_threads; i++){
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&t1, NULL);
    search_time = (float)(t1.tv_sec - t0.tv_sec) + ((float)(t1.tv_usec - t0.tv_usec)/1000000.0);

#if INDEX_DEBUG_TIME > 0
    printf(PROGNAME ": search (wall clock) time %f\n", search_time);
#endif


#if INDEX_DEBUG_PROGRESS > 0
    if(val_found == 0){
        printf(PROGNAME ": value not found!\n");
    } else {
        printf(PROGNAME ": value found!\n");
    }
    printf(PROGNAME ": all threads complete\n");
#endif

    //printf("size\t\tthreads\t\ttime\n");
    printf("%d\t\t%d\t\t%f\t\t%d\n", array_size, num_threads, search_time, val_found);

    return 0;
}

void *thread_search_array(void *args)
{
    int i, id, *array, start, end;
    index_thread_args ta = *((index_thread_args *)args);

    id = ta.id;
    array = ta.array;
    start = ta.start;
    end = ta.end;

#if INDEX_DEBUG_THREADS > 0
    printf("thread %d: search index from %d to %d...\n", id, start, end);
#endif

    for(i = start; i < end; i++){
        //Check to see if we are done
        if(val_found){
            break;
        }

        if(array[i] == search_val){
            pthread_mutex_lock(&val_found_mutex);
                val_found = 1;
            pthread_mutex_unlock(&val_found_mutex);
#if INDEX_DEBUG_THREADS > 0
            printf("thread %d: value %d found at index %d!\n", id, array[i], i);
#endif
        }
    }

#if INDEX_DEBUG_THREADS > 0
    printf("thread %d: exiting...\n", id);
#endif
    pthread_exit(0);
}

