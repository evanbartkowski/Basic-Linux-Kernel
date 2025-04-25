#ifndef DEQUE_H
#define DEQUE_H

#include <pthread.h>
#include <semaphore.h>

#define DATA_LENGTH 1024 // Size of each data block in bytes

// Priority Levels
#define HIGH_PRIORITY 0
#define MEDIUM_PRIORITY 1
#define LOW_PRIORITY 2

// Node Structure for the Deque
typedef struct node
{
    struct node *next;
    struct node *prev;
    char data[DATA_LENGTH];
} node_t;

// Deque Structure to Support Multiple Priority Levels
typedef struct deque_421
{
    node_t *high_head;   // Head of the high-priority queue
    node_t *high_tail;   // Tail of the high-priority queue
    node_t *medium_head; // Head of the medium-priority queue
    node_t *medium_tail; // Tail of the medium-priority queue
    node_t *low_head;    // Head of the low-priority queue
    node_t *low_tail;    // Tail of the low-priority queue
    int high_count;      // Count of high-priority elements
    int medium_count;    // Count of medium-priority elements
    int low_count;       // Count of low-priority elements
} deque_421_t;

// Global Variables for Deque and Synchronization
extern deque_421_t *deque;
extern sem_t sem_high;              // Semaphore to track high-priority slots
extern sem_t sem_medium;            // Semaphore to track medium-priority slots
extern sem_t sem_low;               // Semaphore to track low-priority slots
extern pthread_mutex_t deque_mutex; // Mutex for deque access

// Function Prototypes
int init_deque(void);
int enqueue_front(char *data, int priority);
int enqueue_rear(char *data, int priority);
int dequeue_front(char *data, int *priority);
int dequeue_rear(char *data, int *priority);
int delete_deque(void);

#endif
