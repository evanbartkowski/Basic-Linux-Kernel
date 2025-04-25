/********************************************************************************
* File: img_funcs.c                                                             *
* Course: CMSC421 Operating Systems, Fall 2024, UMBC                            *
*                                                                               *
* About: Read image file and access FAT, superblock, etc.                       *
*                                                                               *
********************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "img_funcs.h"
///////////////////////////////////////////////////////////////////////////////////
// declare memory for superblock
struct memefs_superblock superblock;
// declare memory for directory_structure
struct memefs_directory_structure directory_structure;

// internal big endian formatter (workaround)
struct big_endian_16_bit {
  uint16_t value;
} __attribute__((scalar_storage_order("big-endian")));

// internal big endian formatter (workaround)
struct memefs_userdata {
  char data[BLOCK_SIZE];
} __attribute__((scalar_storage_order("big-endian")));

// starting indices in blocks for image file containers
// (can change manually, e.g. to save/load FAT backup)
int img_superblock_index = IMG_SUPERBLOCK_INDEX;
int img_fat_index = IMG_FAT_INDEX; // set manually for backup 
int img_directory_structure_index = IMG_DIRECTORY_STRUCTURE_INDEX;
int img_userdata_index = IMG_USERDATA_INDEX;

FILE *img_file = NULL;
////////////////////////////////////////////////////////////////////////////////
// IMAGE FILE
// load image file stream into memory
int img_load(const char *filename) {
  img_file = fopen(filename, "r+");
  if (img_file == NULL) {
    perror("Failed to open filesystem image file\n");
    return -1;
  }
  return 0;
}

// unload image file stream from memory
int img_unload(void) {
  int ret = fclose(img_file);
  if (ret != 0) {
    perror("Failed to close filesystem image file\n");
    return -1;
  }
  img_file = NULL;
  return 0;
}
////////////////////////////////////////////////////////////////////////////////
// SUPERBLOCK
// load superblock
int img_superblock_fetch(void) {
  if (!img_file) { fprintf(stderr, "err: img_file is NULL\n"); return -1; }

  // todo err handle these

  // go to location of superblock in file
  fseek(img_file, BLOCK_SIZE * img_superblock_index, SEEK_SET);
  // read into superblock struct
  fread(&superblock, SUPERBLOCK_SIZE, 1, img_file);

  return 0;
}
////////////////////////////////////////////////////////////////////////////////////
// save superblock
int img_superblock_write(void) {
  if (!img_file) { fprintf(stderr, "err: img_file is NULL\n"); return -1; }
  
  // go to location of superblock in file
  fseek(img_file, BLOCK_SIZE * img_superblock_index, SEEK_SET);
  // write superblock struct to file
  fwrite(&superblock, SUPERBLOCK_SIZE, 1, img_file);

  // save main superblock to backup if applicable
  if (img_superblock_index != IMG_SUPERBLOCK_INDEX_BACKUP) {
    fseek(img_file, BLOCK_SIZE * IMG_SUPERBLOCK_INDEX_BACKUP, SEEK_SET);
    fwrite(&superblock, SUPERBLOCK_SIZE, 1, img_file);
  }
  
  return 0;
}
/////////////////////////////////////////////////////////////////////////////
// print superblock info -> pass stdout or stderr
// append superblock info to file -> pass file stream
//                                   (in append mode or use fseek()
void img_superblock_print(FILE *stream) {
  // prints each struct member appropriately
  fprintf(stream, "signature:         \"%s\"\n", superblock.signature);
  fprintf(stream, "cleanly_unmounted: 0x%X\n", superblock.cleanly_unmounted);
  fprintf(stream,
	"reserved:          [ %u, %u, %u ]\n",
        superblock.reserved[0], superblock.reserved[1], superblock.reserved[2]);
  fprintf(stream, "fs_version:        %u\n", superblock.fs_version);
  fprintf(stream, "fs_ctime:          [ %x, %x, %x, %x, %x, %x, %x, %x ]\n",
	  superblock.fs_ctime[0], superblock.fs_ctime[1],
	  superblock.fs_ctime[2], superblock.fs_ctime[3],
	  superblock.fs_ctime[4], superblock.fs_ctime[5],
	  superblock.fs_ctime[6], superblock.fs_ctime[7]);
  fprintf(stream, "main_fat:          %u\n", superblock.main_fat);
  fprintf(stream, "main_fat_size:     %u\n", superblock.main_fat_size);
  fprintf(stream, "backup_fat:        %u\n", superblock.backup_fat);
  fprintf(stream, "backup_fat_size:   %u\n", superblock.backup_fat_size);
  fprintf(stream, "directory_start:   %u\n", superblock.directory_start);
  fprintf(stream, "directory_size:   %u\n", superblock.directory_size);
  fprintf(stream, "num_user_blocks:   %u\n", superblock.num_user_blocks);
  fprintf(stream, "first_user_block:  %u\n", superblock.first_user_block);
  fprintf(stream, "volume_label:      \"%s\"\n", superblock.volume_label);
  fflush(stream);
}
////////////////////////////////////////////////////////////////////////////////
// FILE ALLOCATION TABLE
// get the fat entry (16 bit) at index and save in value 
int img_fat_get(uint16_t index, uint16_t *read_value) {
  if (!img_file) { fprintf(stderr, "err: img_file is NULL\n"); return -1; }
  if ( !(0 <= index && index < FAT_LEN) ) {
    fprintf(stderr, "err: index %d illegal\n", index);
    return -1;
  }

  // go to position in file for requested FAT entry
  fseek(img_file, (BLOCK_SIZE * img_fat_index) + (2 * index), SEEK_SET);

  // read into big endian formatter
  struct big_endian_16_bit read_into;
  fread(&read_into, 2, 1, img_file);

  *read_value = read_into.value;

  return 0;
}
///////////////////////////////////////////////////////////////////////////////
// set the fat entry at index to value (16 bit)
int img_fat_set(uint16_t index, uint16_t value) {
  if (!img_file) { fprintf(stderr, "err: img_file is NULL\n"); return -1; }
  if ( !(0 <= index && index < FAT_LEN) ) {
    fprintf(stderr, "err: index %d illegal\n", index);
    return -1;
  }
		   
  // go to position in file for requested FAT entry
  fseek(img_file, (BLOCK_SIZE * img_fat_index) + (2 * index), SEEK_SET);

  // write from big endian formatter
  struct big_endian_16_bit write_from = { value };
  fwrite(&write_from, 2, 1, img_file);

  // save FAT entry to backup if applicable
  if (img_fat_index != IMG_FAT_INDEX_BACKUP) {
    fseek(img_file, BLOCK_SIZE * IMG_FAT_INDEX_BACKUP + (2 * index), SEEK_SET);
    fwrite(&write_from, 2, 1, img_file);
  }

  return 0;
}
////////////////////////////////////////////////////////////////////////////////
// DIRECTORY STRUCTURE
// load directory_structure
int img_directory_structure_fetch(uint16_t index) {
  if (!img_file) { fprintf(stderr, "err: img_file is NULL\n"); return -1; }
  if ( !(0 <= index && index < DIRECTORY_STRUCTURES_LEN) ) {
    fprintf(stderr, "err: index %d invalid\n", index);
    return -1;
  }

  //todo add err handling to these

  // go to loc of directory_structure in file
  fseek(img_file, BLOCK_SIZE * img_directory_structure_index
                  - (DIRECTORY_STRUCTURE_SIZE * index) - 32, SEEK_SET);
  // I have no idea why we have to subtract 32 here. ----^
  // It is a true magic number.
  // Because it magically makes the code work.
  // It took me two hours to fix this.
  // Blessed be the 32.
  // read it into memory
  fread(&directory_structure, DIRECTORY_STRUCTURE_SIZE, 1, img_file);

  return 0;
}
///////////////////////////////////////////////////////////////////////////////
// save directory_structure
int img_directory_structure_write(uint16_t index) {
  if (!img_file) { fprintf(stderr, "err: img_file is NULL\n"); return -1; }
  if ( !(0 <= index && index < DIRECTORY_STRUCTURES_LEN) ) {
    fprintf(stderr, "err: index %d invalid\n", index);
    return -1;
  }

  // go to loc of directory_structure in file
  fseek(img_file, BLOCK_SIZE * img_directory_structure_index
	         - (DIRECTORY_STRUCTURE_SIZE * index) - 32, SEEK_SET);
  // write it from memory to file
  fwrite(&directory_structure, DIRECTORY_STRUCTURE_SIZE, 1, img_file);
  return 0;
}
///////////////////////////////////////////////////////////////////////////////
// print directory_structure info -> pass stdout or stderr
// append directory_structure info to file -> pass file stream
//                                            (in append mode or use fseek() )
void img_directory_structure_print(FILE *stream) {

  fprintf(stream, "permissions:    0x%X\n", directory_structure.permissions);
  fprintf(stream, "first_block:    %u\n", directory_structure.first_block);
  fprintf(stream, "filename:       \"%s\"\n", directory_structure.filename);
  fprintf(stream, "unused:         %u\n", directory_structure.unused);
  fprintf(stream, "bcd_timestamp:  [ %x, %x, %x, %x, %x, %x, %x, %x ]\n",
	  directory_structure.bcd_timestamp[0],
	  directory_structure.bcd_timestamp[1],
	  directory_structure.bcd_timestamp[2],
	  directory_structure.bcd_timestamp[3],
	  directory_structure.bcd_timestamp[4],
	  directory_structure.bcd_timestamp[5],
	  directory_structure.bcd_timestamp[6],
	  directory_structure.bcd_timestamp[7]);
  fprintf(stream, "file_size:      %u\n", directory_structure.file_size);
  fprintf(stream, "owner_user_id:  %u\n", directory_structure.owner_user_id);
  fprintf(stream, "owner_group_id: %u\n", directory_structure.owner_group_id);
  fflush(stream);
  return;
}
////////////////////////////////////////////////////////////////////////////////
// USER DATA
// read userdata (1 block) into data at a particular file index
int img_userdata_get(uint16_t index, char data[BLOCK_SIZE]) {
  if (!img_file) { fprintf(stderr, "err: img_file is NULL\n"); return -1; }
  if ( !(0 <= index && index < USERDATA_LEN) ) {
    fprintf(stderr, "err: index %d invalid\n", index);
    return -1;
  }
  // go to indexed block within userdata section
  fseek(img_file, BLOCK_SIZE * img_userdata_index
	          + BLOCK_SIZE * index, SEEK_SET);

  // read data from file
  struct memefs_userdata read_into;
  fread(&read_into, BLOCK_SIZE, 1, img_file);
  // read data into data argument
  memcpy(data, &read_into, BLOCK_SIZE);

  return 0;
}
///////////////////////////////////////////////////////////////////////////////////
// write userdata (1 block) from data into a particular file index
int img_userdata_set(uint16_t index, char data[BLOCK_SIZE]) {
  if (!img_file) { fprintf(stderr, "err: img_file is NULL\n"); return -1; }
  if ( !(0 <= index && index < USERDATA_LEN) ) {
    fprintf(stderr, "err: index %d invalid\n", index);
    return -1;
  }

  // go to indexed block within userdata section
  fseek(img_file, BLOCK_SIZE * img_userdata_index
	          + BLOCK_SIZE * index, SEEK_SET);
  // convert data to Big Endia
  struct memefs_userdata write_from;
  memcpy(&write_from, data, BLOCK_SIZE);

  // write to file
  fwrite(&write_from, BLOCK_SIZE, 1, img_file);

  return 0;
}
///////////////////////////////////////////////////////////////////////////////////
