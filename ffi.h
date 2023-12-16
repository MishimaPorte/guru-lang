#ifndef GURU_FFI
#define GURU_FFI

#include "value.h"
#include "vm.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct __guru_object (*__nat_func)(struct __guru_object *args);
#define nat_func(name, fs, ar, ...) struct __guru_object __##name(struct __guru_object *args); \
    static struct native_func name = {\
        .arity = (ar),\
        .flags = (fs),\
        .func = __##name,\
        .args_types = {__VA_ARGS__},\
    };

struct native_func {
    uint32_t flags;
    uint32_t arity;
    __nat_func func;
    uint8_t args_types[];
};

#define NONE 0x00
#define VARIADIC 0x01

nat_func(print_file, VARIADIC, 2, VAL_FILE, VAL_ANY);
nat_func(get_time, NONE, 0);
nat_func(open_file, NONE, 2, BLOB_STRING, BLOB_STRING);
nat_func(alloc_unsafe, NONE, 1, VAL_NUMBER);

void def_nats(struct guru_vm *v);

enum stdlib_natives {
    PRINT_FILE, TIME, ALLOC_UNSAFE,
};

#endif
