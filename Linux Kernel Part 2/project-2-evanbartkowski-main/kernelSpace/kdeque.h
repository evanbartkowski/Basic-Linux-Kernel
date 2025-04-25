#ifndef _KDEQUE_H
#define _KDEQUE_H

#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/tracepoint.h>
#include <trace/syscall.h>
#include <linux/syscalls.h>
#include <trace/events/syscalls.h>

// In your kdeque.h file
static inline void enter_syscall_print_funcs(void) {}
static inline void exit_syscall_print_funcs(void) {}
static inline void event_class_syscall_enter(void) {}
static inline void event_class_syscall_exit(void) {}

#define DATA_LENGTH 256

// Priority Levels
#define HIGH_PRIORITY 0
#define MEDIUM_PRIORITY 1
#define LOW_PRIORITY 2

// Structur of  deque node
typedef struct kdeque_node {
    char data[DATA_LENGTH];
    struct kdeque_node *next;
    struct kdeque_node *prev;
    int priority;
} node_t;

// Structure
typedef struct {
    node_t *high_head;
    node_t *high_tail;
    node_t *medium_head;
    node_t *medium_tail;
    node_t *low_head;
    node_t *low_tail;
    int high_count, medium_count, low_count;
    struct mutex deque_mutex;
    struct semaphore sem_high, sem_medium, sem_low;
} deque_421_t;

// trying to allow  user-space programs to interact with kernel
long sys_init_deque(void);
long sys_enqueue_front(char __user *data, int priority);
long sys_enqueue_rear(char __user *data, int priority);
long sys_dequeue_front(char __user *data, int priority);
long sys_dequeue_rear(char __user *data, int priority);
long sys_delete_deque(void);

#endif
