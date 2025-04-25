/*******************************************************************************
* File: filesystem.c                                                           *
* Course: CMSC421 Operating Systems, Fall 2024, UMBC                           *
* By: Evan Bartkowski e168@umbc.edu, and  Benjamin Whitehurst b236@umbc.edu                                                                             *
* About: A Linux kernel module to read from a file in our FUSE mountpoint and  *
*        make availabe in the /dev/memefs node.                                *
*******************************************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/namei.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/stat.h>

// for managing dynamic memory
#include "dynamicmemorybank.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("project-3-evanbartkowski-embee-w");
MODULE_DESCRIPTION("Read-only MEMEfs Kernel Module with /dev entry");
///////////////////////////////////////////////////////////////////////////////////////////////
// get the path of the file as argument
static char *filepath = "/tmp/memefs/testfile"; // (default)
module_param(filepath, charp, 0);
MODULE_PARM_DESC(filepath, "Path to the file in MEMEfs to read");
///////////////////////////////////////////////////////////////////////////////////////////////
// this is the device's identifiers
static dev_t dev_num;
static struct cdev memefs_cdev;
static struct class *memefs_class;
///////////////////////////////////////////////////////////////////////////////////////////////
//data buffer
#define MEMEFS_BUFFER_SIZE 4096
MemBank memefs_membank;        //dynamic memory bank
static char *memefs_buffer;    //file content
static size_t memefs_data_len; //length of the data
///////////////////////////////////////////////////////////////////////////////////////////////
//declarations
static int memefs_open(struct inode *inode, struct file *file);
static int memefs_release(struct inode *inode, struct file *file);
static ssize_t memefs_read_dev(struct file *file, char __user *buf, size_t count, loff_t *ppos);
///////////////////////////////////////////////////////////////////////////////////////////////
//file operations (readonly)
static struct file_operations memefs_fops = {
    .owner = THIS_MODULE,
    .open = memefs_open,
    .release = memefs_release,
    .read = memefs_read_dev,
};
///////////////////////////////////////////////////////////////////////////////////////////////
// Reads file from provided mounted filepath to our kernel buffer to interchange between the two
static ssize_t read_file_to_kernel_buffer(const char *path, char **buffer, size_t *data_len) {
    struct path p;
    struct kstat stat;
    struct file *filp;
    loff_t pos = 0;
    ssize_t read_len;
    ssize_t file_size;
    int err;

    // looks up the path
    err = kern_path(path, LOOKUP_FOLLOW, &p);
    if (err) {
        printk(KERN_ERR "memefs_kmodule: Failed to resolve path %s, error: %d\n", path, err);
        return err;
    }

    //Uses getattr of file to retrieve its size
    err = vfs_getattr(&p, &stat, STATX_BASIC_STATS, AT_STATX_SYNC_AS_STAT);
    path_put(&p);
    if (err) {
        printk(KERN_ERR "memefs_kmodule: vfs_getattr failed for %s, error: %d\n", path, err);
        return err;
    }

    file_size = stat.size;
    if (file_size <= 0) {
        printk(KERN_INFO "memefs_kmodule: File %s is empty or size <= 0\n", path);
        return 0;
    }

    // avoid buffer overflow by temporarily resizing if necessary
    if (file_size >= MEMEFS_BUFFER_SIZE)
      membank_resize(&memefs_membank, &memefs_buffer, file_size);
    else
      membank_resize(&memefs_membank, &memefs_buffer, MEMEFS_BUFFER_SIZE);

    // opens
    filp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        printk(KERN_ERR "memefs_kmodule: Failed to open file %s\n", path);
        return PTR_ERR(filp);
    }

    // reads
    read_len = kernel_read(filp, *buffer, file_size, &pos);
    filp_close(filp, NULL);
    if (read_len < 0) {
        printk(KERN_ERR "memefs_kmodule: Failed to read file %s\n", path);
        return read_len;
    }

    // finally save to memefs buffer
    (*buffer)[read_len] = '\0';
    *data_len = read_len;
    return read_len;
}
///////////////////////////////////////////////////////////////////////////////////////////////
static int memefs_open(struct inode *inode, struct file *file) {
    //reads the file from /tmp/memefs ONLY WHEN DEVICE IS OPEN
    memefs_data_len = 0;
    ssize_t ret = read_file_to_kernel_buffer(filepath, &memefs_buffer, &memefs_data_len);
    if (ret < 0) {
        printk(KERN_ERR "memefs_kmodule: ERRROR reading file on open\n");
        return (int)ret;
    }

    printk(KERN_INFO "memefs_kmodule: Device opened. Read %zu bytes from %s\n", memefs_data_len, filepath);

    // put contents of file into dmesg
    if (memefs_data_len > 0) {
        printk(KERN_INFO "memefs_kmodule: Content of %s:\n%s\n", filepath, memefs_buffer);
    } else {
        printk(KERN_INFO "memefs_kmodule: File %s is empty or not accessible.\n", filepath);
    }
    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////
static int memefs_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "memefs_kmodule: Device closed\n");
    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////
static ssize_t memefs_read_dev(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    size_t remaining = 0;
    size_t to_copy = 0;

    if (*ppos >= memefs_data_len)
        return 0;

    // move position pointer to offset
    remaining = memefs_data_len - *ppos;
    to_copy = (count < remaining) ? count : remaining;

    // read from our memefs buffer
    if (copy_to_user(buf, memefs_buffer + *ppos, to_copy))
        return -EFAULT;

    *ppos += to_copy;
    return to_copy;
}
///////////////////////////////////////////////////////////////////////////////////////////////
static int __init memefs_init(void) {
    int ret;

    // allocate buffer with which to hold onto file contents
    membank_init(&memefs_membank, 1);
    memefs_buffer = membank_alloc(&memefs_membank, 1, MEMEFS_BUFFER_SIZE);
    if (memefs_membank.error != MEMERR_NONE) {
        printk(KERN_ERR "memefs_kmodule: Failed to allocate buffer\n");
        return -ENOMEM;
    }

    // acquires a major number for our device
    ret = alloc_chrdev_region(&dev_num, 0, 1, "memefs");
    if (ret < 0) {
        printk(KERN_ERR "memefs_kmodule: Failed to allocate major number\n");
        membank_destroy(&memefs_membank);
        return ret;
    }

    // creates the character device MODULE
    cdev_init(&memefs_cdev, &memefs_fops);
    memefs_cdev.owner = THIS_MODULE;

    ret = cdev_add(&memefs_cdev, dev_num, 1);
    if (ret < 0) {
        printk(KERN_ERR "memefs_kmodule: Failed to add cdev\n");
        unregister_chrdev_region(dev_num, 1);
        membank_destroy(&memefs_membank);
        return ret;
    }

    // add the device entry into sysfs
    memefs_class = class_create(THIS_MODULE, "memefs_class");
    if (IS_ERR(memefs_class)) {
        printk(KERN_ERR "memefs_kmodule: Failed to create class\n");
        cdev_del(&memefs_cdev);
        unregister_chrdev_region(dev_num, 1);
        kfree(memefs_buffer);
        return PTR_ERR(memefs_class);
    }

    // makes a node for the device in /dev (ls /dev/) dont forget to remove node after use
    if (!device_create(memefs_class, NULL, dev_num, NULL, "memefs")) {
        printk(KERN_ERR "memefs_kmodule: Failed to create device\n");
        class_destroy(memefs_class);
        cdev_del(&memefs_cdev);
        unregister_chrdev_region(dev_num, 1);
        membank_destroy(&memefs_membank);
        return -1;
    }

    printk(KERN_INFO "memefs_kmodule: Loaded with major=%d, minor=%d\n", MAJOR(dev_num), MINOR(dev_num));
    printk(KERN_INFO "memefs_kmodule: Device node should be /dev/memefs\n");
    printk(KERN_INFO "memefs_kmodule: Inserted. Open /dev/memefs to read from %s\n", filepath);

    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////
//  module removed, CLEANS UP EVERYTHIBNG
static void __exit memefs_exit(void) {
    device_destroy(memefs_class, dev_num);
    class_destroy(memefs_class);
    cdev_del(&memefs_cdev);
    unregister_chrdev_region(dev_num, 1);
    membank_destroy(&memefs_membank);

    printk(KERN_INFO "memefs_kmodule: Unloaded\n");
}
///////////////////////////////////////////////////////////////////////////////////////////////
module_init(memefs_init);
module_exit(memefs_exit);
///////////////////////////////////////////////////////////////////////////////////////////////
