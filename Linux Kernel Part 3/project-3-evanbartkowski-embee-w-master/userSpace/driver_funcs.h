#ifndef DRIVER_FUNCS_H
#define DRIVER_FUNCS_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#define FUSE_USE_VERSION 31
#include <fuse.h>

/////////////////////////////////////////////////////////////////////////////
// Debug File
extern FILE *debug_logger;

// File structure for representing files in the filesystem
typedef struct file_t {
    char *name;            // File name
    char *contents;        // File contents
    size_t size;           // File size in bytes
    struct file_t *next;   // Pointer to the next file in the linked list
} file_t;

// Function prototypes for file operations
void init_driver(const char filename[12], const char volume_label[16]);
void cleanup_driver();
uint16_t find_file(const char *name);
uint16_t create_file(const char *name);
void delete_file(const char *name);

// FUSE operations
int driver_getattr(const char *path, struct stat *stbuf);
int driver_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int driver_open(const char *path, struct fuse_file_info *fi);
int driver_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int driver_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int driver_unlink(const char *path);
int driver_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int driver_truncate(const char *path, off_t size);
int driver_utimens(const char *path, const struct timespec ts[2]);

#endif // DRIVER_FUNCS_H
