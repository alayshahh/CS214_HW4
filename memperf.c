#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "constants.h"
#include "mymalloc.h"

typedef struct MemPerf {
    void* node;
    struct MemPerf* next;
} Node;

typedef struct List {
    Node* head;
    int numNodes;
} List;
void addPointer(List* pointers, Node* new) {
    new->next = pointers->head->next;
    pointers->head = new;
    pointers->numNodes = pointers->numNodes + 1;
}

Node* getRandomNode(List* pointer) {
    int i = rand() % pointer->numNodes;
    Node* random = pointer->head;
    for (; i > 0; i--) {
        random = random->next;
    }
    return random;
}

void deleteNode(List* pointers, Node* node) {
    Node* ptr = pointers->head;
    if (ptr == node) {
        pointers->head = pointers->head->next;
        free(node);
        return;
    }
    while (ptr->next != node && ptr->next != NULL) {
        ptr = ptr->next;
    }
    if (ptr->next == NULL) {
        printf("node not found\n");
    } else {
        ptr->next = ptr->next->next;
        free(ptr->next);
    }
}

Node* createNode(void* node) {
    Node* new = (Node*)malloc(sizeof(Node));
    new->node = node;
    new->next = NULL;
    return new;
}
int OPERATIONS = 100;

void performance(int allocAlg) {
    myinit(allocAlg);
    List pointers;
    pointers.numNodes = 0;
    pointers.head = NULL;
    time_t begin;
    time_t end;
    time(&begin);
    for (int i = 0; i < OPERATIONS; i++) {
        /*
            if head is not null, then just rnad b/w 0-2 (malloc, free, realloc)
            otherwise malloc 
            if malloc size = rand b/w 0-256 bytes (null check) & create node, add node
            if realloc -> pick random node, realloc w size b/w 0-256 (null check), edit or delete old node
            if free -> pick random node, myfree, delete node if all is good in the hood
            
        */
    }
    sleep(10);
    time(&end);
    time_t timePass = end - begin;
    double opRate = OPERATIONS / (double)timePass;
    double util = utilization();
    switch (allocAlg) {
        case FIRST_FIT:
            printf("First fit throughput: %.2f ops/sec\nFirst fit utilization: %.2f\n", opRate, util);
            break;
        case NEXT_FIT:
            printf("Next fit throughput: %.2f ops/sec\nNext fit utilization: %.2f\n", opRate, util);
            break;
        case BEST_FIT:
            printf("Best fit throughput: %.2f ops/sec\nBest fit utilization: %.2f\n", opRate, util);
            break;
        default:
            break;
    }
    mycleanup();
}

int main(int argc, char** argv) {
    srand(time(NULL));
    performance(0);
    performance(1);
    performance(2);
}
