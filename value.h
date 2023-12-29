#ifndef GURU_VALUE
#define GURU_VALUE

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "hashmap.h"
#include "stddef.h"


#define __INITIAL_BLOB_PIT 10240
#define __INITIAL_STORAGE_PIT 0

/*
 * VAL_* things are inline abd BLOB_* things are heap-allocated
 * */
enum guru_type {
    VAL_BOOL, VAL_NOTHING, VAL_NUMBER,
    VAL_VOID,
    VAL_BYTE, VAL_2BYTE, VAL_4BYTE, VAL_8BYTE,
    VAL_FILE, VAL_UNSAFE_ALLOCATED,

    BLOB_CLASS,
    BLOB_MODULE,
    BLOB_STRING, BLOB_INST, BLOB_VARINT,
    BLOB_BLOB, BLOB_UNUSED,

    //a reference; is used in closures
    VAL_LINK, VAL_ARRAY_REF, VAL_DYNAMIC_ARRAY,

    BLOB_CALLABLE,
    BLOB_NATIVE,
    BLOB_CLOSURE_VARIABLE, //just a wrapper over a __guru_object

    VAL_ANY, //for natives that want to take any argument type

    __PIT_OBJECT_END
};

#ifdef DEBUG_MODE
static char *type_names_map[] = { [VAL_BOOL] = "VAL_BOOL",[VAL_NOTHING] = "VAL_NOTHING",[VAL_NUMBER] = "VAL_NUMBER", [VAL_VOID] = "VAL_VOID", [VAL_BYTE] = "VAL_BYTE",[VAL_2BYTE] = "VAL_2BYTE",[VAL_4BYTE] = "VAL_4BYTE",[VAL_8BYTE] = "VAL_8BYTE", [VAL_FILE] = "VAL_FILE", [BLOB_CLASS] = "BLOB_CLASS", [BLOB_MODULE] = "BLOB_MODULE", [BLOB_STRING] = "BLOB_STRING",[BLOB_INST] = "BLOB_INST",[BLOB_VARINT] = "BLOB_VARINT", [BLOB_BLOB] = "BLOB_BLOB",[BLOB_UNUSED] = "BLOB_UNUSED", [VAL_LINK] = "VAL_LINK", [BLOB_CALLABLE] = "BLOB_CALLABLE", [BLOB_NATIVE] = "BLOB_NATIVE", [BLOB_CLOSURE_VARIABLE] = "BLOB_CLOSURE_VARIABLE" , [VAL_ANY] = "VAL_ANY", [__PIT_OBJECT_END] = "__PIT_OBJECT_END",[VAL_UNSAFE_ALLOCATED] = "VAL_UNSAFE_ALLOCATED" };
#endif

struct __blob_header
{
    uint64_t len;
    uint64_t __rc;
    uint8_t __cont[];
};

//backward pointer walking assumes that all of our things like strings, closures and shit are allocated
//obly as blobs and in no other way
#define __as_blob(o) (((struct __blob_header*)(o)) - 1)
#define __take_as_blob(bh) (*(((uint64_t*)(bh)) - 1) += 1)
#define __untake_as_blob(bh) (*(((uint64_t*)(bh)) - 1) -= 1)

struct __guru_object
{
    enum guru_type tag;
    union {
        uint8_t bool;
        uint8_t byte;
        uint16_t bytes_2;
        uint32_t bytes_4;
        uint64_t bytes_8;
        void *pointer;
        double numeric;
        struct __blob_header *blob; // tagged heap blobs as 1-class values!
        struct __blob_header *func; // a function
    } as;
};

struct __value_link
{
    struct __guru_object o;
};

// Garbage collection-related definitions
uint64_t __gc_collect();
void collect_all();
struct blobs {
        uint64_t __first_free_offset;
        uint64_t cap;
        void *mem;
    };

static struct {
    struct hashmap int_strings;
    struct hashmap globals;
    struct blobs __blobs; // blob allocations like strings and literal blobs
    void *storage; // FIXME: unused???
} pit;

struct blobs* pit_mem();

void init_pit();

struct array_of_refs {
    uint64_t len;
    struct __guru_object *__vals[];
};

struct __blob_header *alloc_array_of_refs(size_t num);

uint8_t get_global(const void *name, size_t nsize, struct __guru_object *val);
void set_global(const void *name, size_t nsize, const struct __guru_object *val);

uint8_t test(const struct __guru_object *val);

struct __blob_header *get_int_str(const void *key, size_t ksize);

void obtake(struct __guru_object *o);
void obfree(struct __guru_object *o);

/* Some reduced malloc implementation for use by the memory allocation subsystem of the Guru
 * */
struct __blob_header *__alloc_blob(size_t s);
struct __blob_header *__realloc_blob(struct __blob_header *b, size_t s); // s - new size

struct guru_callable
{
    uint32_t func_begin;
    uint8_t __is_method;
    uint8_t arity; // you dont actually need more than 255 arguments to a function
    struct __guru_object *this;
    struct __value_link *closures[];
};

struct __blob_header *alloc_guru_callable(uint32_t start, uint8_t arity, uint8_t closures);

struct __consts
{
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
