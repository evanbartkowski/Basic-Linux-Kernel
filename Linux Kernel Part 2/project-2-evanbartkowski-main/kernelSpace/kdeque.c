#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <trace/syscall.h>
#include <linux/tracepoint.h>
#include <trace/events/syscalls.h>

#define DATA_LENGTH 256

// Priority Levels and correlating values
#define HIGH_PRIORITY 0
#define MEDIUM_PRIORITY 1
#define LOW_PRIORITY 2

// Structure of  deque node
typedef struct kdeque_node {
    char data[DATA_LENGTH];
    struct kdeque_node *next;
    struct kdeque_node *prev;
    int priority;} node_t;

// separate priority levels, mutex, sempahores for each
typedef struct {
    node_t *high_head;
    node_t *high_tail;
    node_t *medium_head;
    node_t *medium_tail;
    node_t *low_head;
    node_t *low_tail;
    int high_count, medium_count, low_count;
    struct mutex deque_mutex;
    struct semaphore sem_high, sem_medium, sem_low;} deque_421_t;

// allows user-space programs to interact with kernel
long sys_init_deque(void);
long sys_enqueue_front(char __user *data, int priority);
long sys_enqueue_rear(char __user *data, int priority);
long sys_dequeue_front(char __user *data, int __user *priority);
long sys_dequeue_rear(char __user *data, int priority);
long sys_delete_deque(void);

static deque_421_t *deque = NULL;

////////////////////////////////////////////////////////////////////////
// Function: sys_init_deque
// Initializes the deque with three separate priority double-sided queues.
////////////////////////////////////////////////////////////////////////
SYSCALL_DEFINE0(init_deque) {
    deque = kmalloc(sizeof(deque_421_t), GFP_KERNEL);
    if (!deque) {
        printk(KERN_ERR "Memory allocation FAILED for DEQUE \n");
        return -ENOMEM;}

    deque->high_head = deque->high_tail = NULL;
    deque->medium_head = deque->medium_tail = NULL;
    deque->low_head = deque->low_tail = NULL;
    deque->high_count = deque->medium_count = deque->low_count = 0;

    mutex_init(&deque->deque_mutex);
    sema_init(&deque->sem_high, 0);
    sema_init(&deque->sem_medium, 0);
    sema_init(&deque->sem_low, 0);

    printk(KERN_INFO "Deque HAS BEEN INITIALIZED.\n");
    return 0;}

/////////////////////////////////////////////////////////////////////////////
// Function: sys_enqueue_front
// Adds a data block to the front of picked priority queue.
/////////////////////////////////////////////////////////////////////////////
SYSCALL_DEFINE2(enqueue_front, const char __user *, data, int, priority) {
    node_t *new_node;
    char kdata[DATA_LENGTH];

    if (copy_from_user(kdata, data, DATA_LENGTH))
        return -EFAULT;

    new_node = kmalloc(sizeof(node_t), GFP_KERNEL);
    if (!new_node)
        return -ENOMEM;

    strncpy(new_node->data, kdata, DATA_LENGTH - 1);
    new_node->data[DATA_LENGTH - 1] = '\0';
    new_node->prev = NULL;

    mutex_lock(&deque->deque_mutex);

    if (priority == HIGH_PRIORITY){
        new_node->next = deque->high_head;
        if (deque->high_head == NULL){
            deque->high_tail = new_node;
        } else{
            deque->high_head->prev = new_node;}
        deque->high_head = new_node;
        deque->high_count++;
        up(&deque->sem_high);
    } else if (priority == MEDIUM_PRIORITY) {
        new_node->next = deque->medium_head;
        if (deque->medium_head == NULL) {
            deque->medium_tail = new_node;
        } else {
            deque->medium_head->prev = new_node;}
        deque->medium_head = new_node;
        deque->medium_count++;
        up(&deque->sem_medium);
    } else if (priority == LOW_PRIORITY) {
        new_node->next = deque->low_head;
        if (deque->low_head == NULL) {
            deque->low_tail = new_node;
        } else {
            deque->low_head->prev = new_node;
        }
        deque->low_head = new_node;
        deque->low_count++;
        up(&deque->sem_low);
    } else {
        kfree(new_node);
        mutex_unlock(&deque->deque_mutex);
        return -EINVAL;}

    mutex_unlock(&deque->deque_mutex);
    return 0;}

/////////////////////////////////////////////////////////////////////////////
// Function: sys_dequeue_front
// Removes a data block from the front of the highest priority queue.
/////////////////////////////////////////////////////////////////////////////
SYSCALL_DEFINE2(dequeue_front, char __user *, data, int __user *, priority) {
    node_t *node = NULL;
    int kpriority;

    mutex_lock(&deque->deque_mutex);

    if (deque->high_count > 0) {
        down(&deque->sem_high);
        node = deque->high_head;
        deque->high_head = deque->high_head->next;
        if (deque->high_head == NULL) {
            deque->high_tail = NULL;
        } else{
            deque->high_head->prev = NULL;}
        deque->high_count--;
        kpriority = HIGH_PRIORITY;

    } else if (deque->medium_count > 0) {
        down(&deque->sem_medium);
        node = deque->medium_head;
        deque->medium_head = deque->medium_head->next;
        if (deque->medium_head == NULL) {
            deque->medium_tail = NULL;
        } else {
            deque->medium_head->prev = NULL;}
        deque->medium_count--;
        kpriority = MEDIUM_PRIORITY;

    } else if (deque->low_count > 0) {
        down(&deque->sem_low);
        node = deque->low_head;
        deque->low_head = deque->low_head->next;
        if (deque->low_head == NULL) {
            deque->low_tail = NULL;
        } else{
            deque->low_head->prev = NULL;}
        deque->low_count--;
        kpriority = LOW_PRIORITY;

    } else {
        mutex_unlock(&deque->deque_mutex);
        return -ENODATA;} //noo data

    if (copy_to_user(data, node->data, DATA_LENGTH) || copy_to_user(priority, &kpriority, sizeof(int))) {
        kfree(node);
        mutex_unlock(&deque->deque_mutex);
        return -EFAULT;}

    kfree(node);
    mutex_unlock(&deque->deque_mutex);
    return 0;}
/////////////////////////////////////////////////////////////////////////////
// Function: sys_delete_deque
// Deletes all elements in the deque and frees allocated memory.
/////////////////////////////////////////////////////////////////////////////
SYSCALL_DEFINE0(delete_deque) {
    node_t *temp;

    mutex_lock(&deque->deque_mutex);
    //for high medium and low head
    while (deque->high_head) {
        temp = deque->high_head;
        deque->high_head = deque->high_head->next;
        kfree(temp);}
    while (deque->medium_head) {
        temp = deque->medium_head;
        deque->medium_head = deque->medium_head->next;
        kfree(temp);}
    while (deque->low_head) {
        temp = deque->low_head;
        deque->low_head = deque->low_head->next;
        kfree(temp);}

    kfree(deque);
    deque = NULL;

    mutex_unlock(&deque->deque_mutex);
    printk(KERN_INFO "Deque deleted successfully.\n");
    return 0;}

MODULE_LICENSE("GPL"); //stops weird error
/////////////////////////////////////////////////////////////////////////////
