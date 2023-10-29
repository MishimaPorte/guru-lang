#ifndef GURU_VALUE
#define GURU_VALUE

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "stddef.h"

#define __INITIAL_BLOB_PIT 200

enum guru_type {
    VAL_BOOL, VAL_NOTHING, VAL_NUMBER,
    VAL_VOID,
    VAL_BYTE, VAL_2BYTE, VAL_4BYTE, VAL_8BYTE,
    BLOB_STRING, BLOB_INST, BLOB_VARINT, BLOB_FUNCTION,
    BLOB_BLOB, BLOB_UNUSED,

    __PIT_OBJECT_END
};

struct __blob_header {
    uint64_t len;
    uint64_t __rc;
    uint8_t __cont[];
};
struct __guru_object {
    enum guru_type tag;
    union {
        uint8_t bool;
        uint8_t byte;
        uint16_t bytes_2;
        uint32_t bytes_4;
        uint64_t bytes_8;
        double numeric;
        struct __blob_header *blob; // introducing CS language innovation: tagged heap blobs as 1-class values!
    } as;
};

// Garbage collection-related definitions
void __gc_collect();
void collect_all();

struct __the_pit {
    struct {
        uint64_t __first_free_offset;
        uint64_t cap;
        void *mem;
    } __blobs; // blob allocations like strings and literal blobs
};
static struct __the_pit pit;
void init_pit();

/* Some reduced malloc implementation for use by the memory allocation subsystem of the Guru
 * */
struct __guru_object *__alloc_go_header(enum guru_type tt);
struct __blob_header *__alloc_blob(size_t s);
struct __blob_header *__realloc_blob(struct __blob_header *b, size_t s); // s - new size

struct __consts {
    uint16_t count;
    uint16_t capacity;
    struct __guru_object *vals;
};
void __free_const_array(struct __consts *c);
uint16_t __add_const(struct __consts *c, void *val, size_t s, enum guru_type tt);

// static allocation of literals
// may be static allocation of numerics from 1 to 256 e.g.
static struct __guru_object __GURU_FALSE = (struct __guru_object){VAL_BOOL, {.bool = 0}};
static struct __guru_object __GURU_TRUE = (struct __guru_object){VAL_BOOL, {.bool = 1}};
static struct __guru_object __GURU_NOTHING = (struct __guru_object){VAL_NOTHING, {.bytes_8 = 0}};
static struct __guru_object __GURU_VOID = (struct __guru_object){VAL_VOID, {.bytes_8 = 0}};

/*  some utility macros
 * */

#define __mem(t) (pit.__##t.mem)
#define __blob_at(s) (((struct __blob_header *)(__mem(blobs) + (s))))
#define __next_blob(b) ((struct __blob_header *)(((void *) ((b)+1)) + (b)->len))
#define __ffb (((struct __blob_header *)(__mem(blobs) + pit.__blobs.__first_free_offset)))
#define FOR_OBJECT_o for (struct __guru_object *__iter_o = __mem(objects); __iter_o->tag != __PIT_OBJECT_END; __iter_o++)
void __debug_print_object_pool();
#endif
