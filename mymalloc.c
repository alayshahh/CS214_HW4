#include <stdio.h>
#include <stdlib.h>
#include "constants.h"

void *heap;
void *firstFree;          //contains address of the first free block
unsigned long nextBlock;  //for next fit algo

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
    unsigned long *point = (unsigned long *)heap;
    *point = BEGINNING_FREE_SPACE;
        
    unsigned long* next = (unsigned long *) (point + 1);
    *next = 69;

    unsigned long* prev = (unsigned long *) (next + 1);
    *prev = 420;
    
    unsigned long *point_2 = ((unsigned long *)heap) + 131071;  // (1MB - 8 bytes)/ 8 bytes
    *point_2 = BEGINNING_FREE_SPACE;

    allocAlg = allocArg;
    firstFree = heap;
}

void mycleanup() {
    free(heap);
}

size_t findNearestMultipleof8(size_t size){
    return size += (8 - (size % 8));
}

void* mymalloc(size_t size) {
    size = size + 16;  // [size , payload, size]
    size = findNearestMultipleof8(size);

    if (allocAlg == FIRST_FIT) {
        
        //iterate to find the first fit
        unsigned long* ptr = (unsigned long *)firstFree;
        while (ptr != 0 && *(ptr) <= size) {
            ptr = (unsigned long *) *(ptr + 1);
        }
        if(ptr == 0) return NULL; //reached end of list, not enough space

        //save old data
        size_t oldSize = *ptr;
        unsigned long next = *(ptr + 1);
        unsigned long prev = *(ptr + 2);

        printf("oldSize: %lu\n", oldSize);

        //there is enough space, but split
        *ptr = size;
        *ptr |= 1 << 0; //set allocated bit
        
        //set second size
        char* secondSize = ((char*)ptr + size) - 1;
        *secondSize = size;

        //set split block
        unsigned long* block = (unsigned long*) (secondSize + 1);
        *block = oldSize - size;
        *(block + 1) = (unsigned long) next;
        *(block + 2) = (unsigned long) prev;
        unsigned long* endSize = (unsigned long*) ((char*) ptr + oldSize - 1);
        *endSize = oldSize - size;

        printf("size rem: %lu\n", *block);
        printf("next: %lu\n", *(block + 1));
        printf("prev: %lu\n", *(block + 2));
        printf("endSize: %lu\n", *endSize);
        return ptr;
    }

    return NULL;
}

int main() {
    myinit(0);
    printf(" starting at : %p\n", heap);
    void* ptr = mymalloc(5);
    if(ptr == NULL){
        printf("fuck\n");
    } else {
        printf("making progress\n");
    }
}