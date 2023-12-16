#include "ffi.h"
#include "value.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct __guru_object __alloc_unsafe(struct __guru_object *args) {
    return __GURU_VOID;
};

struct __guru_object __open_file(struct __guru_object *args) {
    char *name = (char*)args++->as.blob->__cont;
    FILE *f = fopen(name, (char*)args->as.blob->__cont);
    if (f == NULL) return __GURU_NOTHING;
    return (struct __guru_object){
        .tag = VAL_FILE,
        .as.pointer = f,
    };
}

struct __guru_object __print_file(struct __guru_object *args) {
    FILE *f = (FILE*)(args++)->as.pointer;
    switch (args->tag) {
        case VAL_BOOL: 
             fprintf(f, "%s\n", args->as.bool ? "true" : "false");
             break;
        case VAL_NUMBER: 
             fprintf(f, "%f\n", args->as.numeric);
             break;
        case VAL_NOTHING:
             fprintf(f, "nothing\n");
             break;
        case BLOB_CALLABLE: {
             struct guru_callable *func = (struct guru_callable*) args->as.blob->__cont;
             fprintf(f, "[function] (%d:%d)\n", func->func_begin, func->arity);
             break;
        }
        case VAL_VOID: 
             fprintf(f, "void\n");
             break;
        case BLOB_STRING:
             fprintf(f, "%s\n", args->as.blob->__cont);
             break;
        default:
             fprintf(f, "unprintable\n");
             return __GURU_VOID;
    }
    return __GURU_VOID;
};
struct __guru_object __get_time(struct __guru_object *args) {
    time_t t = time(NULL);
    return (struct __guru_object){
        .tag = VAL_NUMBER,
        .as.numeric = t,
    };
};

void def_nats(struct guru_vm *v) {
    struct __guru_object *o = malloc(sizeof(struct __guru_object));
    struct __blob_header *bh;
    o->tag = BLOB_NATIVE;

    bh = __alloc_blob(sizeof(struct native_func));
    memcpy(bh->__cont, &print_file, bh->len);
    o->as.blob = bh;
    set_global("printf", 7, o);

    bh = __alloc_blob(sizeof(struct native_func));
    memcpy(bh->__cont, &get_time, bh->len);
    o->as.blob = bh;
    set_global("get_time", 9, o);

    bh = __alloc_blob(sizeof(struct native_func));
    memcpy(bh->__cont, &open_file, bh->len);
    o->as.blob = bh;
    set_global("open_file", 10, o);

    o->tag = VAL_FILE;
    o->as.pointer = stdout;
    set_global("stdout", 7, o);

    o->tag = VAL_FILE;
    o->as.pointer = stderr;
    set_global("stderr", 7, o);

    free(o);
};
