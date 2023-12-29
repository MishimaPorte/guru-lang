#include "ffi.h"
#include "hashmap.h"
#include "object.h"
#include "scanner.h"
#include "value.h"
#include "vm.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static struct guru_vm *__running_vm = NULL;

struct __guru_object __ufree(struct __guru_object *args)
{
    free(args->as.pointer);
    return __GURU_VOID;
};

struct __guru_object __umalloc(struct __guru_object *args)
{
    void *mem = malloc((int)args->as.numeric);
    if (mem == NULL)
        return __GURU_VOID;
    else
        return (struct __guru_object) {
            .tag = VAL_UNSAFE_ALLOCATED,
            .as.pointer = mem,
        };
};

uint8_t typecheck(struct __guru_object *args, struct native_func*nf)
{
    for (size_t i = 0; i < nf->arity; i++)
    {
        if (args[i].tag != nf->args_types[i] && nf->args_types[i] != VAL_ANY)
            return 0;
    }

    return 1;
};

struct __guru_object __dump_pit(struct __guru_object *args)
{
    uint64_t i = 0;
    printf("[DEBUG] Dumping pit\n");
    for (struct __blob_header *h = (struct __blob_header*) pit_mem()->mem; i != pit_mem()->cap;) {
        printf("length: %lu, rc: %lu, i: %lu, content: %.*s, ptr: %p\n", h->len, h->__rc, i, h->len, h->__cont, h);
        i += (sizeof(struct __blob_header) + h->len);
        h = __next_blob(h);
    }
    printf("[DEBUG] End\n");
    return (struct __guru_object) {
        .tag = VAL_NUMBER,
        .as.numeric =  (double) i,
    };
}

struct __guru_object __print_stack(struct __guru_object *args)
{
    printf("[DEBUG] Dumping stack\n");
    for (struct __guru_object *o = __running_vm->stack.head - 1; o != __running_vm->stack.stack; o--)
    {
        printf("\t");
        __fprint_val(stdout, o);
    }
    printf("[DEBUG] End\n");
    return __GURU_NOTHING;
}

struct __guru_object __garbage_collect(struct __guru_object *args)
{
    __gc_collect();
    return __GURU_NOTHING;
}

struct __guru_object __open_file(struct __guru_object *args)
{
    char *name = (char*)args++->as.blob->__cont;
    FILE *f = fopen(name, (char*)args->as.blob->__cont);
    if (f == NULL) return __GURU_NOTHING;
    return (struct __guru_object){
        .tag = VAL_FILE,
        .as.pointer = f,
    };
}

struct __guru_object __alloc_array_refs(struct __guru_object *args)
{
    struct __blob_header *bh = alloc_array_of_refs((size_t)args->as.numeric);
    return (struct __guru_object){
        .tag = VAL_ARRAY_REF,
        .as.blob = bh,
    };
};

struct __guru_object __alloc_dynamic_array(struct __guru_object *args)
{
    return (struct __guru_object){
        .tag = VAL_NUMBER,
        .as.numeric = ((struct array_of_refs*)args->as.blob->__cont)->len,
    };
};

struct __guru_object __arrlen(struct __guru_object *args)
{
    return (struct __guru_object){
        .tag = VAL_NUMBER,
        .as.numeric = ((struct array_of_refs*)args->as.blob->__cont)->len,
    };
};

struct __guru_object __close_file(struct __guru_object *args)
{
    FILE *f = (FILE*)args->as.pointer;
    fclose(f);
    return (struct __guru_object){
        .tag = VAL_NOTHING,
        .as.byte = 0,
    };
};

struct __guru_object __read_file(struct __guru_object *args)
{
    FILE *f = (FILE*)args->as.pointer;

    fseek(f, 0l, SEEK_END);
    size_t fsize = ftell(f);
    rewind(f);
    struct __blob_header *chars = __alloc_blob(fsize + 1);
    if (chars == NULL) return (struct __guru_object){
        .tag = VAL_VOID,
        .as.byte = 0,
    };
    size_t i = fread((char*) chars->__cont, sizeof(char), fsize, f);
    if (i != fsize) return (struct __guru_object){
        .tag = VAL_VOID,
        .as.byte = 0,
    };;
    ((char*) chars->__cont)[i] = '\0';

    return (struct __guru_object){
        .tag = BLOB_STRING,
        .as.blob = chars,
    };
}

struct __guru_object __print_file(struct __guru_object *args)
{
    FILE *f = (FILE*)(args++)->as.pointer;
    __fprint_val(f, args);
    return __GURU_VOID;
};

struct __guru_object __get_time(struct __guru_object *args)
{
    time_t t = time(NULL);
    return (struct __guru_object){
        .tag = VAL_NUMBER,
        .as.numeric = t,
    };
};

void def_nats(struct guru_vm *v)
{

    __running_vm = v;

    struct __guru_object *o = malloc(sizeof(struct __guru_object));
    struct __blob_header *bh;

    *o = define_class("adam", 5);
    set_global("adam", sizeof("adam"), o);

    struct __blob_header *syst = alloc_instance((struct guru_class*)o->as.blob->__cont);
    o->tag = BLOB_INST;
    o->as.blob = syst;
    set_global("system", sizeof("system"), o);

    struct __blob_header *obj;

#define define_member_for_instance(nomen, parent)  \
    struct __blob_header *nomen = alloc_instance((struct guru_class*)o->as.blob->__cont);\
    obj = __alloc_blob(sizeof(struct __guru_object)); \
    ((struct __guru_object *)obj->__cont)->tag = BLOB_INST; \
    ((struct __guru_object *)obj->__cont)->as.blob = nomen; \
    set_field((struct guru_instance*)parent->__cont, #nomen, sizeof(#nomen), (struct __guru_object *)obj->__cont);

    define_member_for_instance(io, syst);
    define_member_for_instance(debug, syst);
    define_member_for_instance(unsafe, syst);
    define_member_for_instance(arrays, syst);


    obj = __alloc_blob(sizeof(struct __guru_object));
    ((struct __guru_object *)obj->__cont)->tag = VAL_FILE;
    ((struct __guru_object *)obj->__cont)->as.pointer = stderr;
    set_field((struct guru_instance*)io->__cont, "stderr", 7, (struct __guru_object *)obj->__cont);

    obj = __alloc_blob(sizeof(struct __guru_object));
    ((struct __guru_object *)obj->__cont)->tag = VAL_FILE;
    ((struct __guru_object *)obj->__cont)->as.pointer = stdout;
    set_field((struct guru_instance*)io->__cont, "stdout", 7, (struct __guru_object *)obj->__cont);

    obj = __alloc_blob(sizeof(struct __guru_object));
    ((struct __guru_object *)obj->__cont)->tag = VAL_FILE;
    ((struct __guru_object *)obj->__cont)->as.pointer = stdin;
    set_field((struct guru_instance*)io->__cont, "stdin", 6, (struct __guru_object *)obj->__cont);

    struct __blob_header *func_o;
    
#define define_native_in_class(nomen, inst) do{\
    bh = __alloc_blob(nomen##_size);\
    memcpy(bh->__cont, &nomen, bh->len);\
    func_o = __alloc_blob(sizeof(struct __guru_object));\
    ((struct __guru_object *)func_o->__cont)->tag = BLOB_NATIVE;\
    ((struct __guru_object *)func_o->__cont)->as.blob = bh;\
    set_field((struct guru_instance*)inst->__cont, #nomen, sizeof(#nomen), (struct __guru_object *)func_o->__cont);\
    } while (0)

    define_native_in_class(get_time, syst);

    define_native_in_class(print_file, io);
    define_native_in_class(open_file, io);
    define_native_in_class(close_file, io);
    define_native_in_class(read_file, io);

    define_native_in_class(garbage_collect, debug);
    define_native_in_class(print_stack, debug);
    define_native_in_class(dump_pit, debug);

    define_native_in_class(umalloc, unsafe);
    define_native_in_class(ufree, unsafe);

    define_native_in_class(alloc_array_refs, arrays);
    define_native_in_class(arrlen, arrays);

#undef define_native_in_class

    free(o);
};
