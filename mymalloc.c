#include <stdio.h>
#include <stdlib.h>

#include "constants.h"

void* heap;
void* firstFree;  //contains address of the first free block
void* nextBlock;  //for next fit algo
size_t askedSize = 0;

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
int allocAlg = -1;

void myinit(int allocArg) {
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
    firstFree = heap;
    nextBlock = heap;
}

void mycleanup() {
    free(heap);
}
void* setMallocBlock(unsigned long* ptr, size_t size) {
    // printf("%p\n", ptr);
    //save old data
    size_t oldSize = *ptr & -2;
    unsigned long next = *(ptr + 1);
    unsigned long prev = *(ptr + 2);

    //there is enough space
    //do we split?
    printf("pointer at: %p, value at pointer %lu \n", ptr, *ptr);
    printf("*ptr (%lu) - size (%lu) = %lu bytes \n", *ptr, size, (*ptr - size));
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
            firstFree = (void*)next;
        //set head of freeBlock
        printf("malloced starts at: %p\n", ptr);
        printf("size of malloc: %lu\n", *ptr);
        printf("next: %lu\n", *(ptr + 1));
        printf("prev: %lu\n", *(ptr + 2));
        printf("endSize: %lu\n", *endSize);
        printf("ends at: %p\n", endSize);
        return ptr;
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
        firstFree = (void*)block;
    }
    nextBlock = (void*)block;
    unsigned long* endSize = (ptr + (oldSize / 8) - 1);
    *endSize = oldSize - size;
    *endSize |= 0 << 0;  //set unallocated bit
    printf("malloc starts at (%p) -> size: %lu \n", (void*)ptr, *ptr);
    printf("end of malloc block at (%p) -> size: %lu\n", (void*)secondSize, *secondSize);
    printf("split block starts at %p -> value %lu \n", block, *block);
    printf("split block ends at (%p) -> value %lu\n", endSize, *endSize);
    printf("next free from split: %lu\n", *(block + 1));
    printf("prev free from split: %lu\n", *(block + 2));

    return (void*)ptr;
}
void* findFit(void* start, size_t size) {
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
    unsigned long* ptr = firstFree;
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
    if (size == 0) {
        return NULL;
    }
    size_t mallocAsk = size;
    size = size + 16;  // [size , payload, size]
    size = findNearestMultipleof8(size);
    // printf("Size = %lu\n", size);
    void* ptr = NULL;
    if (allocAlg == FIRST_FIT) {
        ptr = findFit(firstFree, size);
    } else if (allocAlg == NEXT_FIT) {
        // printf("next fit");
        // if (nextBlock == NULL) printf("NEXTBLOCK = NULL\n");
        ptr = findFit(nextBlock, size);  //start from next free block from allocated block
        if (ptr == NULL) {               //if we dont find anything, try from the beginning
            ptr = findFit(firstFree, size);
        }
    } else if (allocAlg == BEST_FIT) {  //best fit
        ptr = findBestFit(size);
    }
    if (ptr != NULL) {
        askedSize += mallocAsk;
    }
    return ptr;
}

int main() {
    myinit(2);
    printf(" starting at : %p\n", heap);
    void* ptr = mymalloc(1048);
    printf(" pointer : %p\n", ptr);
    printf("firstFree = %p\n", firstFree);

    void* ptr2 = mymalloc(5);
    printf(" pointer 2 : %p\n", ptr2);
    printf("firstFree = %p\n", firstFree);
    if (ptr == NULL || ptr2 == NULL) {
        printf("fuck\n");
    } else {
        printf("making progress\n");
    }
}
