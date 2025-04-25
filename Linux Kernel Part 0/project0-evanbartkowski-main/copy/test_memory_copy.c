#include <stdio.h>
#include <unistd.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <errno.h>

#define __NR_memory_copy 549

long memory_copy(unsigned char *to, unsigned char *from, int size) {
    return syscall(__NR_memory_copy, to, from, size);
}

int main() {
    unsigned char src[] = "Hello, Kernel!";
    unsigned char dest[20] = {0};
    long result;

    result = memory_copy(dest, src, sizeof(src));
    if (result == 0) {
        printf("Memory copy successful: %s\n", dest);
    } else {
        perror("Memory copy failed");
    }

    return 0;
}
