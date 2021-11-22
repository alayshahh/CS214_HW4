#include <stdio.h>
#include <stdlib.h>

#include "constants.h"

void* heap;
void* firstBlock;  //contains address of the first free block
void* nextBlock;   //for next fit algo
size_t askedSize = 0;
int allocAlg = -1;

/*
design of free block            design of malloced block
   | size_t     |                    | size_t  |
   | next       |                    | ...     | 
   | prev       |                    | payload |
   | free space |                    | ...     | 
   | size_t     |                    | size_t  |

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
    unsigned long* point = (unsigned long*)heap;
    *point = BEGINNING_FREE_SPACE;
    *point |= 0 << 0;  //set unallocated bit
    // printf("Point1=%p\n", (void*)point);
    // printf("value at point  = %zu\n", *point);

    unsigned long* next = (unsigned long*)(point + 1);
    *next = 0;

    unsigned long* prev = (unsigned long*)(next + 1);
    *prev = 0;
    // printf("next =%p\n", (void*)next);
    // printf("prev =%p\n", (void*)prev);
    unsigned long* point_2 = ((unsigned long*)heap) + 131071;  // (1MB - 8 bytes)/ 8 bytes
    *point_2 = BEGINNING_FREE_SPACE;
    *point_2 |= 0 << 0;  //set unallocated bit
    // printf("Point2=%p\n", (void*)point_2);
    // printf("diff bw pointers: %ld \n", ((char*)point_2) - ((char*)point));

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

void* setMallocBlock(unsigned long* ptr, size_t size) {
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
    // printf("pointer at: %p, value at pointer %lu \n", ptr, *ptr);
    // printf("*ptr (%lu) - size (%lu) = %lu bytes \n", *ptr, size, (*ptr - size));
    if (((*ptr) - size) < 32) {  // dont split
        // printf("dont split");
        unsigned long* endSize = (ptr + (*ptr / 8)) - 1;

        *endSize |= 1 << 0;  // set allocated bit at end
        *ptr |= 1 << 0;      //set allocated bit in front
        nextBlock = (void*)next;
        if (next != 0) {
            *((unsigned long*)next + 2) = prev;
        }
        if (prev != 0) {
            *((unsigned long*)prev + 1) = next;
        } else
            firstBlock = (void*)next;
        // set head of freeBlock
        // printf("malloced starts at: %p\n", ptr);
        // printf("payload at %p\n", ptr + 1);
        // printf("size of malloc: %lu\n", *ptr);
        // printf("next: %lu\n", *(ptr + 1));
        // printf("prev: %lu\n", *(ptr + 2));
        // printf("endSize: %lu\n", *endSize);
        // printf("ends at: %p\n", endSize);

        return (void*)(ptr + 1);
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
    *block |= 0 << 0;  //set unallocated bit
    *(block + 1) = next;
    *(block + 2) = prev;
    if (prev == 0) {
        firstBlock = (void*)block;
    }
    nextBlock = (void*)block;
    unsigned long* endSize = (ptr + (oldSize / 8) - 1);
    *endSize = oldSize - size;
    *endSize |= 0 << 0;  //set unallocated bit
    // printf("malloc starts at (%p) -> size: %lu \n", (void*)ptr, *ptr);
    // printf("payload at %p\n", ptr + 1);
    // printf("end of malloc block at (%p) -> size: %lu\n", (void*)secondSize, *secondSize);
    // printf("split block starts at %p -> value %lu \n", block, *block);
    // printf("split block ends at (%p) -> value %lu\n", endSize, *endSize);
    // printf("next free from split: %lu\n", *(block + 1));
    // printf("prev free from split: %lu\n", *(block + 2));

    return (void*)(ptr + 1);
}

void* findFit(void* start, size_t size) {
    /*
    finds the next fit beginning from the start pointer 

    */
    if (start == NULL) {
        return NULL;
    }
    unsigned long* ptr = (unsigned long*)start;
    while (ptr != 0 && *(ptr) < size) {
        ptr = (unsigned long*)*(ptr + 1);
    }
    if (ptr == 0 || *ptr < size) return NULL;
    return setMallocBlock(ptr, size);  //reached end of list, not enough space
}

void* findBestFit(size_t size) {
    /*
    finds the smallest possible block that fits the size needed
    */
    unsigned long* ptr = firstBlock;
    unsigned long* bestFit = NULL;
    while (ptr != 0) {
        if (*ptr >= size) {
            if (bestFit == NULL) {
                bestFit = ptr;
            } else if ((((*ptr) - size) < (*bestFit) - size)) {
                bestFit = ptr;
            }
        }
        ptr = (unsigned long*)*(ptr + 1);
    }
    return setMallocBlock(bestFit, size);
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
    size = size + 16;  // [size , payload, size]
    size = findNearestMultipleof8(size);
    // printf("Size = %lu\n", size);
    void* ptr = NULL;
    if (allocAlg == FIRST_FIT) {
        ptr = findFit(firstBlock, size);
    } else if (allocAlg == NEXT_FIT) {
        // printf("next fit");
        // if (nextBlock == NULL) printf("NEXTBLOCK = NULL\n");
        ptr = findFit(nextBlock, size);  //start from next free block from allocated block
        if (ptr == NULL) {               //if we dont find anything, try from the beginning
            ptr = findFit(firstBlock, size);
        }
    } else if (allocAlg == BEST_FIT) {  //best fit
        ptr = findBestFit(size);
    }
    if (ptr != NULL) {
        askedSize += mallocAsk;
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
}

void freeHeadandMakeItFirstFree(unsigned long* head) {
    //set head's next to be firstBlock, prev to be 0
    unsigned long* nextPtr = head + 1;
    *nextPtr = (unsigned long)firstBlock;
    unsigned long* prevPtr = head + 2;
    *prevPtr = 0;
    unsigned long* nextPrev = ((unsigned long*)firstBlock) + 1;  //set prev of next to head
    *nextPrev = (unsigned long)head;
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
    unsigned long* head = ((unsigned long*)ptr) - 1;  // if ptr is in heap, then to find it we will need to search using head

    unsigned long* find = heap;
    unsigned long* prevFree = NULL;
    while (find < head) {
        if (!(*find & 0x1)) {  // free block
            prevFree = find;
        }
        find = find + ((*find) / 8);  // add the size at find to move to the next block
    }

    if (prevFree != NULL) {
        if (((head > prevFree) && (head < (prevFree + ((*prevFree) / 8))))) {  // address is apart of a free block then we will condiser it a double free
            printf("error: double free\n");
        }
    } else if ((!(*find & 0x1) && find == head)) {  // the address is free already -> double free
        printf("error: double free\n");
        return;
    } else if (find != head) {  // the address is not malloced
        printf("error: not a malloced address\n");
        return;
    }

    //you can free so start by setting unallocated bits
    *head &= ~(1 << 0);
    // printf("head (should have unalloc bit now) (%p) -> %lu \n", head, *head);
    unsigned long headSize = *head;
    unsigned long* endSizePtr = (head + (headSize / 8)) - 1;
    *endSizePtr &= ~(1 << 0);

    //prev free does not exist
    if (prevFree == NULL) {
        // printf("new first free block\n");
        freeHeadandMakeItFirstFree(head);
    } else {  // prev free exists
        //mark prevfree's next block as head
        unsigned long oldNext = *(prevFree + 1);
        *(prevFree + 1) = (unsigned long)head;

        //make head's prev equal to prevFree, head's next equal to prevFree's next
        *(head + 1) = oldNext;
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
    if (!(*endOfHeadPtr & 1)) {  //endOfHeadPtr is a free block
        coalesce(head, endOfHeadPtr);
    }

    // printf("find = %p, head = %p, ptr = %p\n", find, head, ptr);
}

int main() {
    myinit(0);
    printf(" starting at : %p\n", heap);
    void* ptr = mymalloc(1048);
    printf(" pointer : %p\n", ptr);
    printf("firstBlock = %p\n", firstBlock);

    unsigned long* ptr2 = (unsigned long*)mymalloc(5);
    printf("pointer 2 : %p\n", ptr2);
    printf("firstBlock = %p\n", firstBlock);
    myfree(ptr2);
    printf("(%p) - > %lu\n", firstBlock, *((unsigned long*)firstBlock));
    unsigned long* ptr3 = (unsigned long*)mymalloc(5);
    printf("ptr 3 :%p -> %lu \n", ptr3, *ptr3);
    myfree(ptr);
    printf("(%p) - > %lu\n", firstBlock, *((unsigned long*)firstBlock));
    if (ptr == NULL || ptr2 == NULL) {
        printf("fuck\n");
    } else {
        printf("making progress\n");
    }
}
