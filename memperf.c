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
} List;
void addPointer(List* pointers, Node* new) {
    new->next = pointers->head;
    pointers->head = new;
}

int sizeList(List* pointers) {
    Node* ptr = pointers->head;
    int size = 0;
    while (ptr != NULL) {
        ptr = ptr->next;
        size++;
    }
    return size;
}
void deleteNode(List* pointers, Node* node) {
    if (pointers->head == node) {
        pointers->head = pointers->head->next;
        free(node);
        return;
    }
    Node* ptr = pointers->head;

    while (ptr->next != node && ptr->next != NULL) {
        ptr = ptr->next;
    }
    if (ptr->next == NULL) {
        printf("node not found\n");
    } else {
        ptr->next = node->next;
        free(node);
    }
}
void freeList(List* pointers) {
    while (pointers->head != NULL) {
        Node* ptr = pointers->head;
        pointers->head = pointers->head->next;
        free(ptr);
    }
}

Node* getRandomNode(List* pointer) {
    int i = rand() % sizeList(pointer);
    Node* random = pointer->head;
    for (; i > 0; i--) {
        random = random->next;
    }
    // printf("random node: %p\n", random);
    return random;
}

Node* createNode(void* node) {
    Node* new = (Node*)malloc(sizeof(Node));
    new->node = node;
    new->next = NULL;
    return new;
}
int OPERATIONS = 10000000;

void performance(int allocAlg) {
    myinit(allocAlg);
    List pointers;
    pointers.head = NULL;
    time_t begin;
    time_t end;
    time(&begin);
    int num_free = 0;
    int num_malloc = 0;
    int num_realloc = 0;
    for (int i = 0; i < OPERATIONS; i++) {
        /*
            if head is not null, then just rnad b/w 0-2 (malloc, free, realloc)
            otherwise malloc 
            if malloc size = rand b/w 0-256 bytes (null check) & create node, add node
            if realloc -> pick random node, realloc w size b/w 0-256 (null check), edit or delete old node
            if free -> pick random node, myfree, delete node if all is good in the hood
            
        */
        // printf("******** NEW REQUEST *********\n");
        int op = rand() % 3;
        if (pointers.head == NULL) {
            op = 0;
        }
        if (op == 0) {
            num_malloc++;
            void* ptr = mymalloc((size_t)(rand() % 256) + 1);
            if (ptr == NULL) {
                // printf("error: unable to malloc\n");
                ;
            } else {
                Node* new = createNode(ptr);
                addPointer(&pointers, new);
            }
        } else if (op == 1) {
            num_free++;
            Node* node = getRandomNode(&pointers);
            myfree(node->node);
            deleteNode(&pointers, node);
        } else if (op == 2) {
            num_realloc++;
            Node* node = getRandomNode(&pointers);
            node->node = myrealloc(node->node, (size_t)(rand() % 257));
            if (node->node == NULL) {
                deleteNode(&pointers, node);
            }
        }
    }
    time(&end);
    time_t timePass = end - begin;
    double opRate = OPERATIONS / (double)timePass;
    double util = utilization();
    switch (allocAlg) {
        case FIRST_FIT:
            printf("First fit throughput: %f ops/sec\nFirst fit utilization: %f\n \tmalloc: %d\n\t free: %d\n\trealloc: %d\n", opRate, util, num_malloc, num_free, num_realloc);
            break;
        case NEXT_FIT:
            printf("Next fit throughput: %f ops/sec\nNext fit utilization: %f\n\tmalloc: %d\n\t free: %d\n\trealloc: %d\n", opRate, util, num_malloc, num_free, num_realloc);
            break;
        case BEST_FIT:
            printf("Best fit throughput: %f ops/sec\nBest fit utilization: %f\n\tmalloc: %d\n\t free: %d\n\trealloc: %d\n", opRate, util, num_malloc, num_free, num_realloc);
            break;
        default:
            break;
    }
    mycleanup();
    freeList(&pointers);
}

int main(int argc, char** argv) {
    srand(time(NULL));
    // performance(0);
    performance(1);
    // performance(2);
}
