#include <stdio.h>
#include <stdlib.h>

#include "constants.h"
void *heap;
void *endPtr;
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
    // printf("starting address=%p\n", heap);
    size_t *point = (size_t *)heap;
    // printf("heap = %p\n", heap);
    *point = BEGINNING_FREE_SPACE;  // 1024^2 - 16 bytes( size_t)
    // printf("Point1=%p\n", (void *)point);
    // printf("value at point  = %zu\n", *point);
    int *next = (int *)(point + 1);
    *next = 0;
    unsigned long *prev = (unsigned long *)point + 2;
    *prev = 0;
    // printf("next =%p\n", (void *)next);
    // printf("prev =%p\n", (void *)prev);
    size_t *point_2 = ((size_t *)heap) + 131071;  // (1MB - 8 bytes)/ 8 bytes
    // printf("Point2=%p\n", (void *)point_2);
    *point_2 = BEGINNING_FREE_SPACE;
    // printf("diff bw pointers: %ld \n", ((char *)point_2) - ((char *)point));
    allocAlg = allocArg;
    firstFree = heap;
}

void mycleanup() {
    free(heap);
}

void mymalloc(size_t size) {
    size = size + 16;  // [size , payload, size]
    if (size < 32) {   //big enough for when we free to store that metadata [size, next, prev, size]
        size = 32;
    }
    if (allocAlg == FIRST_FIT) {
    firstFit:
        unsigned long *ptr = (unsigned long *)firstFree;
        while (*(ptr + 1) != 0 && *(ptr) <= size) {
            ptr = *(ptr + 1);
        }
        if (*(ptr + 1) == 0 && *(ptr) <= size) {
            return null;  //not enough space left
        }

    } else if (allocAlg == NEXT_FIT) {
        unsigned long *ptr = (unsigned long *)firstFree;
        while (*(ptr + 1) != 0 && *(ptr) <= size) {
            ptr = *(ptr + 1);
        }
        if (*(ptr) < size) {  //if we dotn find it where we start, then use the first fit algo (jump statement lol)
            goto firstFit;
        }
    } else {  //bestfit algo
    }
}

int main() {
    myinit(0);
    mycleanup();
    return 0;
}