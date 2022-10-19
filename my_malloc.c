#include "my_malloc.h"

size_t alignSize(size_t size){
    return ((((size - 1) >> 2) << 2) + 4);
}

void *ts_malloc_lock(size_t size){
    pthread_mutex_lock(&lock);
    size = alignSize(size);
    block* myBlock;
    myBlock = findBestBlock(size);
    if (myBlock == NULL){
        myBlock = extendHeap(size);
    }
    else{
        if (myBlock->mallocSize - size >= BLOCKSIZE + MINIMUM_DATA_SIZE){
            splitBlock(myBlock, size);
        }
        else {
            removeFromList(myBlock);
        }
    }
    pthread_mutex_unlock(&lock);
    return myBlock->dataBegin;
}

void *ts_malloc_nolock(size_t size){
    size = alignSize(size);
    block* myBlock;
    myBlock = findBestBlock(size);
    if (myBlock == NULL){
        pthread_mutex_lock(&lock);
        myBlock = extendHeap(size);
        pthread_mutex_unlock(&lock);
    }
    else{
        if (myBlock->mallocSize - size >= BLOCKSIZE + MINIMUM_DATA_SIZE){
            splitBlock(myBlock, size);
        }
        else {
            removeFromList(myBlock);
        }
    }
    return myBlock->dataBegin;
}

void ts_free_lock(void *ptr){
    pthread_mutex_lock(&lock);
    char* curr = ptr;
    block* freeBlock = ptr - BLOCKSIZE;
    if (freeBlock == NULL){
        fprintf(stderr, "A wrong address for free is given!\n");
        exit(EXIT_FAILURE);
    }
    addToList(freeBlock);
    combineBlock(freeBlock); 
    pthread_mutex_unlock(&lock);
}

void ts_free_nolock(void *ptr){
    pthread_mutex_lock(&lock);
    char* curr = ptr;
    block* freeBlock = ptr - BLOCKSIZE;
    if (freeBlock == NULL){
        fprintf(stderr, "A wrong address for free is given!\n");
        exit(EXIT_FAILURE);
    }
    addToList(freeBlock);
    combineBlock(freeBlock); 
    pthread_mutex_unlock(&lock);
}

block* extendHeap(size_t size){
    block *curr = sbrk(0);
    if (sbrk(BLOCKSIZE + size) == (void *)-1){
        fprintf(stderr, "The sbrk function gets wrong. Exiting!\n");
        exit(EXIT_FAILURE);
    }
    curr->mallocSize = size;
    curr->next = NULL;
    curr->prev = NULL;
    return curr;
}

block* findBestBlock(size_t size){
    block *curr = globalHead;
    block *bestBlock = NULL;
    size_t bestSize = LONG_MAX;
    while (curr != NULL){
        if (curr->mallocSize >= size && curr->mallocSize < bestSize){
            bestSize = curr->mallocSize;
            bestBlock = curr;
            if (bestSize <= size + TOLERANCE){
                return bestBlock;
            }
        }
        else{
            curr = curr->next;
        }
    }
    return bestBlock;
}

void splitBlock(block *curr, size_t size){
    char* temp = (char* )curr;
    temp += BLOCKSIZE + size;
    block* new = (block *)temp;    
    new->mallocSize = curr->mallocSize - size - BLOCKSIZE;
    curr->mallocSize = size;
    replace(curr, new);
    //addToList(new);
    //removeFromList(curr); 
    combineBlock(new);
}

void replace(block* curr, block* new){
    new->prev = curr->prev;
    new->next = curr->next;
    //printf("Replace:    ");
    if (curr == globalHead && curr == globalTail){
        //printf("Only 1 block.\n");
        globalHead = new;
        globalTail = new;
    }
    else if (curr == globalHead){
        //printf("curr = globalHead\n");
        globalHead = new;
        curr->next->prev = new;
        curr->next = NULL;
    }
    else if (curr == globalTail){
        //printf("curr = globalTail\n");
        globalTail = new;
        curr->prev->next = new;
        curr->prev = NULL;
    }
    else {
        //printf("curr in middle\n");
        curr->next->prev = new;
        curr->prev->next = new;
        curr->next = NULL;
        curr->prev = NULL;
    }
}

void addToList(block* curr){
    if (globalHead == NULL){
        //printf("Empty! Add!\n");
        globalHead = curr;
        globalTail = curr;
        return;
    }
    else if (curr < globalHead){
        //printf("Add to globalHead!\n");
        curr->prev = NULL;
        curr->next = globalHead;
        globalHead->prev = curr;
        globalHead = curr;
    }
    else if (curr > globalTail){
        //printf("Add to globalTail!\n");
        curr->prev = globalTail;
        curr->next = NULL;
        globalTail->next = curr;
        globalTail = curr;
    }
    else {
        block * ptr = globalHead;
        while(ptr != NULL){
            if (curr < ptr){
                curr->next = ptr;
                curr->prev = ptr->prev;
                ptr->prev->next = curr;
                ptr->prev = curr;
                return;
            }
            ptr = ptr->next;
        }
        //printf("Add to middle!\n");
    }
}

void removeFromList(block* curr){
    if (globalHead == NULL){
        return;
    }
    else if (curr == globalHead && curr == globalTail){
        globalHead = NULL;
        globalTail = NULL;
    }
    else if (curr == globalHead){
        globalHead = curr->next;
        globalHead->prev = NULL;
        curr->next = NULL;
    }
    else if (curr == globalTail){
        globalTail = curr->prev;
        globalTail->next = NULL;
        curr->prev = NULL;
    }
    else {
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
        curr->prev = NULL;
        curr->next = NULL;
    }
}

void combineBlock(block *curr){
    if (curr == globalHead && curr == globalTail){
        //printf("Only 1 block");
        return;
    }
    else if (curr == globalHead){
        //printf("curr = globalHead\n");
        mergeAfter(curr);
    }
    else if (curr == globalTail){
        //printf("curr = globalTail\n");
        mergeBefore(curr);
    }
    else {
        //printf("curr in middle\n");
        mergeAfter(curr);
        mergeBefore(curr);
    }
}

void mergeAfter(block* curr){
    if (curr->dataBegin + curr->mallocSize == (char*)curr->next){
        curr->mallocSize += curr->next->mallocSize + BLOCKSIZE;
        curr->next = curr->next->next;
        if (curr->next){
            curr->next->prev = curr;
        }
        else {
            globalTail = curr;
        }
    }
}

void mergeBefore(block* curr){
    //if (curr->prev == NULL) //printf("curr->prev=NULL\n");
    if (curr->prev->dataBegin + curr->prev->mallocSize == (char*)curr){
        //printf("Merging before!\n");
        block* temp = curr;
        curr = curr->prev;
        curr->mallocSize += curr->next->mallocSize + BLOCKSIZE;
        curr->next = temp->next;
        if (temp == globalTail){
            globalTail = curr;
        }
        else{
            temp->next->prev = curr;
        }
    }
}