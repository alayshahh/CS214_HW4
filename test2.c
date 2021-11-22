#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    char* ptr_1 = malloc(sizeof(char) * 25);
    printf("%p\n", ptr_1);

    char* ptr_2 = malloc(sizeof(char) * 25);
    printf("%p\n", ptr_2);
    free(ptr_1);
    free(ptr_2);
    free(ptr_1 + 1);
    // free(ptr_2);
}