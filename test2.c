#include <stdio.h>

#include "mymalloc.h"

int main(int argc, char** argv) {
    myinit(0);
    void* ptr = mymalloc(12043);
    printf("before free, asked for 12043 bytes, util: %f, used %f bytes\n", utilization(), 12043 / utilization());
    myfree(ptr);
    printf("util: %f", utilization());
    mycleanup();
}