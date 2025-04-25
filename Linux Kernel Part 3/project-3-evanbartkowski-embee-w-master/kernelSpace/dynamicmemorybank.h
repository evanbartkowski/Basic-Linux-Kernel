#ifndef DYNAMICMEMORYBANK_H
#define DYNAMICMEMORYBANK_H

/*******************************************************************************
* dynamicmemorybank.h                                                          *
* compile with C99 standard.                                                   *
*                                                                              *
* Made for UMBC Operating Systems (CMSC421), Fall 2024.                        *
*                                                                              *
* Description: A crude memory management tool; handles dynamic memory          *
*              allocation & deallocation plus error handling via "borrowed"    *
*              pointers.                                                       *
*                                                                              *
* Note: I created this for Project 2 for this course, reused w/ permission     *
*       from prof.                                                             *
*                                                                              *
*******************************************************************************/

#include <stdbool.h>
#include <stddef.h>

// do not manually use malloc, realloc, free, etc
// do not manually touch MemBank contents
typedef struct DynamicMemoryBank {
  void **allocs;
  size_t size;
  size_t capacity;

  bool do_autohandle_err; // if true, handle all errors automatically (w.i.p.)
  
  // TODO: ADD MORE ERRORS AND (AUTOMATIC) ERROR HANDLING
  enum MemErr {MEMERR_NONE, MEMERR_UNKNOWN = 1} error;
} MemBank;

// metadata attached before each alloc'd segment
typedef struct DynamicMemoryBankPrependData {
  size_t index;
  size_t type_size;
} MemBankPrepend;

// -------------------------
// -- essential functions --
// -------------------------

// -initialize a MemBank struct
// -init_capacity -> number of pointers the MemBank will store at inception
void membank_init(MemBank *membank, size_t init_capacity);

// -destroy the MemBank struct and any dynamic memory under its jurisdiction
void membank_destroy(MemBank *membank);

// -get a pointer to a section of dynamic memory managed by the MemBank
// -automatically freed by membank_destroy()
// type_size -> the required memory space for the type you are trying to store
//              ( e.g. 'sizeof (char)' or 'sizeof (struct myStruct)' )
// num -> the number quantity of the type you are trying to store
// returns -> "borrowed" pointer to a section of memory you can read/write to
void* membank_alloc(MemBank *membank, size_t type_size, size_t num);

// -----------------------------
// -- useful but nonessential --
// -----------------------------

// -immediately free & release a pointer from the MemBank
// -POINTER SHOULD BE PREPENDED BY A VALID MEMBANKPREPEND
// -i.e. don't use external pointers, use the ones given by the MemBank
//      alloc functions
// to_free -> the "borrowed" pointer you want to deallocate from the MemBank
void membank_free(MemBank *membank, void *to_free); // optional

// -realloc a pointer from the Membank to be new_num * type_size long
// -ALWAYS ALWAYS update your pointer with the return value
// to_resize -> the "borrowed" pointer you would like to resize
// new_num -> the new number quantity of the type specified during alloc
// returns -> a (potentially) new "borrowed" pointer to the freshly sized alloc
void* membank_resize(MemBank *membank, void *to_resize, size_t new_num);

// - let the MemBank handle your errors? (default false)
// state -> whether the MemBank should handle its errors itself
void membank_set_autohandle(MemBank *membank, bool state);

#endif




/* TODO LIST

 - better error handling (it's all over the place right now)
   - log file, line #s, etc
 - pool memory rather than grab it on-demand
   - choose suitable data structures and logic
 - add more auxiliary functionality
   - e.g. optional bounds checking
 - use syscalls rather than wrapping malloc, realloc, etc to manage dynamic mem

 */
