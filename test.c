#include <stdlib.h>
#include <stdio.h>

int main(){
    int size = 3;
    size += (8 - (size % 8));
    printf("%d\n", size);
}