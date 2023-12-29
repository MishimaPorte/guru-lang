#ifndef GURU_FFI
#define GURU_FFI

#include "value.h"
#include "vm.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>


typedef struct __guru_object (*__nat_func)(struct __guru_object *args);
#define nat_func(name, fs, ar, ...) struct __guru_object __##name(struct __guru_object *args); \
    static struct native_func name = {\
        .arity = (ar),\
        .flags = (fs),\
        .func = __##name,\
        .args_types = {__VA_ARGS__},\
    };\
    static size_t name##_size = sizeof(struct native_func) + ar * sizeof(uint8_t);

struct native_func {
    uint32_t flags;
    uint32_t arity;
    __nat_func func;
    uint8_t args_types[];
};

uint8_t typecheck(struct __guru_object *args, struct native_func*nf);

#define NONE 0x00
#define VARIADIC 0x01

nat_func(garbage_collect, NONE, 0);
nat_func(print_stack, NONE, 0);
nat_func(dump_pit, NONE, 0);

nat_func(print_file, VARIADIC, 2, VAL_FILE, VAL_ANY);
nat_func(read_file, NONE, 1, VAL_FILE);
nat_func(close_file, NONE, 1, VAL_FILE);
nat_func(get_time, NONE, 0);
nat_func(open_file, NONE, 2, BLOB_STRING, BLOB_STRING);
nat_func(umalloc, NONE, 1, VAL_NUMBER);
nat_func(ufree, NONE, 1, VAL_UNSAFE_ALLOCATED);

nat_func(alloc_array_refs, NONE, 1, VAL_NUMBER);
nat_func(alloc_dynamic_array, NONE, 1, VAL_NUMBER);
nat_func(arrlen, NONE, 1, VAL_ARRAY_REF);

void def_nats(struct guru_vm *v);

enum stdlib_natives {
    PRINT_FILE, TIME, ALLOC_UNSAFE,
};

#endif
