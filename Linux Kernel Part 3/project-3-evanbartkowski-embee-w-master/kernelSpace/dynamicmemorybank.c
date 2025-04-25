/*******************************************************************************
* dynamicmemorybank.c                                                          *
* compile with C99 standard.                                                   *
*                                                                              *
* Made for UMBC Operating Systems (CMSC421), Fall 2024.                        *
*                                                                              *
* Description: Implementation of dynamicmemorybank.h . Modified to fit kernel  *
*              requirements.                                                   *
*                                                                              *
*******************************************************************************/
#include <linux/module.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("project-3-evanbartkowski-embee-w");
MODULE_DESCRIPTION("Read-only MEMEfs Kernel Module with /dev entry");

#include <linux/slab.h>
#include "dynamicmemorybank.h"


// initialize a MemBank struct and allocate memory for its contents
void membank_init(MemBank *membank, size_t init_capacity) {
  if (membank == NULL) return; // maybe perror this
  if (init_capacity == 0) {
    membank->allocs = NULL;
    membank->size = 0;
    membank->capacity = 0;
    membank->error = MEMERR_UNKNOWN;
    return;
  }

  // set initial state and dynamically allocate space for pointers
  membank->allocs = kmalloc(sizeof (void*) * init_capacity, GFP_KERNEL);
  if (membank->allocs == NULL) { // kmalloc fail
    membank->error = MEMERR_UNKNOWN; 
    return;
  }
  membank->size = 0;
  membank->capacity = init_capacity;
  membank->do_autohandle_err = false;
  membank->error = MEMERR_NONE;
}


// deallocate the contents of a MemBank struct
void membank_destroy(MemBank *membank) {
  if (membank == NULL) {
    printk(KERN_ERR "MemErr: membank does not exist.\n");
    return;
  }
  if (membank->allocs == NULL || membank->capacity == 0) {
    printk(KERN_ERR "MemErr: cannot destroy uninitialized MemBank.\n");
    membank->error = MEMERR_UNKNOWN;
    return;
  }

  // pls do checks on size and capacity

  // free all memory within the membank
  for (size_t i = 0; i < membank->size; i++) {
    // pls ERROR CHECK THE FREES
    kfree(membank->allocs[i]);
    membank->allocs[i] = NULL;
  }
  kfree(membank->allocs);
  membank->allocs = NULL;
  membank->error = MEMERR_NONE;
}


// add a new dynamic memory pointer to a MemBank struct
void* membank_alloc(MemBank *membank, size_t type_size, size_t num) {
  // validate membank exists and is initialized
  if (membank == NULL || membank->allocs == NULL || membank->capacity == 0) {
    if (membank) membank->error = MEMERR_UNKNOWN;
    printk(KERN_ERR "MemErr: membank does not exist or is not initialized.\n");
    return NULL;
  }
  // ensure user did not specify 0 bytes to be allocated
  if (type_size == 0 || num == 0) {
    membank->error = MEMERR_UNKNOWN;
    printk(KERN_ERR "MemErr: Cannot allocate 0 bytes.\n");
    return NULL;
  }
  
  // pls kmalloc more safely
  void *new_mem = kmalloc(type_size * num
			  + sizeof (MemBankPrepend), GFP_KERNEL);
  if(new_mem == NULL) {
    printk(KERN_CRIT "kmalloc failed");
    if (membank->do_autohandle_err) return NULL; // unused in kernel module
    membank->error = MEMERR_UNKNOWN;
    return NULL;
  }
    
  // prepend the data
  MemBankPrepend *prepend = new_mem;
  *prepend = (MemBankPrepend){membank->size, type_size};
  

  // increase internal space if pointer list is full
  if (membank->size >= membank->capacity) {
    void **new_allocs = krealloc(membank->allocs, membank->capacity * 2, GFP_KERNEL);
    // handle realloc failure (W.I.P.)
    if (new_allocs == NULL) {
      membank->error = MEMERR_UNKNOWN;
      kfree(new_mem);
      printk(KERN_CRIT "kmalloc failed");
      if (membank->do_autohandle_err) return NULL; // unused in kernel module
      return NULL;
    }
    membank->capacity *= 2;
    membank->allocs = new_allocs;
  }

  // track the newly allocated memory
  membank->allocs[membank->size] = new_mem;
  membank->size += 1;
  
  membank->error = MEMERR_NONE;
  return new_mem + sizeof (MemBankPrepend);
} 


// immediately free & release a pointer from the memory bank
// POINTER SHOULD BE PREPENDED BY A VALID MEMBANKPREPEND
// i.e. don't use external pointers,
//      use the ones given by the MemBank alloc functions
void membank_free(MemBank *membank, void *to_free) {
  // validate membank exists and is initialized
  if (membank == NULL || membank->allocs == NULL || membank->capacity == 0) {
    if (membank) membank->error = MEMERR_UNKNOWN;
    printk(KERN_ERR "MemErr: membank does not exist or is not initialized.\n");
    return;
  }
  // validate to_free is not bad
  if (to_free == NULL) {
    membank->error = MEMERR_UNKNOWN;
    printk(KERN_ERR "MemErr: freeing a NULL pointer is supicious.\n");
    return;
  }

  // walk backwards from pointer to check prepend to get internal index
  size_t internal_index =
    ((MemBankPrepend*)(to_free - sizeof (MemBankPrepend)))->index;

  if (internal_index >= membank->size) {
    // error trying to free a pointer that doesn't exist
    printk(KERN_ERR "MemErr: the given pointer does not exist in this MemBank. Be careful!\n");
    membank->error = MEMERR_UNKNOWN;
    return;
  }
  else if (internal_index == membank->size - 1) {
    // if we are releasing the last pointer in the array, simply remove it
    kfree(membank->allocs[internal_index]);
    membank->allocs[internal_index] = NULL;
    membank->size -= 1;
  }
  else {
    // we release to_free and swap last pointer of internal array to the "hole"
    kfree(membank->allocs[internal_index]);
    membank->allocs[internal_index] = membank->allocs[membank->size - 1];
    // update the prepend to reflect new internal index of the pointer we moved
    ((MemBankPrepend*)membank->allocs[internal_index])->index = internal_index;
    membank->size -= 1;
  }

  membank->error = MEMERR_NONE;
  return;
}


void* membank_resize(MemBank *membank, void *to_resize, size_t new_num) {
  // validate membank exists and is initialized
  if (membank == NULL || membank->allocs == NULL || membank->capacity == 0) {
    if (membank) membank->error = MEMERR_UNKNOWN;
    printk(KERN_ERR "MemErr: membank does not exist or is not initialized.\n");
    return NULL;
  }
  // ensure user did not specify 0 bytes to be allocated
  if (to_resize == NULL || new_num == 0) {
    membank->error = MEMERR_UNKNOWN;
    printk(KERN_ERR "MemErr: Cannot allocate 0 bytes.\n");
    return NULL;
  }

  // walk backwards from pointer to check prepend to get internal index
  MemBankPrepend* prepend = to_resize - sizeof (MemBankPrepend);
  size_t internal_index = prepend->index;
  
  if (internal_index >= membank->size) {
    // error trying to resize pointer that doesn't exist
    membank->error = MEMERR_UNKNOWN;
    printk(KERN_ERR "MemErr: trying to resize pointer that doesn't exist.\n");
    return NULL;
  }
  if (membank->allocs[internal_index] == NULL) {
    // error trying to resize non-alloc'd pointer
    membank->error = MEMERR_UNKNOWN;
    printk(KERN_ERR "MemErr: trying to resize non-alloc'd pointer.\n");
    return NULL;
  }
  
  // pls realloc more safely
  void *new_mem = krealloc(membank->allocs[internal_index],
			  prepend->type_size * new_num + sizeof (MemBankPrepend), GFP_KERNEL);
  if(new_mem == NULL) {
    // perror this
    membank->error = MEMERR_UNKNOWN;
    return NULL;
  }  

  membank->allocs[internal_index] = new_mem;
  
  membank->error = MEMERR_NONE;
  return new_mem + sizeof (MemBankPrepend);
}


void membank_set_autohandle(MemBank *membank, bool state) {
  // validate membank exists
  if (membank == NULL) {
    printk(KERN_ERR "MemErr: membank does not exist.\n");
    return;
  }
  membank->do_autohandle_err = state;
}



EXPORT_SYMBOL(membank_init);
EXPORT_SYMBOL(membank_destroy);
EXPORT_SYMBOL(membank_alloc);
EXPORT_SYMBOL(membank_free);
EXPORT_SYMBOL(membank_resize);
EXPORT_SYMBOL(membank_set_autohandle);

