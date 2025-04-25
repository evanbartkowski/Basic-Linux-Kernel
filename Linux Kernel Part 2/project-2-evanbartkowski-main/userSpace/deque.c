// deque.c
// evan bartkowski
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "deque.h"

// Global Variables for ( Deque and Synchronization )
deque_421_t *deque = NULL;
sem_t sem_high;
sem_t sem_medium;
sem_t sem_low;
pthread_mutex_t deque_mutex = PTHREAD_MUTEX_INITIALIZER;

////////////////////////////////////////////////////////////////////////
// Function: init_deque
// Initializes deque with three separate priority double sided queues.
////////////////////////////////////////////////////////////////////////
int init_deque(void) {
    deque = (deque_421_t *)malloc(sizeof(deque_421_t));
    if (deque == NULL) {
        printf("ERROR memory allocation failure for deque");
        return -10000;}  //mem failure

    // Initialize queues, with 3 different types
    deque->high_head = deque->high_tail = NULL;
    deque->medium_head = deque->medium_tail = NULL;
    deque->low_head = deque->low_tail = NULL;

    deque->high_count = 0;
    deque->medium_count = 0;
    deque->low_count = 0;

    // Initialize semaphores
    sem_init(&sem_high, 0, 0);
    sem_init(&sem_medium, 0, 0);
    sem_init(&sem_low, 0, 0);

    return 0;}
/////////////////////////////////////////////////////////////////////////////
// Function: enqueue_front
// Adds a data block to the front of  priority queue.
////////////////////////////////////////////////////////////////////////////
int enqueue_front(char *data, int priority) {
    pthread_mutex_lock(&deque_mutex);

    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    if (new_node == NULL){
        pthread_mutex_unlock(&deque_mutex);
        return -1;}
    strncpy(new_node->data, data, DATA_LENGTH - 1);
    new_node->data[DATA_LENGTH - 1] = '\0';
    new_node->prev = NULL;

    if (priority == HIGH_PRIORITY) {
        new_node->next = deque->high_head;
        if (deque->high_head == NULL) {
            deque->high_tail = new_node;}
	else{
            deque->high_head->prev = new_node;}
        deque->high_head = new_node;
        deque->high_count++;
        sem_post(&sem_high);}
    else if (priority == MEDIUM_PRIORITY) {
        new_node->next = deque->medium_head;
        if (deque->medium_head == NULL) {
            deque->medium_tail = new_node;}
	else{
            deque->medium_head->prev = new_node;}
        deque->medium_head = new_node;
        deque->medium_count++;
        sem_post(&sem_medium);
    } else if (priority == LOW_PRIORITY) {
        new_node->next = deque->low_head;
        if (deque->low_head == NULL) {
            deque->low_tail = new_node;}
	else {
            deque->low_head->prev = new_node;}
        deque->low_head = new_node;
        deque->low_count++;
        sem_post(&sem_low);}
    pthread_mutex_unlock(&deque_mutex);
    return 0;}
////////////////////////////////////////////////////////////////////////////
// Function: dequeue_front
// Removes data block from the front of the high prio queue
////////////////////////////////////////////////////////////////////////////
int dequeue_front(char *data, int *priority) { //priority as pointer
    pthread_mutex_lock(&deque_mutex);
    node_t *node = NULL;
    //DIFFERENT PRIORITY LEVELS HIGH,MEDIUM,LOW
    if (deque->high_count > 0) {
        sem_wait(&sem_high);
        node = deque->high_head;
        deque->high_head = deque->high_head->next;
        if (deque->high_head == NULL) {
            deque->high_tail = NULL;}
	else{
            deque->high_head->prev = NULL;}
        deque->high_count--;
        *priority = HIGH_PRIORITY; // Set priority to high (queue 0)
    } else if (deque->medium_count > 0) {
        sem_wait(&sem_medium);
        node = deque->medium_head;
        deque->medium_head = deque->medium_head->next;
        if (deque->medium_head == NULL) {
            deque->medium_tail = NULL;}
	else {
            deque->medium_head->prev = NULL;}
        deque->medium_count--;
        *priority = MEDIUM_PRIORITY; // Set the priority to medium (queue 1)
    } else if (deque->low_count > 0) {
        sem_wait(&sem_low);
        node = deque->low_head;
        deque->low_head = deque->low_head->next;
        if (deque->low_head == NULL) {
            deque->low_tail = NULL;}
	else {
            deque->low_head->prev = NULL;}
        deque->low_count--;
        *priority = LOW_PRIORITY; // Sets to low priority (queue 2)
    } else{
        pthread_mutex_unlock(&deque_mutex);
        return -1;}
    strncpy(data, node->data, DATA_LENGTH);
    free(node);
    pthread_mutex_unlock(&deque_mutex); //protect data when changing
    return 0;}

////////////////////////////////////////////////////////////////////////////
// Function: delete_deque
// Deletes all elements in deque /frees allocated memory
/////////////////////////////////////////////////////////////////////////////
int delete_deque(void){
    pthread_mutex_lock(&deque_mutex);

    node_t *temp;
    while (deque->high_head){
        temp = deque->high_head;
        deque->high_head = deque->high_head->next;
        free(temp);}
    while (deque->medium_head){
        temp = deque->medium_head;
        deque->medium_head = deque->medium_head->next;
        free(temp);}
    while (deque->low_head){
        temp = deque->low_head;
        deque->low_head = deque->low_head->next;
        free(temp);}

    free(deque);
    deque = NULL;
    //SYNCHRONIZATION DESTROY
    sem_destroy(&sem_high); //semaphores from initialization
    sem_destroy(&sem_medium);
    sem_destroy(&sem_low);
    pthread_mutex_unlock(&deque_mutex); //for enqueue and dequeue
    pthread_mutex_destroy(&deque_mutex);

    return 0;}
/////////////////////////////////////////////////////////////////////////////
