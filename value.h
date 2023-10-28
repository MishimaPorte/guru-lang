#ifndef GURU_VALUE
#define GURU_VALUE

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "stddef.h"

#define GC_INITIAL_CAP 100
#define __INITIAL_BLOB_PIT 1024*1024
#define __INITIAL_OBJECT_PIT 1024*1024

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

    // reference count
    // used by The Kernel, so why not try?
    uint32_t __rc;
};

void init_pit();

void __gc_collect();
void collect_all();

#define __free_object(o) do { switch ((o)->tag) { \
    case BLOB_STRING: case BLOB_BLOB: case BLOB_INST: \
    case BLOB_VARINT: case BLOB_FUNCTION: free((void *) (o)->as.blob); \
    default: free((void *) (o)); }; } while(0)


struct __the_pit {
    struct {
        uint64_t ptr;
        uint64_t __first_free_offset;
        uint64_t cap;
        void *mem;
    } __blobs; // blob allocations like strings and literal blobs
    struct {
        uint64_t ptr;
        uint64_t cap;
        struct __guru_object *mem;
    } __objects; // object headers (and whole objects for simple data types)
};
static struct __the_pit pit;

/* Some reduced malloc implementation for use by the memory allocation subsystem of the Guru
 * */
struct __guru_object *__alloc_go_header(enum guru_type tt);
struct __blob_header *__alloc_blob(size_t s);
struct __blob_header *__realloc_blob(struct __blob_header *b, size_t s); // s - new size

struct gobj_a {
    uint16_t count;
    uint16_t capacity;
    struct __guru_object **vals;
};
void gdafree(struct gobj_a *c);
uint16_t gdaput(struct gobj_a *c, struct __guru_object *val);

// some alloc-dealloc function definitions macros
// alloc-dealloc function definitions for simple types since they all follow the same
// simple pattern
#define __go_malloc_for_type(tt, lift) \
    struct __guru_object *__alloc_##tt(void *val, size_t n) { \
        struct __guru_object *o = calloc(1, sizeof(struct __guru_object)); \
        o->tag = tt; o->__rc = 1; \
        memcpy(&o->as, val, n); return o; \
    };

#define __take_obj(o) (++(o)->__rc)
#define __untake_obj(o) (--(o)->__rc)

#define __GURU_FALSE ((struct __guru_object){VAL_BOOL, {.bool = 0}, 1})
#define __GURU_TRUE ((struct __guru_object){VAL_BOOL, {.bool = 1}, 1})
#define __GURU_NOTHING ((struct __guru_object){VAL_NOTHING, {.bytes_8 = 0}, 1})
#define __GURU_VOID ((struct __guru_object){VAL_VOID, {.bytes_8 = 0}, 1})

#define __as_guru_numeric(val) __alloc_VAL_NUMBER(&val, sizeof(typeof(val)))
#define __as_guru_blob(val, btt)  ((struct __guru_object){btt, {.blob = val}, 1})
#define __guru_not(v) ((v)->as.bool ^= 0x01)

#define __go_malloc_for_type_h(tt, lift) \
    struct __guru_object *__alloc_##tt(void *val, size_t n);

__go_malloc_for_type_h(VAL_NUMBER, numeric)
__go_malloc_for_type_h(VAL_BOOL, bool)
__go_malloc_for_type_h(BLOB_STRING, blob);

/*  some utility macros
 * */

#define mem(t) (pit.__##t.mem)
#define blob_at(s) (((struct __blob_header *)(mem(blobs) + (s))))
#define __next_blob(b) ((struct __blob_header *)(((void *) ((b)+1)) + (b)->len))
#define __ffb (((struct __blob_header *)(mem(blobs) + pit.__blobs.__first_free_offset)))

#endif
