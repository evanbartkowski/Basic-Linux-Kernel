#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "deque.h"

//Change these to test different amounts of programs
#define NUM_PRODUCERS 100
#define NUM_CONSUMERS 100

/////////////////////////////////////////////////////////////////////////////
// Convert prio to string (for printing priority)
/////////////////////////////////////////////////////////////////////////////
const char* priority_to_string(int priority) {
    switch (priority) {
        case 0: return "High";
        case 1: return "Medium";
        case 2: return "Low";
        default: return "Unknown";}}
/////////////////////////////////////////////////////////////////////////////
// Producer function
/////////////////////////////////////////////////////////////////////////////
void* producer(void* arg) {
    int priority = *((int*)arg);
    char data[DATA_LENGTH];

    // Generate data based on priority
    snprintf(data, DATA_LENGTH, "Data Block %d (%s)", rand() % 10, priority_to_string(priority));

    // Enqueue data at the front with the priority
    if (enqueue_front(data, priority) == 0) {
        printf("Producer enqueued: %s\n", data);}
    else{
        printf("Producer failed to enqueue data.\n");}
    free(arg); // Free the allocated priority memory
    return NULL;}
/////////////////////////////////////////////////////////////////////////////
// Consumer thread function
/////////////////////////////////////////////////////////////////////////////
void* consumer(void* arg) {
    char data[DATA_LENGTH];
    int priority;

    // Dequeue from the front
    if (dequeue_front(data, &priority) == 0) { // Modify dequeue to return priority
        printf("Consumer dequeued: %s (Priority: %d)\n", data, priority);}
    else{
        printf("Consumer failed to dequeue data.\n");}
    return NULL;}
/////////////////////////////////////////////////////////////////////////////
int main(){
    pthread_t producer_threads[NUM_PRODUCERS];
    pthread_t consumer_threads[NUM_CONSUMERS];

    // Initialize the deque
    if (init_deque() != 0) {
        fprintf(stderr, "Failed to initialize deque.\n");
        return EXIT_FAILURE;}

    // for randomness
    srand(time(NULL));
    // Create producer threads with random priorities
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        int* priority = malloc(sizeof(int));
        *priority = rand() % 3; // Random priority of 0,1,2
        pthread_create(&producer_threads[i], NULL, producer, priority);}

    // Creates consumer threads
    for (int i = 0; i < NUM_CONSUMERS; i++){
        pthread_create(&consumer_threads[i], NULL, consumer, NULL);}

    // Join producer threads
    for (int i = 0; i < NUM_PRODUCERS; i++){
        pthread_join(producer_threads[i], NULL);}

    // Join consumer threads
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(consumer_threads[i], NULL);}

    // Cleans dynamic memory
    delete_deque();
    return EXIT_SUCCESS;}
////////////////////////////////////////////////////////////////////////////
