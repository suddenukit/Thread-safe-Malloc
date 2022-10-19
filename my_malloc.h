#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#define BLOCKSIZE 24
#define TOLERANCE 0 // Tolerance for best fit method. 
#define MINIMUM_DATA_SIZE 4
typedef struct block_t{
    size_t mallocSize;
    struct block_t *prev;
    struct block_t *next;
    char dataBegin[1];
} block;

pthread_mutex_t lock;
__thread block* localHead = NULL;
__thread block* localTail = NULL;
block *globalHead = NULL;
block *globalTail = NULL;
//Thread Safe malloc/free: locking version
void *ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);
//Thread Safe malloc/free: non-locking version
void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);

block* extendHeap(size_t size);
block* findBestBlock(size_t size);
void splitBlock(block *curr, size_t size);
void replace(block* curr, block* new);
void addToList(block* curr);
void removeFromList(block* curr);
void combineBlock(block *curr);
void mergeAfter(block* curr);
void mergeBefore(block* curr);

// unsigned long get_largest_free_data_segment_size(){
//     block *curr = globalHead;
//     size_t largestSize = 0;
//     while (curr != NULL){
//         if (curr->mallocSize > largestSize){
//             largestSize = curr->mallocSize;
//         }
//         curr = curr->next;
//     }
//     return (unsigned long)largestSize;
// }
// unsigned long get_total_free_size(){
//     block *curr = globalHead;
//     size_t sum = 0;
//     while (curr != NULL){
//         sum += curr->mallocSize;
//         curr = curr->next;
//     }
//     return (unsigned long)sum;
// }