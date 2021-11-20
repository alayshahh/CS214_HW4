#include <stdio.h>
#include <stdlib.h>

#include "constants.h"

void* heap;
void* firstFree;          //contains address of the first free block
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
    unsigned long* point = (unsigned long*)heap;
    *point = BEGINNING_FREE_SPACE;
    *point |= 0 << 0;  //set unallocated bit

    unsigned long* next = (unsigned long*)(point + 1);
    *next = 0;

    unsigned long* prev = (unsigned long*)(next + 1);
    *prev = 0;

    unsigned long* point_2 = ((unsigned long*)heap) + 131071;  // (1MB - 8 bytes)/ 8 bytes
    *point_2 = BEGINNING_FREE_SPACE;
    *point_2 |= 0 << 0;  //set unallocated bit

    allocAlg = allocArg;
    firstFree = heap;
}

void mycleanup() {
    free(heap);
}

size_t findNearestMultipleof8(size_t size) {
    size += (8 - (size % 8));
    // we want the malloced block to be atleast 32 bytes so when we free, it will be big enough to store all meta data
    return size > 32 ? size : 32;
}

void* mymalloc(size_t size) {
    size = size + 16;  // [size , payload, size]
    size = findNearestMultipleof8(size);

    if (allocAlg == FIRST_FIT) {
        //iterate to find the first fit
        unsigned long* ptr = (unsigned long*)firstFree;
        while (ptr != 0 && *(ptr) < size) {
            ptr = (unsigned long*)*(ptr + 1);
        }
        if (ptr == 0) return NULL;  //reached end of list, not enough space

        printf("%p\n", ptr);
        //save old data
        size_t oldSize = *ptr & -2;
        unsigned long next = *(ptr + 1);
        unsigned long prev = *(ptr + 2);

        //there is enough space
        //do we split?
        printf("pointer at: %p, value at pointer %lu \n", ptr, *ptr);
        printf("*ptr (%lu) - size (%lu) = %lu bytes \n", *ptr, size, (*ptr - size));
        if (((*ptr) - size) < 32) {  // dont split
            printf("dont split");
            unsigned long* endSize = (ptr + (*ptr / 8)) - 1;
            *endSize |= 1 << 0;  // set allocated bit at end
            *ptr |= 1 << 0;      //set allocated bit in front
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
        } else {
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
            unsigned long* endSize = (ptr + (oldSize / 8) - 1);
            *endSize = oldSize - size;
            *endSize |= 0 << 0;  //set unallocated bit
            printf("starts at: %p\n", block);
            printf("size rem: %lu\n", *block);
            printf("next: %lu\n", *(block + 1));
            printf("prev: %lu\n", *(block + 2));
            printf("endSize: %lu\n", *endSize);
            printf("ends at: %p\n", endSize);
        }

        return ptr;
    }

    return NULL;
}

int main() {
    myinit(0);
    printf(" starting at : %p\n", heap);
    void* ptr = mymalloc(1048529);
    if (ptr == NULL) {
        printf("fuck\n");
    } else {
        printf("making progress\n");
    }
}

// void* findFit(void* start, size_t size) {
// }