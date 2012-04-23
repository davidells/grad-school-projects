// Written by David Ells
//
// A program that uses a parallelized quicksort to sort
// the list of numbers in ./lab6.dat. The number of threads
// to be used is passed on the command line.

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define DEBUG_LEVEL 3

//Global constants
const char *LAB6_DATA_FILE = "lab6.dat";
const int MAX_NUMS = 50;


typedef struct {
    int group_id;
    int group_start;
    int group_end;
    int group_pivot;
    int group_size;
    int group_leader_id;
    int group_gone;
    int group_barrier_num;
    int *s_sums;
    int *l_sums;
    pthread_mutex_t s_sums_mutex;
    pthread_mutex_t l_sums_mutex;
    pthread_mutex_t barrier_mutex;
    pthread_cond_t barrier_cond;
} thread_group;

typedef struct {
    int id;
    int start;
    int end;
    thread_group *group;
} thread_env;



//Global variables
char *PROGNAME;
int running_threads;
int globalPivot;
int *int_arr, arr_size, *aux_int_arr;
int *s_sums, *l_sums;
int threads_ready;
thread_env *tenvs;
thread_group *tgroups;

pthread_mutex_t s_sums_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t l_sums_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t barrierMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t barrierCondition = PTHREAD_COND_INITIALIZER;

pthread_mutex_t arrange_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t arrange_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t envs_ready_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t envs_ready_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

//Function prototypes
void *thread_begin_quicksort(void *);
void threaded_quicksort(int num_threads);
void debug(int level, const char* message, ...);
void serial_quicksort(int *array, int start, int end);
void swap(int *a, int *b);
void debug_array(int level, int *arr, int start, int end);
int randint(int min, int max);
void group_barrier(thread_group *);
void global_barrier();
void debug_thread_env(int level, thread_env *te);
void debug_thread_group(int level, thread_group *tg);
int arrange_section_by_pivot(int *array, int start, int end, int pivot);


int main(int argc, char *argv[])
{
    int i;
    int arr_space = 64;

    FILE *f;
    int next_int;
    int num_threads;
    int *thread_id;


    PROGNAME = argv[0];

    //Argument processing
    if(argc != 2){
        fprintf(stderr, "usage: %s [number of threads]\n", PROGNAME);
        exit(1);
    }

    if((num_threads = atoi(argv[1])) <= 0){
        fprintf(stderr, "argument for number of threads must be greater than 0\n");
        exit(1);
    }


    //Open file.
    if((f = fopen(LAB6_DATA_FILE, "r")) == NULL){
        fprintf(stderr, "%s: error opening file %s\n", PROGNAME, LAB6_DATA_FILE);
        exit(1);
    }

    //Allocate initial array for the values.
    int_arr = (int*)malloc(sizeof(int) * arr_space);
    if(int_arr == NULL){
        fprintf(stderr, "%s: error allocating memory\n", PROGNAME);
        exit(1);
    }

    //Scan to get values, growing array if needed.
    fseek(f, 0, SEEK_SET);
    for(i = 0; (fscanf(f,"%d", &next_int) != EOF) && i < MAX_NUMS; i++){
        if(i == arr_space){
            arr_space *= 2;
            int_arr = (int*)realloc(int_arr, sizeof(int) * arr_space);
            if(int_arr == NULL){
                fprintf(stderr, "%s: error reallocating memory for array\n", PROGNAME);
                exit(1);
            }
        }
        int_arr[i] = next_int;
    }
    arr_size = i;
    fclose(f);

    //Allocate auxilary int array
    aux_int_arr = (int *)malloc(sizeof(int) * arr_size);

    if(i < 1){
        fprintf(stderr, "%s: error: no values to sort\n", PROGNAME);
        exit(1);
    }


    debug(1, "array before sort...\n");
    debug_array(1, int_arr, 0, arr_size);

    threaded_quicksort(num_threads);

    debug(1, "array after sort...\n");
    debug_array(1, int_arr, 0, arr_size);

    return 0;
}

void threaded_quicksort(int num_threads)
{
    int i, j, *temp_arr_ptr;
    pthread_t *threads;
    int initial_pivot = int_arr[0];
    int rounds = 0;

    //Set global vars
    running_threads = num_threads;

    //Allocate threads, thread environments, and thread groups
    threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
    tenvs = (thread_env *)malloc(sizeof(thread_env) * num_threads);
    tgroups = (thread_group *)malloc(sizeof(thread_group) * num_threads);


    debug(1, "main: initing threads\n");
    //Init threads and groups
    for(i = 0; i < num_threads; i++){
        tenvs[i].id = i;
        tenvs[i].start = (arr_size/num_threads) * i;
        tenvs[i].end = (arr_size/num_threads) * (i+1);
        if(i == num_threads-1)
            tenvs[i].end = tenvs[i].end + (arr_size%num_threads);
        tenvs[i].group = &tgroups[0];

        tgroups[i].group_id = i;
        tgroups[i].group_start = 0;
        tgroups[i].group_end = arr_size;
        tgroups[i].group_pivot = initial_pivot;
        tgroups[i].group_size = num_threads;
        tgroups[i].group_gone = 0;
        tgroups[i].group_leader_id = 0;
        tgroups[i].s_sums = (int *)malloc(sizeof(int) * (num_threads+1));
        tgroups[i].l_sums = (int *)malloc(sizeof(int) * (num_threads+1));
        for(j = 0; j < num_threads+1; j++){
            tgroups[i].s_sums[j] = 0;
            tgroups[i].l_sums[j] = 0;
        }
        pthread_mutex_init(&(tgroups[i].s_sums_mutex), NULL);
        pthread_mutex_init(&(tgroups[i].l_sums_mutex), NULL);
        pthread_mutex_init(&(tgroups[i].barrier_mutex), NULL);
        pthread_cond_init(&(tgroups[i].barrier_cond), NULL);
    }

    debug(1, "main: creating threads\n");
    //Create threads
    for(i = 0; i < num_threads; i++){
        pthread_create(&threads[i], NULL, thread_begin_quicksort, &tenvs[i]);
    }


    //Next available group has id 1, since threads start in group 0
    int next_available_group = 1;

    //Main thread loop, wait until each round complete, arrange new groups
    //signal threads to continue to next round.
    while(1){


        //Wait for threads to fill auxilary array
        pthread_mutex_lock(&arrange_mutex);
        pthread_cond_wait(&arrange_cond, &arrange_mutex);
        pthread_mutex_unlock(&arrange_mutex);



        //debug(1, "main: done waiting on threads\n");

        //We finally exit when all threads have completed
        pthread_mutex_lock(&envs_ready_mutex);
            if(running_threads <= 0){
                //Make sure and release any (completed) waiting threads
                pthread_mutex_unlock(&envs_ready_mutex);
                pthread_cond_broadcast(&envs_ready_cond);
                break;
            }
        pthread_mutex_unlock(&envs_ready_mutex);

        //Swap aux array into main array
        temp_arr_ptr = int_arr;
        int_arr = aux_int_arr;
        aux_int_arr = temp_arr_ptr;

        debug(2, "main: global arr after round %d...\n", ++rounds);
        debug_array(2, int_arr, 0, arr_size);


        int next_thread = 0;
        int threads_for_smaller = 0;
        int group_small_size = 0;
        int group_divide = 0;

        thread_group *g, *g2;

        //Set up groups for next round
        for(i = 0; i < num_threads; i++){

            //If we have exhausted either the threads or groups, break
            if(next_thread >= num_threads || next_available_group >= num_threads){
                break;
            }

            g = &tgroups[i];

            if(g->group_gone)
                continue;

            //Get size of set of all numbers in group less than the pivot.
            group_small_size = g->s_sums[(g->group_size)];
            if(group_small_size == 0) group_small_size = 1;

            //Determine number of threads to leave on "smaller than" set of current group
            threads_for_smaller = (int)((double)(group_small_size * g->group_size) / 
                                        (double)(g->group_end - g->group_start));
            if(threads_for_smaller == 0) threads_for_smaller = 1;

            //Determine the dividing index in the current group to split groups at.
            group_divide = g->group_start + group_small_size;

            debug(3, "main: group %d, start %d, end %d, group_small_size %d, threads_for_smaller %d\n",
                      g->group_id, 
                      g->group_start, 
                      g->group_end, 
                      group_small_size,
                      threads_for_smaller);

            //Set up group for dealing with the larger than (pivot) set
            g2 = &tgroups[next_available_group++];
            g2->group_start = group_divide;
            g2->group_end = g->group_end;
            g2->group_pivot = int_arr[g2->group_start];
            g2->group_size = g->group_size - threads_for_smaller;
            g2->group_leader_id = g->group_leader_id + threads_for_smaller;

            //Debug new group settings
            debug(2, "main: new group created... ");
            debug_thread_group(2, g2);
            
            //Set new settings for existing group
            g->group_end = group_divide;
            g->group_pivot = int_arr[g->group_start];
            g->group_size = threads_for_smaller;

            
            int k = 0;
            int group_arr_size = 0;

            //Reassign values for threads in old group
            next_thread = g->group_leader_id;
            group_arr_size = g->group_end - g->group_start;
            for(j = 0; j < g->group_size; j++){
                k = next_thread + j;
                tenvs[k].start = g->group_start + ((group_arr_size/g->group_size) * j);
                tenvs[k].end = g->group_start + ((group_arr_size/g->group_size) * (j+1));
                if(j == g->group_size-1)
                    tenvs[k].end = tenvs[k].end + (group_arr_size % g->group_size);
                tenvs[k].group = g;
            }

            //Assign threads to new group
            next_thread = g2->group_leader_id;
            group_arr_size = g2->group_end - g2->group_start;
            for(j = 0; j < g2->group_size; j++){
                k = next_thread + j;
                tenvs[k].start = g2->group_start + ((group_arr_size/g2->group_size) * j);
                tenvs[k].end = g2->group_start + ((group_arr_size/g2->group_size) * (j+1));
                if(j == g2->group_size-1)
                    tenvs[k].end = tenvs[k].end + (group_arr_size % g2->group_size);
                tenvs[k].group = g2;
            }
            next_thread += j;

            for(j = 0; j < num_threads; j++){
                g->s_sums[j] = 0;
                g->l_sums[j] = 0;
            }

        }

        //Signal threads to continue to next round
        debug(2, "main: signaling threads\n");
        pthread_cond_broadcast(&envs_ready_cond);
    }

    debug(1, "main: joining threads...\n");
    //Join threads 
    for(i = 0; i < num_threads; i++){
        pthread_join(threads[i], NULL);
    }


    free(threads);
    free(tenvs);
    free(tgroups);
}

void *thread_begin_quicksort(void *arg)
{
    int i, move;
    int id, start, end;
    int group_id, group_start, group_end, group_pivot, group_leader_id, group_small_size;
    int aux_small_index, aux_large_index;
    int local_small_size, local_large_size, local_group_position;
    int complete = 0;

    thread_env *te;
    thread_group *tg;
    int group_size;
    int *s_sums, *l_sums;
    pthread_mutex_t *s_mutex, *l_mutex, *b_mutex;


    te = (thread_env *)arg;

    debug(1, "%d: started...\n", te->id);

    while(!complete){
        //Get env vars
        id = te->id;
        start = te->start;
        end = te->end;

        //Get group vars
        tg = te->group;
        group_id = tg->group_id;
        group_start = tg->group_start;
        group_end = tg->group_end;
        group_pivot = tg->group_pivot;
        group_size = tg->group_size;
        group_leader_id = tg->group_leader_id;
        s_sums = tg->s_sums;
        l_sums = tg->l_sums;
        s_mutex = &(tg->s_sums_mutex);
        l_mutex = &(tg->l_sums_mutex);
        b_mutex = &(tg->barrier_mutex);

        //debug(2, "%d: executing with env: "); 
        //debug_thread_env(2, te);

        //If I am the only one in my group, serial sort and exit
        if(start == group_start && end == group_end){

            debug(2, "%d: calling serial quicksort on "
                     "int_arr %d through %d\n", id, start, end);
            serial_quicksort(int_arr, start, end);
            for(i = start; i < end; i++) {
                debug(3, "aux_int_arr[%d] = int_arr[%d](%d)\n", i, i, int_arr[i]);
                aux_int_arr[i] = int_arr[i];
            }

            complete = 1;

        } else {

            debug(3, "%d: calling moving step...\n", id);
            move = arrange_section_by_pivot(int_arr, start, end, group_pivot);

            local_small_size = (move-start);
            local_large_size = (end-move);
            local_group_position = id - group_leader_id;

            //Debug local array and local small size
            /*pthread_mutex_lock(&print_mutex);
                debug(2, "%d: local array... local_small_size=%d\n", id, local_small_size);
                debug_array(2, int_arr, start, end);
            pthread_mutex_unlock(&print_mutex);*/
            

            //Add to small size and large size prefix sums
            pthread_mutex_lock(s_mutex);
                for(i = local_group_position; i < group_size; i++){
                    s_sums[i+1] += local_small_size;
                    l_sums[i+1] += local_large_size;
                }
            pthread_mutex_unlock(s_mutex);


            //Barrier to allow all threads to finish adding to the prefix sums
            group_barrier(tg);

            //Debug s sums and l sums
            /*pthread_mutex_lock(&print_mutex);
                debug(2, "group %d s_sums: ", tg->group_id); debug_array(2, s_sums, 0, tg->group_size+1);
                debug(2, "group %d l_sums: ", tg->group_id); debug_array(2, l_sums, 0, tg->group_size+1);
            pthread_mutex_unlock(&print_mutex);*/

            //Get starting indexes for "smaller than" and "larger than" sets
            group_small_size = s_sums[group_size];
            aux_small_index = group_start + s_sums[local_group_position];
            aux_large_index = group_start + group_small_size + l_sums[local_group_position];
            debug(3, "thread %d: small index = %d, large index = %d\n", id, aux_small_index, aux_large_index);


            //Global rearrangement using auxilary array
            for(i = 0; i < local_small_size; i++) {
                debug(3, "aux_int_arr[%d] = int_arr[%d]\n", aux_small_index + i, start+i);
                aux_int_arr[aux_small_index + i] = int_arr[start+i];
            }
            for(i = 0; i < local_large_size; i++) {
                debug(3, "aux_int_arr[%d] = int_arr[%d]\n", aux_small_index + i, start+i);
                aux_int_arr[aux_large_index + i] = int_arr[move+i];
            }
        }

        //We let all groups finish before notifying main thread, waiting for notification from main
        //-------Global Barrier----------------
        pthread_mutex_lock(&envs_ready_mutex);


            if(complete){
                threads_ready--;
                running_threads--;
                tg->group_gone = 1;
            }

            threads_ready += 1;
            if(threads_ready >= running_threads){
                threads_ready = 0;
                pthread_cond_signal(&arrange_cond);
                pthread_cond_wait(&envs_ready_cond, &envs_ready_mutex);
            } 
            else if(!complete){
                debug(2, "%d: waiting on main\n", id);
                pthread_cond_wait(&envs_ready_cond, &envs_ready_mutex);
            }


        pthread_mutex_unlock(&envs_ready_mutex);
        //--------------------------------------


    }

    debug(1, "%d: exiting\n", id);
    pthread_exit(0);
}

void debug(int level, const char* message, ...)
{
#if DEBUG_LEVEL > 0 
    if(level <= DEBUG_LEVEL){
        va_list printf_args;
        va_start(printf_args, message);
        vfprintf(stderr, message, printf_args);
        va_end(printf_args);
    }
#endif
}

int arrange_section_by_pivot(int *array, int start, int end, int pivot)
{
    int i, move;

    if(start < end){
        move = start;
        for(i = start; i < end; i++){
            if(array[i] < pivot){
                swap(&array[move], &array[i]);
                move = move + 1;
            }
        }
    }
    return move;
}

void serial_quicksort(int *array, int start, int end)
{
    int i, pivot, move, temp;

    if(start < end){

        pivot = array[start];
        move = start;

        for(i = start+1; i < end; i++){
            if(array[i] < pivot){
                move = move + 1;
                swap(&array[move], &array[i]);
            }
        }
        swap(&array[start], &array[move]);
        serial_quicksort(array, start, move);
        serial_quicksort(array, move+1, end);
    }
}

void swap(int *a, int *b)
{
    int c = *a;
    *a = *b;
    *b = c;
}

void debug_array(int level, int *arr, int start, int end)
{
#if DEBUG_LEVEL > 0 
    int i;
    if(level <= DEBUG_LEVEL){
        for(i = start; i < end; i++){
            debug(level, "%d  ", arr[i]);
        }
        debug(level, "\n");
    }
#endif
}


int randint(int min, int max)
{
    int range = max - min;
    return min + (((double)rand() / (double)RAND_MAX) * range);
}

void group_barrier(thread_group *tg)
{
    //-------Group Barrier----------------
    pthread_mutex_t *b_mutex = &(tg->barrier_mutex);
    pthread_cond_t *b_cond = &(tg->barrier_cond);

    pthread_mutex_lock(b_mutex);

        tg->group_barrier_num += 1;
        if(tg->group_barrier_num >= tg->group_size){
            tg->group_barrier_num = 0;
            pthread_cond_broadcast(b_cond);
        } else {
            pthread_cond_wait(b_cond, b_mutex);
        } 

    pthread_mutex_unlock(b_mutex);
}

void global_barrier()
{
    //-------Global Barrier----------------
    pthread_mutex_t *b_mutex = &barrierMutex;
    pthread_cond_t *b_cond = &barrierCondition;

    pthread_mutex_lock(b_mutex);

        threads_ready += 1;
        if(threads_ready >= running_threads){
            threads_ready = 0;
            pthread_cond_broadcast(b_cond);
        } else {
            pthread_cond_wait(b_cond, b_mutex);
        } 

    pthread_mutex_unlock(b_mutex);
}

void debug_thread_env(int level, thread_env *te)
{
#if DEBUG_LEVEL > 0 
    thread_group *tg = te->group;
    debug(level, "thread: %d, group: %d, start: %d, end: %d, ",
            te->id, tg->group_id, te->start, te->end);
    debug_thread_group(level, tg);
#endif

}

void debug_thread_group(int level, thread_group *tg)
{
#if DEBUG_LEVEL > 0 
    debug(level, "group: %d, group_start: %d, group_end: %d, group_pivot: %d, "
           "group_leader_id: %d, group_size: %d\n", 
           tg->group_id, tg->group_start, tg->group_end, tg->group_pivot, 
           tg->group_leader_id, tg->group_size);
#endif
}
