#include <stdlib.h>

volatile void* heap;
int allocAlg = -1;

void myinit(int allocArg) {
    heap = malloc(1024 * 1024);
    allocAlg = allocArg;
}

void mycleanup() {
    free(heap);
}