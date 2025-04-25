#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/errno.h>

SYSCALL_DEFINE3(memory_copy, unsigned char __user *, to, unsigned char __user *, from, int, size) {
    int i;
    unsigned char tmp;

    // Check if the user space pointers are valid for the size specified.
    if (!access_ok(to, size) || !access_ok(from, size)) {
        return -EFAULT;  // Invalid memory access.
    }

    // Copy each byte from the source to the destination.
    for (i = 0; i < size; ++i) {
        // Use get_user to read the byte from the source pointer.
        if (get_user(tmp, from + i)) {
            return -EFAULT;
        }

        // Use put_user to write the byte to the destination pointer.
        if (put_user(tmp, to + i)) {
            return -EFAULT;
        }
    }

    return 0;  // Return 0 on successful copy.
}
