/* Written by David Ells
 *
 * A program to demonstrate the parallel DFS search of a binary tree.
 * Run with -h flag to see usage and help. */

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "tree.h"
#include "stack.h"

#define PROGNAME "dfs-search"

//Global constants
const int DFS_THREAD_MAX = 128;
const int DFS_DEBUG_THREADS = 0;
const int DFS_DEBUG_TREE = 0;
const int DFS_DEBUG_PROGRESS = 0;

//Global variables
int DFS_NUM_THREADS, DFS_TREE_SIZE;
int search_val, val_found;
dsp_stack_t **thread_work_stack;

pthread_t *threads;
pthread_mutex_t *thread_work_dsp_stack_mutex;
pthread_mutex_t val_found_mutex = PTHREAD_MUTEX_INITIALIZER;

//Function prototypes
tree *makeRandomTreeFromArray(int, int *, int);
tree *makeBalancedTreeFromArray(int, int *, int);
int search_tree_for_val(tree *t, int num_threads, int val);
void *thread_traverse_tree(void *tid);
treenode *get_next_available_treenode(int my_id);

int randint(int);
void printNode(treenode *, void *);
void findVal(treenode *, void *);

void tree_debug(int level, const char* message, ...);
void thread_debug(int level, const char* message, ...);
void prog_debug(int level, const char* message, ...);

//------
void printArgError()
{
    printf(PROGNAME ": error: wrong number of arguments\n");
}

void printUsage()
{
    printf("\t" PROGNAME " [-h | -b] [filename] [searchvalue] "
           "[number of threads]\n");
}

void printHelp()
{
    printUsage();
    printf("\n\t" PROGNAME " will process the file named, reading consecutive\n"
           "\tintegers the file and generating a binary tree from them.\n"
           "\tIt will create the number of concurrent threads specified to\n"
           "\tperform the depth first search of the tree in parallel.\n"
           "\tNote that just one processor may also be specified.\n"
           "\tIf the number of threads is 0, " PROGNAME " will perform\n"
           "\tthe search repeatedly with a growing number of threads\n"
           "\tstarting at 1 and doubling until reaching MAX_THREADS.\n");
    printf("\tOptions:\n"
           "\t\t-h : show this help\n"
           "\t\t-b : build balanced (not random) tree\n\n");
}

int main(int argc, char **argv)
{
    int i, next_int;
    int num_threads;
    int *int_arr;
    FILE* f;
    char *fname;
    int arr_space = 1024 * 100; //100kb by default

    int option_balanced = 0;
    int keyword_start_index = 1;

    //Check args for -h flag, print help and exit if found.
    for(i = 1; i < argc; i++){
        if( strcmp(argv[i], "-h") == 0){
            printf("\n");
            printHelp(); 
            exit(0);
        }
        if( strcmp(argv[i], "-b") == 0){
            option_balanced = 1;
            keyword_start_index = i+1;
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

    //Set search values
    fname = argv[keyword_start_index];
    search_val = atoi(argv[keyword_start_index+1]);
    num_threads = atoi(argv[keyword_start_index+2]);

    if(num_threads < 0 || num_threads > DFS_THREAD_MAX){
        printArgError();
        printf(PROGNAME ": error: number of threads must nonnegative"
                        " and less than or equal to %d\n", DFS_THREAD_MAX);
        printUsage();
        exit(1);
    }



    //------------- Read in data file ----------------

    //Open file.
    if((f = fopen(fname, "r")) == NULL){
        perror(PROGNAME ": error: problem opening file");
        exit(1);
    }

    //Allocate initial array for the values.
    int_arr = (int*)malloc(sizeof(int) * arr_space);
    if(int_arr == NULL){
        perror(PROGNAME ": error: error allocating memory");
        exit(1);
    }

    //Scan to get values, growing array if needed.
    fseek(f, 0, SEEK_SET);
    for(i = 0; (fscanf(f,"%d", &next_int) != EOF); i++){
        if(i == arr_space){
            arr_space *= 2;
            int_arr = (int*)realloc(int_arr, sizeof(int) * arr_space);
            if(int_arr == NULL){
                perror(PROGNAME ": error: error reallocating memory for array");
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

    DFS_TREE_SIZE = i;


    //------------- Build Tree -------------------

    tree *t;
    if(option_balanced){
        prog_debug(1, PROGNAME ": building balanced tree from input values\n");
        t = makeBalancedTreeFromArray(int_arr[0], int_arr, DFS_TREE_SIZE);
    } else {
        prog_debug(1, PROGNAME ": building random tree from input values\n");
        t = makeRandomTreeFromArray(int_arr[0], int_arr, DFS_TREE_SIZE);
    }


    //tree_print(t, stdout);
    //Print tree using function pointer scheme. Left as an example of
    //how to use tree_visit.
    if(DFS_DEBUG_TREE > 0){
        treenode_func func = printNode;
        tree_visit(t, func, (void *)stdout);
    }



    //--------------- Search The Tree -----------------

    //Single DFS search using a pointer to the findVal function. For fun.
    //func = findVal;
    //tree_visit(t, func, (void *)&search_val);

    //Call the threaded search algorithm.
    if(num_threads == 0){
        for(num_threads = 1; num_threads <= DFS_THREAD_MAX; num_threads *= 2){
                prog_debug(1, PROGNAME ": starting search_tree_for_val...\n");
                search_tree_for_val(t, num_threads, search_val);
        }
    } else {
                prog_debug(1, PROGNAME ": starting search_tree_for_val...\n");
                search_tree_for_val(t, num_threads, search_val);
    }


    return 0;
}

tree *makeRandomTreeFromArray(int randseed, int *array, int array_size)
{
    tree *t;
    treenode *n, *pnode;
    treenode **available;
    int i, r;
    char *side_attached;

    //Allocate list for nodes that have available links to attach children.
    available = (treenode**)malloc(sizeof(treenode*) * array_size);
    if(available == NULL){
        perror(PROGNAME ": error: error allocating memory");
        exit(1);
    }

    //Allocate tree.
    t = (tree*)malloc(sizeof(tree));
    if(t == NULL){
        perror(PROGNAME ": error: error allocating memory");
        exit(1);
    }
    tree_init(t);
    t->node_count = array_size;


    //For randint function
    srand(randseed); 
    for(i = 0; i < array_size; i++){

        //Allocate new treenode
        n = (treenode*)malloc(sizeof(treenode));
        if(n == NULL){
            perror(PROGNAME ": error: error allocating memory");
            exit(1);
        }

        //Initialize values of new treenode.
        treenode_init(n);
        n->id = i;
        n->data = (void*)&array[i];

        //Add to available array. If root node, set as head of tree.
        available[i] = n;
        if(i == 0){
            t->head = n;
            continue;
        }

        //Select parent randomly until found
        pnode = NULL;
        while(pnode == NULL){
            pnode = available[randint(i)];
        }

        tree_debug(2, "Parent chosen for node %d : %d\n", n->id, pnode->id);

        //Randomly select left or right child link to attach to. 
        //Try other child if the selected link is not available.
        r = randint(2);
        if(r == 0){
            if(pnode->left == NULL){
                pnode->left = n;
                side_attached = "left";
            } else {
                pnode->right = n;
                side_attached = "right";
            }
        } else {
            if(pnode->right == NULL){
                pnode->right = n;
                side_attached = "right";
            } else {
                pnode->left = n;
                side_attached = "left";
            }
        }

        //Check if both children are filled, and take off available list if so.
        if(pnode->left != NULL && pnode->right != NULL){
            available[pnode->id] = NULL;
        }

        tree_debug(2, "Child node %d attached at parent node %d on %s\n",
                        n->id, pnode->id, side_attached);
    }
    free(available);
    return t;
}

tree *makeBalancedTreeFromArray(int randseed, int *array, int array_size)
{
    tree *t;
    treenode *n, *pnode;
    treenode **available;
    int i;
    char *side_attached;

    //Allocate list for nodes that have available links to attach children.
    available = (treenode**)malloc(sizeof(treenode*) * array_size);
    if(available == NULL){
        perror(PROGNAME ": error: error allocating memory");
        exit(1);
    }

    //Allocate tree.
    t = (tree*)malloc(sizeof(tree));
    if(t == NULL){
        perror(PROGNAME ": error: error allocating memory");
        exit(1);
    }
    tree_init(t);
    t->node_count = array_size;


    //For randint function
    srand(randseed); 
    for(i = 0; i < array_size; i++){

        //Allocate new treenode
        n = (treenode*)malloc(sizeof(treenode));
        if(n == NULL){
            perror(PROGNAME ": error: error allocating memory");
            exit(1);
        }

        //Initialize values of new treenode.
        treenode_init(n);
        n->id = i;
        n->data = (void*)&array[i];

        //Add to available queue. If root node, set as head of tree.
        available[i] = n;
        if(i == 0){
            t->head = n;
            continue;
        }

        //Root [Parent]:[Child] ...
        //0 1:0 2:0 3:1 4:1 5:2 6:2 7:3 8:3 9:4 10:4
        //parent_id in balanced tree with root labeled 0 is (i-1)/2;
        pnode = available[(i-1)/2];

        if(pnode == NULL){
             printf(PROGNAME ": error: error in building balanced tree"
                             " (parent node already full)\n");
             exit(1);
        }

        tree_debug(2, "Parent chosen for node %d : %d\n", n->id, pnode->id);

        //Always select left first, if full then right.
        if(pnode->left == NULL){
            pnode->left = n;
            side_attached = "left";
        } else {
            pnode->right = n;
            side_attached = "right";
        }

        //Check if both children are filled, and take off available list if so.
        if(pnode->left != NULL && pnode->right != NULL){
            available[pnode->id] = NULL;
        }

        tree_debug(2, "Child node %d attached at parent node %d on %s\n",
                      n->id, pnode->id, side_attached);
    }
    free(available);
    return t;
}

int search_tree_for_val(tree *t, int num_threads, int val)
{
    int i;
    treenode *n;
    dsp_stack_t *s;
    int *thread_id;
    int threads_ready = 1;


    n = t->head;
    if(n == NULL) return -1;

    //Set globals for new search...
    DFS_NUM_THREADS = num_threads;
    search_val = val;
    val_found = 0;

    //Allocate threads and thread id's
    threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
    thread_id = (int *)malloc(sizeof(int) * num_threads);
    if(threads == NULL || thread_id == NULL){
        perror(PROGNAME "error: error allocating threads");
        exit(1);
    }

    //Allocate thread work stacks
    thread_work_stack = (dsp_stack_t **)malloc(sizeof(dsp_stack_t *) * num_threads);
    if(thread_work_stack == NULL){
        perror(PROGNAME "error: error allocating thread_stacks");
        exit(1);
    }
    for(i = 0; i < num_threads; i++){
        thread_work_stack[i] = dsp_stack_create(t->node_count);
    }

    //Allocate thread work stack mutexes
    thread_work_dsp_stack_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * num_threads);
    if(thread_work_dsp_stack_mutex == NULL){
        perror(PROGNAME "error: error allocating thread_stacks");
        exit(1);
    }

    //Initialize threads
    for(i = 0; i < num_threads; i++){
        thread_id[i] = i;
        dsp_stack_init(thread_work_stack[i], t->node_count);
        pthread_mutex_init(&thread_work_dsp_stack_mutex[i], NULL);
    }


    //Put root node on first thread's work stack.
    s = thread_work_stack[0];
    dsp_stack_push(s, n);


    //Spread initial work across (some) other thread work stacks.
    while(n != NULL && threads_ready < num_threads){
        //treenode_print(n, stdout);
        i = threads_ready;
        if(n->right != NULL){
            dsp_stack_push(thread_work_stack[i], n->right);
            threads_ready++;
        }
        n = n->left;
    }


    //Timing vars
    float search_time;
    struct timeval t0, t1;
    gettimeofday(&t0, NULL);


    prog_debug(1, PROGNAME ": starting %d threads\n", num_threads);

    //Create threads
    for(i = 0; i < num_threads; i++){
        pthread_create(&threads[i], NULL, thread_traverse_tree, &thread_id[i]);
    }

    //Join threads 
    for(i = 0; i < num_threads; i++){
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&t1, NULL);
    search_time = (float)(t1.tv_sec - t0.tv_sec) + ((float)(t1.tv_usec - t0.tv_usec)/1000000.0);


    if(val_found == 0){
        prog_debug(1, PROGNAME ": value not found in tree!\n");
    } else {
        prog_debug(1, PROGNAME ": value found!\n");
    }
    prog_debug(1, PROGNAME ": all threads complete\n");

    //printf("size\t\tthreads\t\ttime\t\tfound\n");
    printf("%d\t\t%d\t\t%f\t\t%d\n", DFS_TREE_SIZE, num_threads, search_time, val_found);

    //Clean up
    free(threads);
    free(thread_id);
    for(i = 0; i < num_threads; i++){
        dsp_stack_destroy(thread_work_stack[i]);
    }
    free(thread_work_stack);
    free(thread_work_dsp_stack_mutex);

    return 0;
}

void *thread_traverse_tree(void *tid)
{
    int id = *((int *)tid);
    dsp_stack_t *s = thread_work_stack[id];
    pthread_mutex_t *dsp_stack_mutex = &thread_work_dsp_stack_mutex[id];
    treenode *n;
    int node_val;

    thread_debug(1, "thread %d: started...\n", id);

    while(1){
        //Check to see if we are done
        if(val_found) break;

        //Get next node from my stack
        pthread_mutex_lock(dsp_stack_mutex);
            n = dsp_stack_pop(s);
        pthread_mutex_unlock(dsp_stack_mutex);

        //If my stack is empty, get next available node from another stack.
        if(n == NULL){
            n = get_next_available_treenode(id);
        }

        //If no work found in any other thread's stack, exit...
        if(n == NULL){
            break;
        }

        //Go depth first in searching, going to the left child
        //and pushing the right child onto my stack.
        while(n != NULL){

            //Make sure we are not done.
            if(val_found) break;

            //Because this is hit so often, we don't even compile
            //unless we absolutely need it.
            //thread_debug(3, "thread %d: traversing node %d\n", id, n->id);

            //Check value against search value
            if(n->data != NULL){
                node_val = *((int *)n->data);
                if(node_val == search_val){
                    val_found = 1;
                    thread_debug(1, "thread %d: value %d found at node %d!\n",
                                 id, node_val, n->id);
                    break;
                }
            }

            if(n->right != NULL){
                pthread_mutex_lock(dsp_stack_mutex);
                    dsp_stack_push(s, n->right);
                pthread_mutex_unlock(dsp_stack_mutex);
            }
            n = n->left;
        }
    }

    thread_debug(1, "thread %d: exiting...\n", id);
    pthread_exit(0);
}

treenode *get_next_available_treenode(int my_id)
{
    int r, i, j;
    dsp_stack_t *s;
    treenode *n = NULL;

    r = randint(DFS_NUM_THREADS);

    //Search starting from random index for a thread with available work
    for(i = 0; i < DFS_NUM_THREADS && n == NULL; i++) {

        //Allows us to wrap around.
        j = (i+r)%DFS_NUM_THREADS;

        //Check thread's stack for work
        s = thread_work_stack[j];
        pthread_mutex_lock(&thread_work_dsp_stack_mutex[j]);
            if(!dsp_stack_isempty(s)){
                n = dsp_stack_del_first(s);
                thread_debug(2, "thread %d: found work in thread %d's stack!\n",
                                my_id, j);
            }
        pthread_mutex_unlock(&thread_work_dsp_stack_mutex[j]);

    }

    return n;
}
    

int randint(int max)
{
    return ((double)rand() / (double)RAND_MAX) * max;
}

void printNode(treenode *n, void *arg)
{
    FILE *f = (FILE *)arg;

    fprintf(f, "node %d [%d]:", n->id, *(int *)n->data);
    if(n->left != NULL)
        fprintf(f, " left:%d [%d]", n->left->id, *(int *)n->left->data);
    if(n->right != NULL)
        fprintf(f, " right:%d [%d]", n->right->id, *(int *)n->right->data);
    fprintf(f, "\n");
}

void findVal(treenode *n, void *arg)
{
    int node_val;
    int arg_val = *((int *)arg);
    if(n->data != NULL){
        node_val = *((int *)n->data);
        if(arg_val == node_val){
            printf("Found val at node %d!\n", n->id);
        }
    }
}

void tree_debug(int level, const char* message, ...)
{
    if(DFS_DEBUG_TREE >= level){
        va_list printf_args;
        va_start(printf_args, message);
        vfprintf(stderr, message, printf_args);
        va_end(printf_args);
    }
}

void thread_debug(int level, const char* message, ...)
{
    if(DFS_DEBUG_THREADS >= level){
        va_list printf_args;
        va_start(printf_args, message);
        vfprintf(stderr, message, printf_args);
        va_end(printf_args);
    }
}

void prog_debug(int level, const char* message, ...)
{
    if(DFS_DEBUG_PROGRESS >= level){
        va_list printf_args;
        va_start(printf_args, message);
        vfprintf(stderr, message, printf_args);
        va_end(printf_args);
    }
}
