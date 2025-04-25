/********************************************************************************
* File: img_funcs.h                                                             *
* Course: CMSC421 Operating Systems, Fall 2024, UMBC                            *
*                                                                               *
* About: Read image file and access FAT, superblock, etc.                       *
*                                                                               *
********************************************************************************/
#ifndef IMG_FUNCS_H
#define IMG_FUNCS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define BLOCK_SIZE 512 // # of bytes
#define SUPERBLOCK_SIZE 64 // # of bytes
#define DIRECTORY_STRUCTURE_SIZE 32 // # of bytes
#define FAT_LEN 256 // # of 16-bit integers
#define DIRECTORY_STRUCTURES_LEN 224 // # of 32-byte structures
#define USERDATA_LEN 220 // # of user data blocks

// locations (in blocks) of the various image file containers
#define IMG_SUPERBLOCK_INDEX 255
#define IMG_FAT_INDEX 254
#define IMG_DIRECTORY_STRUCTURE_INDEX 253
#define IMG_USERDATA_INDEX 1
#define IMG_SUPERBLOCK_INDEX_BACKUP 0
#define IMG_FAT_INDEX_BACKUP 239


////////////////////////////////////////////////////////////////////////////////
// for timestamps
static inline uint8_t pbcd(uint8_t num) {
    uint8_t a = num % 10, b = num / 10;
    return b <= 9 ? ((b << 4) | a) : 0xFF;
}

////////////////////////////////////////////////////////////////////////////////
// IMAGE FILE

extern FILE *img_file;

// load img_file
int img_load(const char *filename);

// unload img_file
int img_unload(void);

////////////////////////////////////////////////////////////////////////////////
// SUPERBLOCK

// set this before calling the FAT functions!!
// (this proj: 255 main / 0 backup)
extern int img_superblock_index; // automatically set in img_funcs.c

// superblock, stored in big endian (as per mkmemefs.c)
struct memefs_superblock {
  char signature[16];        // Filesystem signature
  uint8_t cleanly_unmounted; // Flag for unmounted state
  uint8_t reserved[3];       // Reserved bytes
  uint32_t fs_version;       // Filesystem version
  uint8_t fs_ctime[8];       // Creation timestamp in BCD format
  uint16_t main_fat;         // Starting block for main FAT
  uint16_t main_fat_size;    // Size of the main FAT
  uint16_t backup_fat;       // Starting block for backup FAT
  uint16_t backup_fat_size;  // Size of the backup FAT
  uint16_t directory_start;  // Starting block for directory
  uint16_t directory_size;   // Directory size in blocks
  uint16_t num_user_blocks;  // Number of user data blocks
  uint16_t first_user_block; // First user data block
  char volume_label[16];     // Volume label
  uint8_t unused[448];       // Unused space for alignment
} __attribute__((packed, scalar_storage_order("big-endian")));

// loaded with img_fetch_superblock after img_load
extern struct memefs_superblock superblock;

// copies superblock from img_file into superblock struct
int img_superblock_fetch(void);

// copies superblock struct to img_file
int img_superblock_write(void);

// for debugging or logging (e.g. pass with stdout or a log file)
void img_superblock_print(FILE *stream);

////////////////////////////////////////////////////////////////////////////////
// FILE ALLOCATION TABLE

// set this before calling the FAT functions!!
// (this proj: 254 main / 239 backup)
extern int img_fat_index;

// get the fat entry at index, store value in read_value
int img_fat_get(uint16_t index, uint16_t *read_value);

// set the fat entry at index with value
int img_fat_set(uint16_t index, uint16_t value);

////////////////////////////////////////////////////////////////////////////////
// DIRECTORY STRUCTURE

extern int img_superblock_index; // automatically set in img_funcs.c

struct memefs_directory_structure {
  uint16_t permissions;
  uint16_t first_block;
  char filename[11];
  uint8_t unused;
  uint8_t bcd_timestamp[8];
  uint32_t file_size;
  uint16_t owner_user_id;
  uint16_t owner_group_id;
} __attribute__((packed, scalar_storage_order("big-endian")));

extern struct memefs_directory_structure directory_structure;

// copies from img_file at file index into directory_structure struct
int img_directory_structure_fetch(uint16_t index);

// copies directory_structure struct to img_file at file index
int img_directory_structure_write(uint16_t index);

// for debugging or logging (e.g. pass with stdout or a log file)
void img_directory_structure_print(FILE *stream);

////////////////////////////////////////////////////////////////////////////////
// USER DATA

extern int img_userdata_index; // automatically set in img_funcs.c

// read userdata (1 block) into data at a particular file index
int img_userdata_get(uint16_t index, char data[BLOCK_SIZE]);

// write userdata (1 block) from data into a particular file index 
int img_userdata_set(uint16_t index, char data[BLOCK_SIZE]);

////////////////////////////////////////////////////////////////////////////////

#endif
