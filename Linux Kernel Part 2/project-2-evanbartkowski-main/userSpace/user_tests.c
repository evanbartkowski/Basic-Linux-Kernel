#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "deque.h"

#define TEST_DATA_COUNT 3

void test_enqueue() {
    printf("|Testing enqueue operations|\n");
    for (int i = 0; i < TEST_DATA_COUNT; i++) {
        char data[DATA_LENGTH];
        snprintf(data, sizeof(data), "High Priority Task %d", i);
        if (enqueue_front(data, HIGH_PRIORITY) == 0) {
            printf("Enqueued: %s\n", data);
        } else {
            printf("Failed to enqueue: %s\n", data);
        }

        snprintf(data, sizeof(data), "Medium Priority Task %d", i);
        if (enqueue_front(data, MEDIUM_PRIORITY) == 0) {
            printf("Enqueued: %s\n", data);
        } else {
            printf("FailUre to enqueue: %s\n", data);}

        snprintf(data, sizeof(data), "Low Priority Task %d", i);
        if (enqueue_front(data, LOW_PRIORITY) == 0) {
            printf("Enqueued: %s\n", data);
        } else {
            printf("FailUre to enqueue: %s\n", data);}}}

void test_dequeue() {
    printf("Testing Testing, dequeue operations.....\n");
    for (int i = 0; i < TEST_DATA_COUNT * 3; i++) {
        char data[DATA_LENGTH];
        int priority;

        if (dequeue_front(data, &priority) == 0) {
            printf("Dequeued-- %s with priority-- %d\n", data, priority);
        } else {
            printf("No data to dequeue!!!!!!! \n");
            break;}}}

int main() {
    if (init_deque() != 0) {
        fprintf(stderr, "WARNING WARNING, FailURe to initialize deque\n");
        return EXIT_FAILURE;}

    test_enqueue();
    test_dequeue();
    delete_deque();

    return EXIT_SUCCESS;}
