/********************************************************************************
* File: driver_funcs.c                                                          *
* Course: CMSC421 Operating Systems, Fall 2024, UMBC                            *
*                                                                               *
* About: Has the functions to read & modify files for our driver.               *
*                                                                               *
********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#define FUSE_USE_VERSION 31
#include <fuse.h>
#include "img_funcs.h"
#include "driver_funcs.h"
/////////////////////////////////////////////////////////////////////////////
static file_t *root_files = NULL;
// Counter for files in the root directory // todo store this in superblock
static int root_files_count = 0;
FILE *debug_logger = NULL;
/////////////////////////////////////////////////////////////////////////////
// Initializates the filesystem driver, loads the filesystem image, sets up fat index,
//writes updated superblock
void init_driver(const char *filename, const char volume_label[16]) {
  fprintf(stderr, "initializing driver\n");
  if (img_load(filename) < 0) {
    fprintf(stderr, "Failed to load MEMEfs image.\n");
    exit(1);
  }
  // load superblock from image file
  img_superblock_index = IMG_SUPERBLOCK_INDEX;
  if (img_superblock_fetch() < 0) {
    fprintf(stderr, "Failed to fetch superblock.\n");
    img_unload();
    exit(1);}

  // update superbloc
  img_superblock_fetch();
  superblock.cleanly_unmounted = 0xFF;

  if (volume_label)
    strncpy(superblock.volume_label, volume_label, 16);

  img_fat_index = IMG_FAT_INDEX;
  img_fat_set(0, 0xFFFF);
  img_superblock_write();
  img_superblock_print(stdout);
  fprintf(stderr, "driver initialization complete!\n");
}
/////////////////////////////////////////////////////////////////////////////
// Cleans up the filesystem before exiting
void cleanup_driver() {
  fprintf(stderr, "closing driver!!!\n");
  img_unload();
  superblock.cleanly_unmounted = 0x0; // indicates umount has been successfully completed
}
/////////////////////////////////////////////////////////////////////////////
// Find and locate a specific file in the filesystem by inputting the name and traversing
// the FAT to locate the file
uint16_t find_file(const char *name) {
  fprintf(stderr, "finding file of name: %s\n", name);
  // todo also check length of filename?
  img_fat_index = IMG_FAT_INDEX;
  // traverse through linked list until filename or tail found
  uint16_t curr_index = 0;
  uint16_t next_index = 0;
  do {
    curr_index = next_index;
    img_fat_get(curr_index, &next_index);
    // check directory structure for index
    img_directory_structure_fetch(curr_index);
    // check filename (todo also need to check perms)
    char curr_filename[12] = { 0 };
    strncpy(curr_filename, directory_structure.filename, 11);
    if (strcmp(curr_filename, name) == 0){
      fprintf(stderr, " -> successfully searched file: %s\n", name);
      return curr_index;
    }
  }
  while (next_index != 0xFFFF);
  // debug
  fprintf(stderr, " -> did not find file %s\n", name);
  return USERDATA_LEN;
}
/////////////////////////////////////////////////////////////////////////////
// Create a new file using the name input with MEMEfs-based allocation,  finds a free fat
// block for the new file and updates FAT to link the file
uint16_t create_file(const char *name) {
    fprintf(stderr, "Creating new file with name: %s\n", name);
    img_fat_index = IMG_FAT_INDEX;
    // empty FAT entry
    uint16_t recycle_index;
    uint16_t i = 0;
    do {
        img_fat_get(i, &recycle_index);
        if (recycle_index == 0x0000) break;
        i += 1;
    } while (i < FAT_LEN);

    if (i >= FAT_LEN) {
        fprintf(stderr, "Error: Out of space for new files!\n");
        return USERDATA_LEN;
    }
    recycle_index = i;
    fprintf(stderr, "Found free space at index %u\n", recycle_index);

    uint16_t curr_index = 0;
    uint16_t next_index = 0;
    do {
        curr_index = next_index;
        img_fat_get(curr_index, &next_index);
    } while (next_index != 0xFFFF);

    img_fat_set(curr_index, recycle_index);
    img_fat_set(recycle_index, 0xFFFF);

    // Fetch directory structure for the new file index
    img_directory_structure_fetch(recycle_index);
    directory_structure.permissions = 0xCACA; // Temp permissions
    directory_structure.first_block = recycle_index;
    strncpy(directory_structure.filename, name, 8); //only up to 8 character name
    directory_structure.file_size = 0;
    directory_structure.owner_group_id = 0xBEEB;

    // Set initial timestamps
    time_t now = time(NULL);
    struct tm *ts = gmtime(&now);

    directory_structure.bcd_timestamp[0] = pbcd((ts->tm_year + 1900) / 100); // Year high
    directory_structure.bcd_timestamp[1] = pbcd((ts->tm_year + 1900) % 100); // Year low
    directory_structure.bcd_timestamp[2] = pbcd(ts->tm_mon + 1);             // Month
    directory_structure.bcd_timestamp[3] = pbcd(ts->tm_mday);                // Day
    directory_structure.bcd_timestamp[4] = pbcd(ts->tm_hour);                // Hour
    directory_structure.bcd_timestamp[5] = pbcd(ts->tm_min);                 // Minute
    directory_structure.bcd_timestamp[6] = pbcd(ts->tm_sec);                 // Second

    img_directory_structure_write(recycle_index);

    // Update superblock and save to disk
    img_superblock_fetch();
    superblock.directory_size += 1;
    img_superblock_write();
    
    // Reset user data to an empty block
    char nil[BLOCK_SIZE] = {0};
    img_userdata_set(recycle_index, nil);
    fprintf(stderr, "Created file: %s\n", name);
    return recycle_index;
}
/////////////////////////////////////////////////////////////////
// Delete a file by inputting name and freeing up its FAT blocks, clearing directory entry
// and removes its metadata
void delete_file(const char *name) {
    file_t **current = &root_files;
    while (*current) {
        if (strcmp((*current)->name, name) == 0) {
            file_t *to_delete = *current;
            *current = (*current)->next;

            // Free FAT blocks
            struct memefs_directory_structure dir_entry;
            img_directory_structure_fetch(to_delete->size);

            uint16_t block = dir_entry.first_block;
            while (block != 0xFFFF) {
                uint16_t next_block;
                img_fat_get(block, &next_block);
                img_fat_set(block, 0x0000);
                block = next_block;
            }

            memset(&dir_entry, 0, sizeof(dir_entry));
            img_directory_structure_write(to_delete->size);

            free(to_delete->name);
            free(to_delete);
            return;
        }
        current = &((*current)->next);
    }
}
/////////////////////////////////////////////////////////////////////////////
// Handle file attribute requests, fetching size, timestamps, permissions
int driver_getattr(const char *path, struct stat *stbuf) {
    fprintf(stderr, "driver_getattr called on path %s\n", path);
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        // Root directory attributes
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        // File attributes
        int file_index = find_file(path + 1);
        if (file_index >= USERDATA_LEN) {
            return -ENOENT; // File not found
        }

        img_directory_structure_fetch(file_index);

        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = directory_structure.file_size;

        // Decode BCD timestamps
        struct tm ts = {0};
        ts.tm_year = (((directory_structure.bcd_timestamp[0] >> 4) * 10) + (directory_structure.bcd_timestamp[0] & 0x0F)) * 100 +
                     ((directory_structure.bcd_timestamp[1] >> 4) * 10 + (directory_structure.bcd_timestamp[1] & 0x0F)) - 1900;
        ts.tm_mon = ((directory_structure.bcd_timestamp[2] >> 4) * 10 + (directory_structure.bcd_timestamp[2] & 0x0F)) - 1;
        ts.tm_mday = (directory_structure.bcd_timestamp[3] >> 4) * 10 + (directory_structure.bcd_timestamp[3] & 0x0F);
        ts.tm_hour = (directory_structure.bcd_timestamp[4] >> 4) * 10 + (directory_structure.bcd_timestamp[4] & 0x0F);
        ts.tm_min = (directory_structure.bcd_timestamp[5] >> 4) * 10 + (directory_structure.bcd_timestamp[5] & 0x0F);
        ts.tm_sec = (directory_structure.bcd_timestamp[6] >> 4) * 10 + (directory_structure.bcd_timestamp[6] & 0x0F);

        time_t epoch_time = timegm(&ts); // Convert to epoch time
        stbuf->st_atime = epoch_time;
        stbuf->st_mtime = epoch_time;
        stbuf->st_ctime = epoch_time;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Lists files in a directory by traversing FAT and listing names in the directory (ls command)
int driver_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "driver_readdir called on path %s\n", path);
    (void)offset;
    (void)fi;

    if (strcmp(path, "/") != 0)
        return -ENOENT;
    // traverse the linked list
    uint16_t curr_index = 0;
    uint16_t next_index = 0;
    do {
      // lookup next index
      curr_index = next_index;
      img_fat_get(curr_index, &next_index);
      // check directory structure for current index
      img_directory_structure_fetch(curr_index);
      // check filename (todo also need to check perms)
      char curr_filename[12] = { 0 };

      fprintf(stderr, "dir struct filename = %s\n", directory_structure.filename);
      strncpy(curr_filename, directory_structure.filename, 11);
      if (curr_filename[0] != '\0')
	// add to filler
	filler(buf, curr_filename, NULL, 0);
    }
    while (next_index != 0xFFFF);
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
// Create a new file in the system, allocates space, ensures file does nto already exist
int driver_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    fprintf(stderr, "driver_create called on path %s\n", path);
    (void)mode;
    (void)fi;

    if (find_file(path+1) < USERDATA_LEN)
        return -EEXIST; //command for already exists

    if (create_file(path+1) >= USERDATA_LEN)
        return -ENOMEM;
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
// Remove a file from the system by path
int driver_unlink(const char *path) {
    fprintf(stderr, "driver_unlink called on path %s\n", path);
    if (find_file(path+1) >= USERDATA_LEN)
        return -ENOENT;
    delete_file(path+1);
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
// Handle reading requests for a file, reading block by block in the buffer provided
int driver_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "driver_read called on path %s\n", path);
    (void)fi;

    int file_index = find_file(path+1);
    if (file_index >= USERDATA_LEN)
        return -ENOENT;

    img_directory_structure_fetch(file_index);

    uint16_t block = directory_structure.first_block;
    size_t read_size = 0;
    char block_buf[BLOCK_SIZE];

    while (block != 0xFFFF && read_size < size) {
        img_userdata_get(block, block_buf);
        size_t to_copy = BLOCK_SIZE - (size_t)offset < size - read_size ? BLOCK_SIZE - (size_t)offset : size - read_size;
        memcpy(buf + read_size, block_buf + offset, to_copy);
        read_size += to_copy;
        img_fat_get(block, &block);
        offset = 0;
    }
    return read_size;
}
/////////////////////////////////////////////////////////////////////////////
// Finds the file and writes data to its blocks, if it needs more data it  allocates
// new fat blocks for additonal data
int driver_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "driver_write called on path %s\n", path);
    (void)fi;

    int file_index = find_file(path+1);
    if (file_index >= USERDATA_LEN)
        return -ENOENT;

    img_directory_structure_fetch(file_index);

    uint16_t block = directory_structure.first_block;
    char block_buf[BLOCK_SIZE];
    size_t written = 0;

    while (written < size) {
        if (block == 0xFFFF) {
            for (int i = 0; i < FAT_LEN; i++) {
                uint16_t value;
                if (img_fat_get(i, &value) == 0 && value == 0x0000) {
                    img_fat_set(block, i);
                    block = i;
                    img_fat_set(block, 0xFFFF);
                    break;
                }
            }
        }

        img_userdata_get(block, block_buf);
        size_t to_copy = BLOCK_SIZE - (size_t)offset < size - written ? BLOCK_SIZE - (size_t)offset : size - written;
        memcpy(block_buf + offset, buf + written, to_copy);
        img_userdata_set(block, block_buf);
        written += to_copy;
        offset = 0;

        img_fat_get(block, &block);
    }

    directory_structure.file_size += written;
    img_directory_structure_write(file_index);

    return written;
}

/////////////////////////////////////////////////////////////////////////////
// Handles all resizing, making it either bigger or smaller by adjusting fat to either
// release or allocate more blocks
int driver_truncate(const char *path, off_t size) {
    fprintf(stderr, "driver_truncate called on path %s\n", path);
    int file_index = find_file(path+1);
    if (file_index >= USERDATA_LEN)
        return -ENOENT;

    img_directory_structure_fetch(file_index);

    // commented out for the time being
    /*
    if ((uint32_t)size < directory_structure.file_size ) { 
        uint16_t block = directory_structure.first_block;
        while (size > BLOCK_SIZE) {
            img_fat_get(block, &block);
            size -= BLOCK_SIZE;
        }
        img_fat_set(block, 0xFFFF);
    } else {
        uint16_t block = directory_structure.first_block;
        while (block != 0xFFFF) {
            img_fat_get(block, &block);
        }
        for (int i = 0; i < FAT_LEN; i++) {
            uint16_t value;
            if (img_fat_get(i, &value) == 0 && value == 0x0000) {
                img_fat_set(block, i);
                img_fat_set(i, 0xFFFF);
                break;
            }
        }
    }
    */
    directory_structure.file_size = (uint32_t)size;
    img_directory_structure_write(file_index);
    return 0;
}
/////////////////////////////////////////////////////////////////////
// opens the  file and checks if it exists using find_file
int driver_open(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "driver_open called on path %s\n", path);
    (void)fi;

    int file = find_file(path+1);
    if (file >= USERDATA_LEN)
      return -ENOENT;
    return 0;
}
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
// updates file timestamps by converting timestamps to BCD format and writing them 
// to directory_struc.bcd_timestamp
int driver_utimens(const char *path, const struct timespec ts[2]) {
    fprintf(stderr, "driver_utimens called on path %s\n", path);

    // Locates the file
    int file_index = find_file(path + 1);
    if (file_index >= USERDATA_LEN) {
        return -ENOENT;
    }

    // Fetchs the directory structure for the file
    img_directory_structure_fetch(file_index);

    // Update the bcd_timestamp field with the new times
    struct tm *tm_access = gmtime(&ts[0].tv_sec); // Access time
    struct tm *tm_mod = gmtime(&ts[1].tv_sec);    // Modification time

    // Format timestamps into BCD
    directory_structure.bcd_timestamp[0] = pbcd((tm_mod->tm_year + 1900) / 100); // Year high
    directory_structure.bcd_timestamp[1] = pbcd((tm_mod->tm_year + 1900) % 100); // Year low
    directory_structure.bcd_timestamp[2] = pbcd(tm_mod->tm_mon + 1);             // Month
    directory_structure.bcd_timestamp[3] = pbcd(tm_mod->tm_mday);                // Day
    directory_structure.bcd_timestamp[4] = pbcd(tm_mod->tm_hour);                // Hour
    directory_structure.bcd_timestamp[5] = pbcd(tm_mod->tm_min);                 // Minute
    directory_structure.bcd_timestamp[6] = pbcd(tm_mod->tm_sec);                 // Second

    img_directory_structure_write(file_index);

    fprintf(stderr, "Updated timestamps for file %s\n", path + 1);
    return 0;
}
/////////////////////////////////////////////////////////////////////

