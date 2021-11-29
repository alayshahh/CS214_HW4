#include <stdio.h>
#include <stdlib.h>

#include "constants.h"

#define DEBUG 0
#define INIT_DEBUG 0

void* heap;
void* endHeap;
void* firstBlock;  //contains address of the first free block
void* nextBlock;   //for next fit algo
double askedSize = 0;
int allocAlg = -1;

/*
  design of free block              design of malloced block
   | size_t     |                    |      size_t       |
   | next       |                    |  requested size   | 
   | prev       |                    |      payload      |
   | free space |                    |       ...         | 
   | size_t     |                    |      size_t       |

    size_t = 8 bytes                 
    next = 8 bytes
    prev = 8 bytes
    size_ = 8 bytes
    header & footer size in free = 32 bytes
    header & footer size in used block = 16 bytes
*/

void myinit(int allocArg) {
    /*
    initialize the memory allacator:
        malloc 1 MB of memory
        set the alloc Arg
        set the malloc block meta data so we can begin to use it 
        set nextBlock & firstBlock
    */
    heap = malloc(1 << 20);
    unsigned long* beginningOfHeap = (unsigned long*)heap;
    *beginningOfHeap = BEGINNING_FREE_SPACE;
    *beginningOfHeap &= ~(1 << 0);  //set unallocated bit
    if (INIT_DEBUG) {
        printf("begin=%p\n", (void*)beginningOfHeap);
        printf("value at point  = %zu\n", *beginningOfHeap);
    }

    unsigned long* next = (unsigned long*)(beginningOfHeap + 1);
    *next = 0;

    unsigned long* prev = (unsigned long*)(beginningOfHeap + 2);
    *prev = 0;
    if (DEBUG) {
        printf("next =%p\n", (void*)next);
        printf("prev =%p\n", (void*)prev);
    }

    unsigned long* endOfHeap = ((unsigned long*)heap) + 131071;  // (1MB - 8 bytes)/ 8 bytes
    *endOfHeap = BEGINNING_FREE_SPACE;
    *endOfHeap &= ~(1 << 0);  //set unallocated bit
    if (INIT_DEBUG) {
        printf("endheap=%p\n", (void*)endOfHeap);
        printf("diff bw pointers: %ld \n", ((char*)endOfHeap) - ((char*)beginningOfHeap));
    }

    endHeap = endOfHeap + 1;

    allocAlg = allocArg;
    firstBlock = heap;
    nextBlock = heap;
}

void mycleanup() {
    /*
    reset all stats and free heap
    */
    firstBlock = NULL;
    nextBlock = NULL;
    askedSize = 0;
    allocAlg = -1;
    free(heap);
}

void* setMallocBlock(unsigned long* ptr, size_t size, size_t asked) {
    /*
        passed in a pointer to the beginnign fo free block we want to set up for use
        it will set the next and prev block metadata & split the current free block if needed (& set metadata of split block)
    */

    // printf("%p\n", ptr);
    //save old data
    size_t oldSize = *ptr & -2;
    unsigned long next = *(ptr + 1);
    unsigned long prev = *(ptr + 2);

    //there is enough space
    //do we split?
    if (DEBUG) {
        printf("pointer at: %p, value at pointer %lu next: %lu prev: %lu \n", ptr, *ptr, *(ptr + 1), *(ptr + 2));
        printf("*ptr (%lu) - size (%lu) = %lu bytes \n", *ptr, size, (*ptr - size));
    }

    if (((*ptr) - size) < 32) {  // dont split
        // printf("dont split");
        unsigned long* endSize = (ptr + (*ptr / 8)) - 1;

        *endSize |= 1 << 0;  // set allocated bit at end
        *ptr |= 1 << 0;      //set allocated bit in front
        nextBlock = (void*)next;
        if (next != 0) {  //set next prev equal to current prev
            *((unsigned long*)next + 2) = prev;
        }
        if (prev != 0) {  //set prev's next equal to current next
            *((unsigned long*)prev + 1) = next;
        } else
            firstBlock = (void*)next;
        // set head of freeBlock
        *(ptr + 1) = asked;
        if (DEBUG) {
            printf("malloced starts at: %p\n", ptr);
            printf("payload at %p\n", ptr + 2);
            printf("size of malloc: %lu\n", *ptr);
            printf("asked size: %lu\n", *(ptr + 1));
            printf("next: %lu\n", next);
            printf("prev: %lu\n", prev);
            printf("endSize: %lu\n", *endSize);
            printf("ends at: %p\n", endSize);
        }

        return (void*)(ptr + 2);
    }
    *ptr = size;
    *ptr |= 1 << 0;  //set allocated bit

    //set second size
    unsigned long* secondSize = (ptr + (size / 8)) - 1;
    *secondSize = size;
    *secondSize |= 1 << 0;  //set allocated bit

    //set split block
    unsigned long* block = (secondSize + 1);
    *block = oldSize - size;
    *block &= ~(1 << 0);  //set unallocated bit
    *(block + 1) = next;  //set block next to current next
    *(block + 2) = prev;  //set block prev to current prev
    if (prev == 0) {
        firstBlock = (void*)block;
    } else {
        *((unsigned long*)prev + 1) = (unsigned long)block;
        if (DEBUG) printf("prev next %p\n", (unsigned long*)*((unsigned long*)prev + 1));  //set the prev next to block
    }
    if (next != 0) {  //set next prev equal to block
        *((unsigned long*)next + 2) = (unsigned long)block;
    }
    nextBlock = (void*)block;
    unsigned long* endSize = (ptr + (oldSize / 8) - 1);
    *endSize = oldSize - size;
    *endSize &= ~(1 << 0);  //set unallocated bit
    *(ptr + 1) = asked;
    if (DEBUG) {
        printf("malloc starts at (%p) -> size: %lu \n", (void*)ptr, *ptr);
        printf("payload at %p\n", ptr + 1);
        printf("end of malloc block at (%p) -> size: %lu\n", (void*)secondSize, *secondSize);
        printf("split block starts at %p -> value %lu \n", block, *block);
        printf("split block ends at (%p) -> value %lu\n", endSize, *endSize);
        printf("next free from split: %p\n", (unsigned long*)*(block + 1));
        printf("prev free from split: %p\n", (unsigned long*)*(block + 2));
    }

    return (void*)(ptr + 2);
}

void* findFit(void* start, size_t size, size_t ask) {
    /*
    finds the next fit beginning from the start pointer 

    */
    if (start == NULL) {
        return NULL;
    }
    unsigned long* ptr = (unsigned long*)start;

    if (DEBUG) printf("looking at %p, head is %p start is %p\n", ptr, heap, start);
    while (ptr != 0 && *(ptr) < size) {
        ptr = (unsigned long*)*(ptr + 1);
        if (DEBUG) printf("looking at %p, head is %p start is %p\n", ptr, heap, start);
    }
    if (ptr == 0 || *ptr < size) return NULL;
    return setMallocBlock(ptr, size, ask);  //reached end of list, not enough space
}

void* findBestFit(size_t size, size_t ask) {
    /*
    finds the smallest possible block that fits the size needed
    */
    unsigned long* ptr = firstBlock;
    unsigned long* bestFit = NULL;
    while (ptr != 0) {
        if (DEBUG) {
            printf(" finding best fit, checking %p...\n", ptr);
        }

        if (*ptr >= size) {
            if (bestFit == NULL) {
                bestFit = ptr;
            } else if ((((*ptr) - size) < (*bestFit) - size)) {
                bestFit = ptr;
            }
        }
        ptr = (unsigned long*)*(ptr + 1);
    }
    if (bestFit == NULL) {
        return NULL;
    }
    return setMallocBlock(bestFit, size, ask);
}

size_t findNearestMultipleof8(size_t size) {
    if (size % 8 != 0)
        size += (8 - (size % 8));
    // we want the malloced block to be atleast 32 bytes so when we free, it will be big enough to store all meta data
    return size > 32 ? size : 32;
}

void* mymalloc(size_t size) {
    /*
    takes in N number of bytes and tries to find space in the heap to fit the data along w meta data needed
    returns null if not enough space.
    */
    if (size == 0) {
        return NULL;
    }
    size_t mallocAsk = size;
    size = size + 24;  // [size , requested size, payload, size]
    size = findNearestMultipleof8(size);
    // printf("Size = %lu\n", size);
    void* ptr = NULL;
    if (allocAlg == FIRST_FIT) {
        // printf("asked for %lu, giving them %lu\n", mallocAsk, size);
        ptr = findFit(firstBlock, size, mallocAsk);
    } else if (allocAlg == NEXT_FIT) {
        // printf("next fit");
        // if (nextBlock == NULL) printf("NEXTBLOCK = NULL\n");
        ptr = findFit(nextBlock, size, mallocAsk);  //start from next free block from allocated block
        if (ptr == NULL) {                          //if we dont find anything, try from the beginning
            ptr = findFit(firstBlock, size, mallocAsk);
        }
    } else if (allocAlg == BEST_FIT) {  //best fit
        ptr = findBestFit(size, mallocAsk);
    
    if (ptr != NULL) {
        askedSize += mallocAsk;
        size = *((unsigned long*)ptr - 2);
        size &= ~(1 << 0);
        amountUsed += size;
    }
    if (DEBUG) {
        printf("%f amount used (space left = %f), %lu amount needed\n FREE LIST: \n", amountUsed, (1024 * 1024) - amountUsed, size);
        // printFreeList();
    }
    return ptr;
}

void coalesce(unsigned long* head, unsigned long* tail) {
    //store old data
    unsigned long tailSize = *tail;

    //add tailSize to head's size
    *(head) += tailSize;

    //make tail's endSize the size of the coalesced block
    unsigned long* tailEndSizePtr = tail + (tailSize / 8) - 1;
    *tailEndSizePtr = *head;

    //make head's next = tail's next
    *(head + 1) = *(tail + 1);
    //now make next's prev equal to head
    unsigned long* next = (unsigned long*)*(head + 1);
    if (next != NULL) {
        *(next + 2) = (unsigned long)head;
    }

    // printf("head next %p\n", (unsigned long*)*(head + 1));

    // printf("tail next %p\n", (unsigned long*)*(tail + 1));
}

void freeHeadandMakeItFirstFree(unsigned long* head) {
    //set head's next to be firstBlock, prev to be 0
    unsigned long* nextPtr = head + 1;
    *nextPtr = (unsigned long)firstBlock;
    unsigned long* prevPtr = head + 2;
    *prevPtr = 0;
    if (firstBlock != NULL) {
        unsigned long* nextPrev = ((unsigned long*)firstBlock) + 2;  //set prev of next to head
        *nextPrev = (unsigned long)head;
    }

    //make firstBlock head
    firstBlock = head;
}

void myfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    char* heapStart = (char*)heap;
    char* heapEnd = heapStart + (1024 * 1024);

    if (((char*)ptr) > heapEnd || ((char*)ptr) < heapStart) {
        printf("error: not a heap pointer");
    }
    unsigned long* head = ((unsigned long*)ptr) - 2;  // if ptr is in heap, then to find it we will need to search using head

    unsigned long* find = heap;
    unsigned long* prevFree = NULL;
    while (find < head) {
        if (!(*find & 0x1)) {  // free block
            prevFree = find;
        }
        find = find + ((*find) / 8);  // add the size at find to move to the next block
    }

    if (find != head) {  // not a malloced address, we cannot find the head using headers
        printf("error: not a malloced address\n");
        return;
    } else if (!(*find & 0x1)) {  // the address is free already -> double free{
        printf("error: double free\n");
    } else if (DEBUG) {
        printf("found head\n");
    }

    //you can free so start by setting unallocated bits

    *head &= ~(1 << 0);
    // printf("freeing head @ %p -> %lu\n", head, *head);
    askedSize -= *(head + 1);  // for utilization() subtracts the amount of bytes asked by the user
    // printf("head (should have unalloc bit now) (%p) -> %lu \n", head, *head);
    unsigned long headSize = *head;
    unsigned long* endSizePtr = (head + (headSize / 8)) - 1;

    *endSizePtr &= ~(1 << 0);
    // printf("end of free block @ %p -> %lu\n", endSizePtr, *endSizePtr);
    //prev free does not exist
    if (prevFree == NULL) {
        // printf("new first free block\n");
        freeHeadandMakeItFirstFree(head);
    } else {  // prev free exists
        //mark prevfree's next block as head
        unsigned long* oldNext = (unsigned long*)*(prevFree + 1);
        *(prevFree + 1) = (unsigned long)head;

        if (oldNext != 0) {
            *(oldNext + 2) = (unsigned long)head;
        }

        //make head's prev equal to prevFree, head's next equal to prevFree's next
        *(head + 1) = (unsigned long)oldNext;
        *(head + 2) = (unsigned long)prevFree;
        //check if you should coalesce (prevFree is continguous)
        unsigned long* endOfPrevFree = prevFree + (*prevFree / 8);
        if (endOfPrevFree == head) {
            coalesce(prevFree, head);
            //now, make head prevFree
            head = prevFree;
        }
    }

    //check if endOfHeadPtr is a free block (contiguous)
    unsigned long* endOfHeadPtr = head + (*head / 8);
    if (endOfHeadPtr < (unsigned long*)endHeap) {
        // printf("endOfHead %p end heap = %p\n", endOfHeadPtr, endHeap);
        if (!(*endOfHeadPtr & 1)) {  //endOfHeadPtr is a free block
            coalesce(head, endOfHeadPtr);
        }
    }
    if (endOfHeadPtr == nextBlock) {  // if next block is coalesed, we want to update the pointer to it
        nextBlock = head;
    }
    // printFreeList();
    // printf("find = %p, head = %p, ptr = %p\n", find, head, ptr);
}

double utilization() {
    if (askedSize == 0) {
        return 1.0;
    }
    unsigned long* lastUsed = heap;
    unsigned long* ptr = heap;
    while (ptr < (unsigned long*)endHeap) {
        ptr = ptr + ((*ptr) / 8);  // move pointer to the end of the current block
        if (*(ptr - 1) & 0x1) {    //check alloc bit, if alloc, we can set lastUsed
            lastUsed = ptr;        //set last use to the end of the block
        }
    }
    printf("last used : %p, heap %p, difference: %lu", lastUsed, heap, ((lastUsed - (unsigned long*)heap)));
    printf("asked size: %f\n ", askedSize);
    return askedSize / (8 * (lastUsed - (unsigned long*)heap));
}

void* myrealloc(void* ptr, size_t size) {
    if(ptr == NULL && size == 0){
        return NULL;
    }
    if(ptr == NULL){
        return mymalloc(size);
    }
    if(size == 0){
        myfree(ptr);
        return NULL;
    }

    unsigned long* startOfBlock = (unsigned long *) ptr - 2;
    size_t oldPayloadSize = (*startOfBlock & -2) - 3;

    //check if next block is free
    unsigned long* nextBlock = startOfBlock + *startOfBlock;

    //resize within the block if possible
    if(oldPayloadSize > size){
        int sizeDifference = oldPayloadSize - size;
        size = size + 24;
        size = findNearestMultipleof8(size);

        //set new size in the block
        *startOfBlock = size;
        *startOfBlock |= 1 << 0;

        //set endsize
        unsigned long* endSize = startOfBlock + (size / 8) - 1;
        *endSize = size;
        *endSize |= 1 << 0;

        //slack ptr, basically, the left over memory
        unsigned long* slackPtr = endSize + 1;
        *slackPtr &= ~(1 << 0);

        //if the next block is free, coalesce
        if(!(*nextBlock & 0x1)){
            //set unallocated bit
            coalesce(slackPtr, nextBlock);
        } else { //next block is allocated, so see what you can do with the slackPtr
            if(sizeDifference > 32){ //you can make it it's own free block
                //find prev free
                unsigned long* find = heap;
                unsigned long* prevFree = NULL;
                while (find < slackPtr) {
                    if (!(*find & 0x1)) {  // free block
                        prevFree = find;
                    }
                    find = find + ((*find) / 8);  // add the size at find to move to the next block
                }
                if(prevFree == NULL){ //what to do here?

                }

                //save prevFree's next
                unsigned long oldNext = *(prevFree + 1);

                //set up slackPtr to be a free block
                *slackPtr = sizeDifference;
                unsigned long* nextPtr = slackPtr + 1;
                *nextPtr = oldNext;
                unsigned long* prevPtr = slackPtr + 2;
                *prevPtr = (unsigned long) prevFree;
                unsigned long* endSize = slackPtr + (*slackPtr / 8) - 1;
                *endSize = sizeDifference;
                *endSize &= ~(1 << 0);

                //set prevFree's next to be slackPtr 
                unsigned long* prevFreeNext = prevFree + 1;
                *prevFree = (unsigned long) slackPtr;

                //set nextFree's prev to be slackPtr
                unsigned long* nextFree = (unsigned long*) oldNext;
                unsigned long* nextFreePrev = nextFree + 2;
                *nextFreePrev = (unsigned long) slackPtr;
            }
        }
        return startOfBlock; 
    } else { //find a new block
        unsigned long* newPtr = mymalloc(size);
        if(newPtr != NULL){
            //copy over data
            unsigned long* newPayload = newPtr + 2;
            unsigned long* oldPayload = (unsigned long*) ptr + 2;
            unsigned long* endOfPayload = ptr + ((*ptr & -2) / 8) - 1;

            while(oldPayload < endOfPayload){
                *newPayload = *oldPayload;
                oldPayload += 1;
                newPayload += 1;
            }
        } else {
            return NULL;
        }

        //free old ptr
        myfree(ptr);
        return newPtr;
    }
    return NULL;
}

// int main() {
//     myinit(2);
//     unsigned long* ptr = (unsigned long*)mymalloc(1048);
//     printf("ptr malloc bytes : %p, asked: %lu, given: %lu\n", ptr, *(ptr - 1), *(ptr - 2));
//     // printf("first block: (%p) - > %lu firstBlock next: %p \n", firstBlock, *((unsigned long*)firstBlock), (unsigned long*)*(((unsigned long*)firstBlock) + 1));
//     unsigned long* ptr2 = (unsigned long*)mymalloc(5);
//     printf("ptr2 malloc: %p, asked: %lu, given: %lu\n", ptr2, *(ptr2 - 1), *(ptr2 - 2));
//     // printf("first block: (%p) - > %lu firstBlock next: %p \n", firstBlock, *((unsigned long*)firstBlock), (unsigned long*)*(((unsigned long*)firstBlock) + 1));
//     myfree(ptr2);
//     // printf("frist block(%p) - > %lu firstBlock next: %p \n", firstBlock, *((unsigned long*)firstBlock), (unsigned long*)*(((unsigned long*)firstBlock) + 1));
//     unsigned long* ptr3 = (unsigned long*)mymalloc(5);
//     printf("ptr3 malloc: %p, asked: %lu, given: %lu\n", ptr3, *(ptr3 - 1), *(ptr3 - 2));
//     myfree(ptr);
//     // printf("first block(%p) - > %lu firstBlock next: %lu \n", firstBlock, *((unsigned long*)firstBlock), *(((unsigned long*)firstBlock) + 1));
//     unsigned long* ptr4 = (unsigned long*)mymalloc(1040);
//     if (ptr4 == ptr) {
//         printf("fuck yes baby\n");
//     } else {
//         printf("ptr4: %p, ptr: %p", ptr4, ptr);
//     }
//     if (ptr == NULL || ptr2 == NULL) {
//         printf("fuck\n");
//     } else {
//         printf("making progress\n");
//     }
//     free(heap);
// }
