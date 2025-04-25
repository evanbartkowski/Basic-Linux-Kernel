#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HIGH_PRIORITY 0
#define MEDIUM_PRIORITY 1
#define LOW_PRIORITY 2
#define DATA_LENGTH 256

/////////////////////////////////////////////////////////////////////////////
void test_enqueue_front(char *data, int priority) {
    long result = syscall(SYS_ENQUEUE_FRONT, data, priority);
    if (result == 0) {
        printf("Enqueue front successful for priority %d: %s :) \n", priority, data);
    } else {
        printf("Error enqueuing data: %s for priority %d\n", data, priority);}}
/////////////////////////////////////////////////////////////////////////////
void test_dequeue_front() {
    char data[DATA_LENGTH];
    int priority;
    long result = syscall(SYS_DEQUEUE_FRONT, data, &priority);

    if (result == 0) {
        printf("Dequeued data with priority %d: %s :) \n", priority, data);
    } else if (result == -ENODATA) {
        printf("No data to dequeue\n");
    } else {
        printf("Error dequeuing data\n");}}
/////////////////////////////////////////////////////////////////////////////
void test_delete_deque() {
    long result = syscall(SYS_DELETE_DEQUE);
    if (result == 0) {
        printf("Deque deleted successfully :) \n");
    } else {
        printf("Error deleting deque\n");}}
/////////////////////////////////////////////////////////////////////////////
int main() {
    printf("Initializing deque...\n");
    if (syscall(SYS_INIT_DEQUE) != 0) {
        printf("Error initializing deque.\n");
        return -1;}

    printf("Testing enqueuing...\n");
    test_enqueue_front("High Priority Task", HIGH_PRIORITY);
    test_enqueue_front("Medium Priority Task", MEDIUM_PRIORITY);
    test_enqueue_front("Low Priority Task", LOW_PRIORITY);

    printf("Testing dequeuing...........\n");
    test_dequeue_front();  // Should return high prio
    test_dequeue_front();  // Should return "Medium task
    test_dequeue_front();  // Should return "Low protity task
    test_dequeue_front();

    printf("Testing deletion currently ...\n");
    test_delete_deque();

    return 0;}
/////////////////////////////////////////////////////////////////////////////
